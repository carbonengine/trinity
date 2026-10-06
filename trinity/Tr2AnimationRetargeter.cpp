// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2AnimationRetargeter.h"
#include "Tr2GrannyAnimation.h"
#include "Tr2Renderer.h"
#include "Resources/TriGrannyRes.h"

#include <cmf/transforms.h>

#include <cfloat>

// Quaternions compose like the rest of the cmf code: XMQuaternionMultiply( a, b ) applies a, then b, and
// XMVector3Rotate applies a rotation to a vector.  A world rotation takes bone space to model space.

namespace
{
constexpr uint32_t NO_BONE = 0xFFFFFFFF;
constexpr uint32_t GROUND_SAMPLES = 32;	// poses per cycle looked at to find where the feet come down
constexpr float FOOT_BAND = 0.05f;	// mapped bones this close to the lowest in the target bind pose are feet

uint32_t FindBone( const cmf::Skeleton& skeleton, const std::string& name )
{
	for( uint32_t i = 0; i < skeleton.bones.size(); ++i )
	{
		if( cmf::ToStdStringView( skeleton.bones[i] ) == name )
		{
			return i;
		}
	}
	return NO_BONE;
}

// Parents precede children, so a real parent always has the smaller index
bool HasParent( const cmf::Skeleton& skeleton, uint32_t bone )
{
	return skeleton.parents[bone] < bone;
}

void ComputeWorld( const cmf::Skeleton& skeleton, const cmf::Transform* locals, std::vector<cmf::Transform>& world )
{
	world.resize( skeleton.bones.size() );
	for( uint32_t i = 0; i < world.size(); ++i )
	{
		world[i] = HasParent( skeleton, i ) ? cmf::Multiply( locals[i], world[skeleton.parents[i]] ) : locals[i];
	}
}

// The shortest rotation taking unit vector from onto unit vector to
Quaternion ArcBetween( const Vector3& from, const Vector3& to )
{
	const float cosAngle = Dot( from, to );
	if( cosAngle > 0.99999f )
	{
		return IdentityQuaternion();
	}
	// Opposite vectors: half a turn about any perpendicular axis
	const Vector3 axis = Normalize( cosAngle < -0.99999f ? Cross( from, fabsf( from.x ) < 0.9f ? Vector3( 1.f, 0.f, 0.f ) : Vector3( 0.f, 1.f, 0.f ) ) : Cross( from, to ) );
	Quaternion arc = XMQuaternionRotationNormal( axis, acosf( std::max( -1.f, cosAngle ) ) );

	// Keep whichever sense actually turns from onto to
	const Quaternion other = XMQuaternionConjugate( arc );
	if( Dot( Vector3( XMVector3Rotate( from, other ) ), to ) > Dot( Vector3( XMVector3Rotate( from, arc ) ), to ) )
	{
		arc = other;
	}
	return arc;
}

const cmf::Data* GetData( const TriGrannyResPtr& res )
{
	if( !res || !res->IsGood() )
	{
		return nullptr;
	}
	const auto* data = res->GetCMFData();
	return data && !data->skeletons.empty() ? data : nullptr;
}

const cmf::Data* GetClipData( const TriGrannyResPtr& res )
{
	const auto* data = GetData( res );
	return data && !data->animations.empty() ? data : nullptr;
}

float Wrap01( float x )
{
	x = fmodf( x, 1.f );
	return x < 0.f ? x + 1.f : x;
}

float Clamp01( float x )
{
	return x > 0.f ? ( x < 1.f ? x : 1.f ) : 0.f;	// NaN comes out as 0
}
}

Tr2AnimationRetargeter::Tr2AnimationRetargeter( IRoot* lockobj ) :
	m_weight( 1.f ),
	m_speed( 1.f ),
	m_blend( 0.f ),
	m_sourcePhaseOffset( 0.f ),
	m_blendSourcePhaseOffset( 0.f ),
	m_phase( 0.f ),
	m_lastTime( 0.f ),
	m_clockStarted( false ),
	m_needsBind( true ),
	m_groundCalibrated( false ),
	m_boundTarget( nullptr ),
	m_boundTargetBoneCount( 0 ),
	m_clipCount( 0 ),
	m_pelvisTarget( NO_BONE ),
	m_pelvisSourceBindHeight( 0.f ),
	m_pelvisTargetBindPosition( 0.f, 0.f, 0.f ),
	m_pelvisHeightScale( 1.f )
{
}

