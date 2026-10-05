// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2AnimationRetargeter.h"
#include "Tr2GrannyAnimation.h"
#include "Tr2Renderer.h"
#include "Resources/TriGrannyRes.h"

#include <cmf/transforms.h>

// Quaternions compose like the rest of the cmf code: XMQuaternionMultiply( a, b ) applies a, then b, and
// XMVector3Rotate applies a rotation to a vector.  A world rotation takes bone space to model space.

namespace
{
constexpr uint32_t NO_BONE = 0xFFFFFFFF;

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
}

Tr2AnimationRetargeter::Tr2AnimationRetargeter( IRoot* lockobj ) :
	m_weight( 1.f ),
	m_speed( 1.f ),
	m_phase( 0.f ),
	m_lastTime( 0.f ),
	m_clockStarted( false ),
	m_needsBind( true ),
	m_boundTarget( nullptr ),
	m_boundTargetBoneCount( 0 ),
	m_clipSkeleton( nullptr ),
	m_duration( 0.f ),
	m_pelvisTarget( NO_BONE ),
	m_pelvisSource( NO_BONE ),
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

	// The player points into the clip's data, so it goes before the resources
	Unbind();

	auto release = [this]( TriGrannyResPtr& res ) {
		if( res )
		{
			res->RemoveNotifyTarget( this );
		}
	};
	release( m_sourceRes );
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
	m_boneMap.emplace_back( sourceBone, targetBone );
	m_needsBind = true;
}

void Tr2AnimationRetargeter::ClearBoneMap()
{
	m_boneMap.clear();
	m_needsBind = true;
}

bool Tr2AnimationRetargeter::IsReady() const
{
	return GetData( m_sourceRes ) && GetData( m_sourceBindPoseRes ) && GetData( m_targetBindPoseRes );
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
	m_player.reset();
	m_clipSkeleton = nullptr;
	m_boundTarget = nullptr;
	m_boundTargetBoneCount = 0;
	m_duration = 0.f;
	m_links.clear();
	m_linkForTargetBone.clear();
	m_pelvisTarget = NO_BONE;
	m_pelvisSource = NO_BONE;
}

