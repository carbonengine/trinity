// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2AnimationRetargeter.h"
#include "Tr2GrannyAnimation.h"
#include "Tr2Renderer.h"
#include "Resources/TriGrannyRes.h"

#include <TriMath.h>
#include <cmf/transforms.h>

#include <cfloat>
#include <cmath>

// Quaternions compose like the rest of the cmf code: XMQuaternionMultiply( a, b ) applies a, then b, and
// XMVector3Rotate applies a rotation to a vector.  A world rotation takes bone space to model space.

namespace
{
constexpr uint32_t GROUND_SAMPLES = 32;	// poses per cycle looked at to find where the feet come down
constexpr float FOOT_BAND = 0.05f;	// mapped bones this close to the lowest in the target bind pose are feet
constexpr float MIN_PELVIS_HEIGHT = 1e-3f;	// below this height above the feet, the bob is left unscaled

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

// NaN and infinity come out as 0: the clip sampling does no validation of the time it is given
float Wrap01( float x )
{
	if( !std::isfinite( x ) )
	{
		return 0.f;
	}
	x = fmodf( x, 1.f );
	return x < 0.f ? x + 1.f : x;
}

float Clamp01( float x )
{
	return x > 0.f ? ( x < 1.f ? x : 1.f ) : 0.f;	// NaN comes out as 0
}
}

Tr2AnimationRetargeter::Tr2AnimationRetargeter( IRoot* lockobj )
{
}

Tr2AnimationRetargeter::~Tr2AnimationRetargeter()
{
	SetAnimation( nullptr );

	// The players point into the clips' data, so they go before the files
	Unbind();

	for( Clip& clip : m_clips )
	{
		ReleaseRes( clip.res );
	}
	ReleaseRes( m_sourceBindPoseRes );
	ReleaseRes( m_targetBindPoseRes );
}

uint32_t Tr2AnimationRetargeter::FindBone( const cmf::Skeleton& skeleton, const std::string& name )
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

Tr2GrannyAnimation* Tr2AnimationRetargeter::GetAnimation() const
{
	return m_animation;
}

void Tr2AnimationRetargeter::SetAnimation( Tr2GrannyAnimation* animation )
{
	if( m_animation )
	{
		m_animation->RemoveNotifyTarget( this );
		if( m_animation->GetPoseModifier() == this )
		{
			m_animation->SetPoseModifier( nullptr );
		}
	}

	m_animation = animation;

	if( m_animation )
	{
		if( m_animation->GetPoseModifier() && m_animation->GetPoseModifier() != this )
		{
			CCP_LOGWARN( "Tr2AnimationRetargeter: replacing the animation's existing pose modifier" );
		}
		m_animation->SetPoseModifier( this );

		// Hear when the animation's skeleton is released or rebuilt, as Tr2AnimationMeshBinding does
		m_animation->AddNotifyTarget( this );
	}

	m_needsBind = true;
}

const std::string& Tr2AnimationRetargeter::GetSourcePath() const
{
	return m_clips[0].path;
}

void Tr2AnimationRetargeter::SetSourcePath( const std::string& path )
{
	m_clips[0].path = path;
	LoadRes( path, m_clips[0].res );
}

const std::string& Tr2AnimationRetargeter::GetBlendSourcePath() const
{
	return m_clips[1].path;
}