Tr2AnimationRetargeter::~Tr2AnimationRetargeter()
{
	if( m_animation && m_animation->GetPoseModifier() == this )
	{
		m_animation->SetPoseModifier( nullptr );
	}

	// The players point into the clips' data, so they go before the resources
	Unbind();

	auto release = [this]( TriGrannyResPtr& res ) {
		if( res )
		{
			res->RemoveNotifyTarget( this );
		}
	};
	release( m_sourceRes );
	release( m_blendSourceRes );
	release( m_sourceBindPoseRes );
	release( m_targetBindPoseRes );
}

Tr2GrannyAnimation* Tr2AnimationRetargeter::GetAnimation() const
{
	return m_animation;
}

void Tr2AnimationRetargeter::SetAnimation( Tr2GrannyAnimation* animation )
{
	if( m_animation && m_animation->GetPoseModifier() == this )
	{
		m_animation->SetPoseModifier( nullptr );
	}

	m_animation = animation;

	if( m_animation )
	{
		if( m_animation->GetPoseModifier() && m_animation->GetPoseModifier() != this )
		{
			CCP_LOGWARN( "Tr2AnimationRetargeter: replacing the animation's existing pose modifier" );
		}
		m_animation->SetPoseModifier( this );
	}

	m_needsBind = true;
}

const std::string& Tr2AnimationRetargeter::GetSourcePath() const
{
	return m_sourcePath;
}

void Tr2AnimationRetargeter::SetSourcePath( const std::string& path )
{
	m_sourcePath = path;
	LoadRes( m_sourcePath, m_sourceRes );
}

const std::string& Tr2AnimationRetargeter::GetBlendSourcePath() const
{
	return m_blendSourcePath;
}

void Tr2AnimationRetargeter::SetBlendSourcePath( const std::string& path )
{
	m_blendSourcePath = path;
	LoadRes( m_blendSourcePath, m_blendSourceRes );
}

const std::string& Tr2AnimationRetargeter::GetSourceBindPosePath() const
{
	return m_sourceBindPosePath;
}

void Tr2AnimationRetargeter::SetSourceBindPosePath( const std::string& path )
{
	m_sourceBindPosePath = path;
	LoadRes( m_sourceBindPosePath, m_sourceBindPoseRes );
}

const std::string& Tr2AnimationRetargeter::GetTargetBindPosePath() const
{
	return m_targetBindPosePath;
}

void Tr2AnimationRetargeter::SetTargetBindPosePath( const std::string& path )
{
	m_targetBindPosePath = path;
	LoadRes( m_targetBindPosePath, m_targetBindPoseRes );
}

void Tr2AnimationRetargeter::MapBone( const std::string& sourceBone, const std::string& targetBone )
{
	m_boneMap.push_back( { sourceBone, targetBone, false } );
	m_needsBind = true;
}

void Tr2AnimationRetargeter::MapBoneFromBind( const std::string& sourceBone, const std::string& targetBone )
{
	m_boneMap.push_back( { sourceBone, targetBone, true } );
	m_needsBind = true;
}

void Tr2AnimationRetargeter::ClearBoneMap()
{
	m_boneMap.clear();
	m_needsBind = true;
}

bool Tr2AnimationRetargeter::IsReady() const
{
	return GetClipData( m_sourceRes ) && ( m_blendSourcePath.empty() || GetClipData( m_blendSourceRes ) ) && GetData( m_sourceBindPoseRes ) && GetData( m_targetBindPoseRes );
}

void Tr2AnimationRetargeter::LoadRes( const std::string& path, TriGrannyResPtr& res )
{
	// Drop anything pointing into the old data first
	Unbind();
	m_needsBind = true;

	if( res )
	{
		res->RemoveNotifyTarget( this );
		res.Unlock();
	}

	if( !path.empty() )
	{
		BeResMan->GetResource( path.c_str(), "raw", BlueInterfaceIID<TriGrannyRes>(), (void**)&res );
	}

	if( res )
	{
		res->AddNotifyTarget( this );
	}
}

void Tr2AnimationRetargeter::ReleaseCachedData( BlueAsyncRes* )
{
	Unbind();
	m_needsBind = true;
}

void Tr2AnimationRetargeter::RebuildCachedData( BlueAsyncRes* )
{
	m_needsBind = true;
}