bool Tr2AnimationRetargeter::Bind( const cmf::Skeleton& target )
{
	Unbind();
	m_needsBind = false;

	const auto* clip = GetData( m_sourceRes );
	const auto* sourceBindData = GetData( m_sourceBindPoseRes );
	const auto* targetBindData = GetData( m_targetBindPoseRes );
	if( !clip || !sourceBindData || !targetBindData || clip->animations.empty() || m_boneMap.empty() )
	{
		// Try again once the missing piece arrives (RebuildCachedData, MapBone)
		return false;
	}

	const cmf::Skeleton& clipSkeleton = clip->skeletons[0];
	const cmf::Skeleton& sourceBind = sourceBindData->skeletons[0];
	const cmf::Skeleton& targetBind = targetBindData->skeletons[0];

	std::vector<cmf::Transform> sourceBindWorld;
	std::vector<cmf::Transform> targetBindWorld;
	ComputeWorld( sourceBind, sourceBind.restTransforms.data(), sourceBindWorld );
	ComputeWorld( targetBind, targetBind.restTransforms.data(), targetBindWorld );

	// Resolve every pair in each of the four skeletons; bone order differs between files
	struct Resolved
	{
		uint32_t target, source, sourceBind, targetBind;
	};
	std::vector<Resolved> resolved;
	std::vector<int32_t> resolvedForTarget( target.bones.size(), -1 );
	for( const auto& [sourceName, targetName] : m_boneMap )
	{
		const Resolved r{ FindBone( target, targetName ), FindBone( clipSkeleton, sourceName ), FindBone( sourceBind, sourceName ), FindBone( targetBind, targetName ) };
		if( r.target == NO_BONE || r.source == NO_BONE || r.sourceBind == NO_BONE || r.targetBind == NO_BONE )
		{
			CCP_LOGWARN( "Tr2AnimationRetargeter: skipping %s -> %s, a bone is missing from one of the skeletons", sourceName.c_str(), targetName.c_str() );
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
	// bone map order) onto the source's.  Bones without a mapped child keep their parent's correction, and the
	// topmost mapped bones none.  Visiting in target order guarantees the parent is done first.
	std::vector<Quaternion> correction( resolved.size(), IdentityQuaternion() );
	std::vector<uint32_t> order( resolved.size() );
	for( uint32_t i = 0; i < order.size(); ++i )
	{
		order[i] = i;
	}
	std::sort( order.begin(), order.end(), [&]( uint32_t a, uint32_t b ) { return resolved[a].target < resolved[b].target; } );

	for( uint32_t i : order )
	{
		const int32_t ancestor = mappedAncestor( resolved[i].target );
		if( ancestor < 0 )
		{
			if( m_pelvisTarget == NO_BONE )
			{
				m_pelvisTarget = resolved[i].target;
				m_pelvisSource = resolved[i].source;
				m_pelvisSourceBindHeight = sourceBindWorld[resolved[i].sourceBind].position.y;
				m_pelvisTargetBindPosition = targetBindWorld[resolved[i].targetBind].position;
				m_pelvisHeightScale = m_pelvisSourceBindHeight > 0.f ? m_pelvisTargetBindPosition.y / m_pelvisSourceBindHeight : 1.f;
			}
			continue;
		}

		correction[i] = correction[ancestor];
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

	// Target bind rotation, then the correction, then back out of the source bind rotation
	m_linkForTargetBone.assign( target.bones.size(), -1 );
	for( uint32_t i = 0; i < resolved.size(); ++i )
	{
		const Quaternion targetBindRotation = targetBindWorld[resolved[i].targetBind].rotation;
		const Quaternion sourceBindRotation = sourceBindWorld[resolved[i].sourceBind].rotation;
		const Quaternion offset = XMQuaternionNormalize( XMQuaternionMultiply( XMQuaternionMultiply( targetBindRotation, correction[i] ), XMQuaternionConjugate( sourceBindRotation ) ) );
		m_linkForTargetBone[resolved[i].target] = (int32_t)m_links.size();
		m_links.push_back( { resolved[i].source, offset } );
	}

	m_clipSkeleton = &clipSkeleton;
	m_player = std::make_unique<cmf::AnimationPlayer>( clipSkeleton, clip->animations[0] );
	m_duration = clip->animations[0].duration;
	m_boundTarget = &target;
	m_boundTargetBoneCount = target.bones.size();
	return true;
}

void Tr2AnimationRetargeter::UpdateClock()
{
	const float now = Tr2Renderer::GetAnimationTime();
	if( m_clockStarted )
	{
		m_phase += Tr2Renderer::GetAnimationTimeElapsed( m_lastTime ) * m_speed;
	}
	m_lastTime = now;
	m_clockStarted = true;

	if( m_duration > 0.f )
	{
		m_phase = fmodf( m_phase, m_duration );
		if( m_phase < 0.f )
		{
			m_phase += m_duration;
		}
	}
}

void Tr2AnimationRetargeter::ModifyPose( const cmf::Skeleton& skeleton, cmf::SkeletonPose& pose )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	UpdateClock();

	if( m_weight <= 0.f )
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
	if( !m_player || m_duration <= 0.f || pose.boneTransforms.size() != skeleton.bones.size() )
	{
		return;
	}

	// Source pose and its world transforms
	cmf::RestPose( m_sourcePose, *m_clipSkeleton );
	m_player->SampleAtLocalTime( m_sourcePose, m_phase );
	ComputeWorld( *m_clipSkeleton, m_sourcePose.boneTransforms.data(), m_sourceWorld );

	// Retargeted pose: unmapped bones keep what the animation sampled
	m_retargetPose.skeleton = pose.skeleton;
	m_retargetPose.boneTransforms = pose.boneTransforms;
	m_targetWorld.resize( skeleton.bones.size() );

	const cmf::Transform identity{ Vector3( 0.f, 0.f, 0.f ), IdentityQuaternion(), Vector3( 1.f, 1.f, 1.f ) };
	for( uint32_t i = 0; i < skeleton.bones.size(); ++i )
	{
		const cmf::Transform& parentWorld = HasParent( skeleton, i ) ? m_targetWorld[skeleton.parents[i]] : identity;
		cmf::Transform& local = m_retargetPose.boneTransforms[i];

		const int32_t link = m_linkForTargetBone[i];
		if( link >= 0 )
		{
			const BoneLink& boneLink = m_links[link];
			const Quaternion world = XMQuaternionNormalize( XMQuaternionMultiply( boneLink.offset, m_sourceWorld[boneLink.source].rotation ) );
			local.rotation = XMQuaternionNormalize( XMQuaternionMultiply( world, XMQuaternionConjugate( parentWorld.rotation ) ) );

			if( i == m_pelvisTarget )
			{
				// Only the bob: the clip plays in place and the caller moves the character
				Vector3 wanted = m_pelvisTargetBindPosition;
				wanted.y += ( m_sourceWorld[m_pelvisSource].position.y - m_pelvisSourceBindHeight ) * m_pelvisHeightScale;
				local.position = cmf::TransformPoint( wanted, cmf::Inverse( parentWorld ) );
			}
		}

		m_targetWorld[i] = HasParent( skeleton, i ) ? cmf::Multiply( local, parentWorld ) : local;
	}

	cmf::BlendPoses( pose, pose, m_retargetPose, std::min( m_weight, 1.f ) );
}