void Tr2AnimationRetargeter::SetBlendSourcePath( const std::string& path )
{
	m_clips[1].path = path;
	LoadRes( path, m_clips[1].res );
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

float Tr2AnimationRetargeter::GetSourcePhaseOffset() const
{
	return m_clips[0].phaseOffset;
}

void Tr2AnimationRetargeter::SetSourcePhaseOffset( float offset )
{
	m_clips[0].phaseOffset = offset;
}

float Tr2AnimationRetargeter::GetBlendSourcePhaseOffset() const
{
	return m_clips[1].phaseOffset;
}

void Tr2AnimationRetargeter::SetBlendSourcePhaseOffset( float offset )
{
	m_clips[1].phaseOffset = offset;
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
	return GetClipData( m_clips[0].res ) && ( m_clips[1].path.empty() || GetClipData( m_clips[1].res ) ) &&
		( m_sourceBindPosePath.empty() || GetData( m_sourceBindPoseRes ) ) && GetData( m_targetBindPoseRes );
}

void Tr2AnimationRetargeter::LoadRes( const std::string& path, TriGrannyResPtr& res )
{
	// Drop anything pointing into the old data first
	Unbind();
	m_needsBind = true;

	ReleaseRes( res );

	if( !path.empty() )
	{
		BeResMan->GetResource( path.c_str(), "raw", BlueInterfaceIID<TriGrannyRes>(), (void**)&res );
	}

	// A file can sit in more than one slot, a clip and its bind pose say; it is watched once
	if( res && CountSlotsHolding( res ) == 1 )
	{
		res->AddNotifyTarget( this );
	}
}

void Tr2AnimationRetargeter::ReleaseRes( TriGrannyResPtr& res )
{
	if( !res )
	{
		return;
	}
	// Stop watching the file once no other slot holds it
	if( CountSlotsHolding( res ) == 1 )
	{
		res->RemoveNotifyTarget( this );
	}
	res.Unlock();
}

uint32_t Tr2AnimationRetargeter::CountSlotsHolding( const TriGrannyRes* res ) const
{
	const TriGrannyRes* slots[] = { m_clips[0].res, m_clips[1].res, m_sourceBindPoseRes, m_targetBindPoseRes };
	return (uint32_t)std::count( std::begin( slots ), std::end( slots ), res );
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
	m_boundTargetBoneCount = target.bones.size();

	if( !IsReady() || m_boneMap.empty() )
	{
		// Try again next frame: a file is still loading, or no bone is mapped yet
		m_needsBind = true;
		return false;
	}
	// From here a failure is not retried until a file, the bone map or the animation's skeleton changes
	m_needsBind = false;

	const uint32_t clipCount = m_clips[1].path.empty() ? 1 : 2;
	const cmf::Data* clipData[2] = { GetClipData( m_clips[0].res ), clipCount > 1 ? GetClipData( m_clips[1].res ) : nullptr };
	const cmf::Data* sourceBindData = m_sourceBindPosePath.empty() ? clipData[0] : GetData( m_sourceBindPoseRes );
	const cmf::Data* targetBindData = GetData( m_targetBindPoseRes );

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
			CCP_LOGWARN( "Tr2AnimationRetargeter: %s is mapped more than once, keeping its first pair", mapping.target.c_str() );
			continue;
		}
		resolvedForTarget[r.target] = (int32_t)resolved.size();
		resolved.push_back( r );
	}
	if( resolved.empty() )
	{
		CCP_LOGWARN( "Tr2AnimationRetargeter: none of the %u bone pairs could be resolved, so nothing plays", (uint32_t)m_boneMap.size() );
		return false;
	}

	// The lowest mapped bone in each bind pose: the feet are found and the pelvis heights measured from it
	float sourceLowest = FLT_MAX;
	float targetLowest = FLT_MAX;
	for( const Resolved& r : resolved )
	{
		sourceLowest = std::min( sourceLowest, sourceBindWorld[r.sourceBind].position.y );
		targetLowest = std::min( targetLowest, targetBindWorld[r.targetBind].position.y );
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

				// Heights above the feet: a rig's origin can sit anywhere (Mixamo's is at the hips)
				const float sourceHeight = m_pelvisSourceBindHeight - sourceLowest;
				const float targetHeight = m_pelvisTargetBindPosition.y - targetLowest;
				m_pelvisHeightScale = sourceHeight > MIN_PELVIS_HEIGHT && targetHeight > MIN_PELVIS_HEIGHT ? targetHeight / sourceHeight : 1.f;
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
				TriQuaternionRotationArc( &correction[i], &targetDirection, &sourceDirection );
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
	for( const Resolved& r : resolved )
	{
		if( targetBindWorld[r.targetBind].position.y < targetLowest + FOOT_BAND )
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

	CCP_LOG(
		"Tr2AnimationRetargeter: bound %u of %u bone pairs and %u clips; pelvis %s, %u foot bones",
		(uint32_t)resolved.size(),
		(uint32_t)m_boneMap.size(),
		clipCount,
		cmf::ToStdString( target.bones[m_pelvisTarget] ).c_str(),
		(uint32_t)m_feet.size() );
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

void Tr2AnimationRetargeter::Retarget( Clip& clip, float phase, const cmf::Skeleton& skeleton, const cmf::SkeletonPose& base, cmf::SkeletonPose& out )
{
	// Source pose and its world transforms
	cmf::RestPose( clip.sourcePose, *clip.skeleton );
	clip.player->SampleAtLocalTime( clip.sourcePose, Wrap01( phase + clip.phaseOffset ) * clip.duration );
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
			Retarget( m_clips[c], (float)k / GROUND_SAMPLES, skeleton, base, m_retargetPose );
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
	TRINITY_STATS_ZONE( __FUNCTION__ );

	UpdateClock();

	const float weight = Clamp01( m_weight );
	if( weight <= 0.f )
	{
		return;
	}
	if( m_needsBind || m_boundTargetBoneCount != skeleton.bones.size() )
	{
		Bind( skeleton );
	}
	if( m_clipCount == 0 || pose.boneTransforms.size() != skeleton.bones.size() )
	{
		return;
	}

	if( !m_groundCalibrated )
	{
		CalibrateGround( skeleton, pose );
	}

	Retarget( m_clips[0], m_phase, skeleton, pose, m_retargetPose );
	const float blend = Clamp01( m_blend );
	if( m_clipCount > 1 && blend > 0.f )
	{
		Retarget( m_clips[1], m_phase, skeleton, pose, m_blendPose );
		cmf::BlendPoses( m_retargetPose, m_retargetPose, m_blendPose, blend );
	}

	cmf::BlendPoses( pose, pose, m_retargetPose, weight );
}