void Tr2AnimationRetargeter::Unbind()
{
	for( Clip& clip : m_clips )
	{
		clip.player.reset();
		clip.skeleton = nullptr;
		clip.duration = 0.f;
		clip.sourceBones.clear();
		clip.pelvisSource = NO_BONE;
		clip.groundOffset = 0.f;
	}
	m_clipCount = 0;
	m_boundTarget = nullptr;
	m_boundTargetBoneCount = 0;
	m_links.clear();
	m_linkForTargetBone.clear();
	m_feet.clear();
	m_pelvisTarget = NO_BONE;
	m_groundCalibrated = false;
}

bool Tr2AnimationRetargeter::Bind( const cmf::Skeleton& target )
{
	Unbind();
	m_needsBind = false;

	const cmf::Data* clipData[2] = { GetClipData( m_sourceRes ), m_blendSourcePath.empty() ? nullptr : GetClipData( m_blendSourceRes ) };
	const uint32_t clipCount = m_blendSourcePath.empty() ? 1 : 2;
	const auto* sourceBindData = GetData( m_sourceBindPoseRes );
	const auto* targetBindData = GetData( m_targetBindPoseRes );
	if( !clipData[0] || ( clipCount == 2 && !clipData[1] ) || !sourceBindData || !targetBindData || m_boneMap.empty() )
	{
		// Try again once the missing piece arrives (RebuildCachedData, MapBone)
		return false;
	}

	const cmf::Skeleton& sourceBind = sourceBindData->skeletons[0];
	const cmf::Skeleton& targetBind = targetBindData->skeletons[0];

	std::vector<cmf::Transform> sourceBindWorld;
	std::vector<cmf::Transform> targetBindWorld;
	ComputeWorld( sourceBind, sourceBind.restTransforms.data(), sourceBindWorld );
	ComputeWorld( targetBind, targetBind.restTransforms.data(), targetBindWorld );

	// Resolve every pair in each skeleton; bone order differs between files
	struct Resolved
	{
		uint32_t target, sourceBind, targetBind;
		uint32_t source[2];
		bool fromBind;
	};
	std::vector<Resolved> resolved;
	std::vector<int32_t> resolvedForTarget( target.bones.size(), -1 );
	for( const BoneMapping& mapping : m_boneMap )
	{
		Resolved r{ FindBone( target, mapping.target ), FindBone( sourceBind, mapping.source ), FindBone( targetBind, mapping.target ), { NO_BONE, NO_BONE }, mapping.fromBind };
		bool missing = r.target == NO_BONE || r.sourceBind == NO_BONE || r.targetBind == NO_BONE;
		for( uint32_t c = 0; c < clipCount; ++c )
		{
			r.source[c] = FindBone( clipData[c]->skeletons[0], mapping.source );
			missing = missing || r.source[c] == NO_BONE;
		}
		if( missing )
		{
			CCP_LOGWARN( "Tr2AnimationRetargeter: skipping %s -> %s, a bone is missing from one of the skeletons", mapping.source.c_str(), mapping.target.c_str() );
			continue;
		}
		if( resolvedForTarget[r.target] >= 0 )
		{
			continue;
		}
		resolvedForTarget[r.target] = (int32_t)resolved.size();
		resolved.push_back( r );
	}
	if( resolved.empty() )
	{
		return false;
	}

	// Nearest mapped ancestor of each target bone, from the live skeleton
	auto mappedAncestor = [&]( uint32_t bone ) -> int32_t {
		while( HasParent( target, bone ) )
		{
			bone = target.parents[bone];
			if( resolvedForTarget[bone] >= 0 )
			{
				return resolvedForTarget[bone];
			}
		}
		return -1;
	};

	// Per mapped bone, a correction that turns the target's bind direction (towards its first mapped child, in
	// bone map order) onto the source's.  Bones without a mapped child keep their parent's correction, the topmost
	// mapped bones and bones mapped from the bind pose none.  Visiting in target order does the parent first.
	std::vector<Quaternion> correction( resolved.size(), IdentityQuaternion() );
	std::vector<uint32_t> order( resolved.size() );
	for( uint32_t i = 0; i < order.size(); ++i )
	{
		order[i] = i;
	}
	std::sort( order.begin(), order.end(), [&]( uint32_t a, uint32_t b ) { return resolved[a].target < resolved[b].target; } );

	uint32_t pelvis = NO_BONE;
	for( uint32_t i : order )
	{
		const int32_t ancestor = mappedAncestor( resolved[i].target );
		if( ancestor < 0 )
		{
			if( pelvis == NO_BONE )
			{
				pelvis = i;
				m_pelvisTarget = resolved[i].target;
				m_pelvisSourceBindHeight = sourceBindWorld[resolved[i].sourceBind].position.y;
				m_pelvisTargetBindPosition = targetBindWorld[resolved[i].targetBind].position;
				m_pelvisHeightScale = m_pelvisSourceBindHeight > 0.f ? m_pelvisTargetBindPosition.y / m_pelvisSourceBindHeight : 1.f;
			}
			continue;
		}

		correction[i] = correction[ancestor];
		if( resolved[i].fromBind )
		{
			correction[i] = IdentityQuaternion();
			continue;
		}
		for( uint32_t j = 0; j < resolved.size(); ++j )
		{
			if( mappedAncestor( resolved[j].target ) != (int32_t)i )
			{
				continue;
			}
			const Vector3 targetDirection = targetBindWorld[resolved[j].targetBind].position - targetBindWorld[resolved[i].targetBind].position;
			const Vector3 sourceDirection = sourceBindWorld[resolved[j].sourceBind].position - sourceBindWorld[resolved[i].sourceBind].position;
			if( LengthSq( targetDirection ) > 1e-10f && LengthSq( sourceDirection ) > 1e-10f )
			{
				correction[i] = ArcBetween( Normalize( targetDirection ), Normalize( sourceDirection ) );
			}
			break;
		}
	}

	if( pelvis == NO_BONE )
	{
		return false;
	}

	// Target bind rotation, then the correction, then back out of the source bind rotation
	m_linkForTargetBone.assign( target.bones.size(), -1 );
	for( uint32_t i = 0; i < resolved.size(); ++i )
	{
		const Quaternion targetBindRotation = targetBindWorld[resolved[i].targetBind].rotation;
		const Quaternion sourceBindRotation = sourceBindWorld[resolved[i].sourceBind].rotation;
		const Quaternion offset = XMQuaternionNormalize( XMQuaternionMultiply( XMQuaternionMultiply( targetBindRotation, correction[i] ), XMQuaternionConjugate( sourceBindRotation ) ) );
		m_linkForTargetBone[resolved[i].target] = (int32_t)m_links.size();
		m_links.push_back( { offset } );
	}

	// The feet: the mapped bones lowest in the target bind pose
	float lowest = FLT_MAX;
	for( const Resolved& r : resolved )
	{
		lowest = std::min( lowest, targetBindWorld[r.targetBind].position.y );
	}
	for( const Resolved& r : resolved )
	{
		if( targetBindWorld[r.targetBind].position.y < lowest + FOOT_BAND )
		{
			m_feet.push_back( r.target );
		}
	}

	for( uint32_t c = 0; c < clipCount; ++c )
	{
		Clip& clip = m_clips[c];
		clip.skeleton = &clipData[c]->skeletons[0];
		clip.player = std::make_unique<cmf::AnimationPlayer>( *clip.skeleton, clipData[c]->animations[0] );
		clip.duration = clipData[c]->animations[0].duration;
		clip.sourceBones.reserve( resolved.size() );
		for( const Resolved& r : resolved )
		{
			clip.sourceBones.push_back( r.source[c] );
		}
		clip.pelvisSource = resolved[pelvis].source[c];
	}
	m_clipCount = clipCount;
	m_boundTarget = &target;
	m_boundTargetBoneCount = target.bones.size();
	return true;
}

void Tr2AnimationRetargeter::UpdateClock()
{
	const float now = Tr2Renderer::GetAnimationTime();
	float duration = m_clips[0].duration;
	if( m_clipCount > 1 )
	{
		duration += ( m_clips[1].duration - duration ) * Clamp01( m_blend );
	}
	if( m_clockStarted && duration > 0.f )
	{
		m_phase += Tr2Renderer::GetAnimationTimeElapsed( m_lastTime ) * m_speed / duration;
	}
	m_lastTime = now;
	m_clockStarted = true;
	m_phase = Wrap01( m_phase );
}

void Tr2AnimationRetargeter::Retarget( uint32_t clipIndex, float phase, const cmf::Skeleton& skeleton, const cmf::SkeletonPose& base, cmf::SkeletonPose& out )
{
	Clip& clip = m_clips[clipIndex];
	const float offset = clipIndex == 0 ? m_sourcePhaseOffset : m_blendSourcePhaseOffset;

	// Source pose and its world transforms
	cmf::RestPose( clip.sourcePose, *clip.skeleton );
	clip.player->SampleAtLocalTime( clip.sourcePose, Wrap01( phase + offset ) * clip.duration );
	ComputeWorld( *clip.skeleton, clip.sourcePose.boneTransforms.data(), clip.sourceWorld );

	// Retargeted pose: unmapped bones keep what the animation sampled
	out.skeleton = base.skeleton;
	out.boneTransforms = base.boneTransforms;
	m_targetWorld.resize( skeleton.bones.size() );

	const cmf::Transform identity{ Vector3( 0.f, 0.f, 0.f ), IdentityQuaternion(), Vector3( 1.f, 1.f, 1.f ) };
	for( uint32_t i = 0; i < skeleton.bones.size(); ++i )
	{
		const cmf::Transform& parentWorld = HasParent( skeleton, i ) ? m_targetWorld[skeleton.parents[i]] : identity;
		cmf::Transform& local = out.boneTransforms[i];

		const int32_t link = m_linkForTargetBone[i];
		if( link >= 0 )
		{
			const cmf::Transform& source = clip.sourceWorld[clip.sourceBones[link]];
			const Quaternion world = XMQuaternionNormalize( XMQuaternionMultiply( m_links[link].offset, source.rotation ) );
			local.rotation = XMQuaternionNormalize( XMQuaternionMultiply( world, XMQuaternionConjugate( parentWorld.rotation ) ) );

			if( i == m_pelvisTarget )
			{
				// Only the bob: the clip plays in place and the caller moves the character
				Vector3 wanted = m_pelvisTargetBindPosition;
				wanted.y += ( source.position.y - m_pelvisSourceBindHeight ) * m_pelvisHeightScale + clip.groundOffset;
				local.position = cmf::TransformPoint( wanted, cmf::Inverse( parentWorld ) );
			}
		}

		m_targetWorld[i] = HasParent( skeleton, i ) ? cmf::Multiply( local, parentWorld ) : local;
	}
}

void Tr2AnimationRetargeter::CalibrateGround( const cmf::Skeleton& skeleton, const cmf::SkeletonPose& base )
{
	// Where the sampled pose (an idle) holds the feet is the ground; over each clip's cycle the feet come down to it
	std::vector<cmf::Transform> world;
	ComputeWorld( skeleton, base.boneTransforms.data(), world );
	float ground = FLT_MAX;
	for( uint32_t foot : m_feet )
	{
		ground = std::min( ground, world[foot].position.y );
	}

	for( uint32_t c = 0; c < m_clipCount; ++c )
	{
		m_clips[c].groundOffset = 0.f;
		float lowest = FLT_MAX;
		for( uint32_t k = 0; k < GROUND_SAMPLES; ++k )
		{
			Retarget( c, (float)k / GROUND_SAMPLES, skeleton, base, m_retargetPose );
			for( uint32_t foot : m_feet )
			{
				lowest = std::min( lowest, m_targetWorld[foot].position.y );
			}
		}
		if( !m_feet.empty() )
		{
			m_clips[c].groundOffset = ground - lowest;
		}
	}
	m_groundCalibrated = true;
}

void Tr2AnimationRetargeter::ModifyPose( const cmf::Skeleton& skeleton, cmf::SkeletonPose& pose )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	UpdateClock();

	const float weight = Clamp01( m_weight );
	if( weight <= 0.f )
	{
		return;
	}
	if( m_needsBind || m_boundTarget != &skeleton || m_boundTargetBoneCount != skeleton.bones.size() )
	{
		if( !Bind( skeleton ) )
		{
			return;
		}
	}
	if( m_clipCount == 0 || pose.boneTransforms.size() != skeleton.bones.size() )
	{
		return;
	}

	if( !m_groundCalibrated )
	{
		CalibrateGround( skeleton, pose );
	}

	Retarget( 0, m_phase, skeleton, pose, m_retargetPose );
	const float blend = Clamp01( m_blend );
	if( m_clipCount > 1 && blend > 0.f )
	{
		Retarget( 1, m_phase, skeleton, pose, m_blendPose );
		cmf::BlendPoses( m_retargetPose, m_retargetPose, m_blendPose, blend );
	}

	cmf::BlendPoses( pose, pose, m_retargetPose, weight );
}
