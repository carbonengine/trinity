// Copyright © 2026 Fenris Creations ehf.

#include "StdAfx.h"
#include "EveChildTurret.h"
#include "Eve/Turret/EveTurretFiringFX.h"
#include "Tr2GrannyAnimation.h"
#include "Tr2MeshBase.h"
#include "TriMath.h"
#include "TriObserverLocal.h"

// invalids
constexpr unsigned int INVALID_BONE_INDEX = 0xffffffff;
constexpr unsigned int INVALID_TURRET_INDEX = 0xffffffff;

// some very static timings, no need to confuse artists by exposing them
constexpr float TRACKING_FADE_TIME = 1.f;

EveChildTurret::EveChildTurret( IRoot* lockobj ) :
	EveChildMesh( lockobj )
{
	for( unsigned int i = 0; i < SYSBONE_MAX; ++i )
	{
		m_systemBoneID[i] = INVALID_BONE_INDEX;
	}

	m_target.CreateInstance();
	m_target->SetFadeOnLocatorChange( true );

	PrepareResources();
}

EveChildTurret::~EveChildTurret()
{
	if( m_hookedUpdater && m_hookedUpdater->GetPoseModifier() == this )
	{
		m_hookedUpdater->SetPoseModifier( nullptr );
	}

	if( m_firingEffect )
	{
		m_firingEffect->CleanUp();
	}

	ReleaseCachedGeometryData();

	ReleaseResources( TRISTORAGE_ALL );
}
bool EveChildTurret::Initialize()
{
	// pass down some user-defined data into submodules we don't save out.
	m_target->SetImpactBehaviour( m_impactSize, m_impactBehaviour );

	// an inline-authored firingEffect wins over the res path at load time
	if( !m_firingEffect && !m_firingEffectResPath.empty() )
	{
		SetFiringEffect( BeResMan->LoadObject<EveTurretFiringFX>( m_firingEffectResPath.c_str() ).p );
	}

	return EveChildMesh::Initialize();
}
bool EveChildTurret::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_impactSize ) || IsMatch( value, m_impactBehaviour ) )
	{
		m_target->SetImpactBehaviour( m_impactSize, m_impactBehaviour );
	}
	if( IsMatch( value, m_firingEffectResPath ) && !m_firingEffectResPath.empty() )
	{
		SetFiringEffect( BeResMan->LoadObject<EveTurretFiringFX>( m_firingEffectResPath.c_str() ).p );
	}
	return EveChildMesh::OnModified( value );
}
void EveChildTurret::RegisterComponents()
{
	EveChildMesh::RegisterComponents();
	const auto registry = GetComponentRegistry();
	if( registry && m_display )
	{
		if( EveEntityPtr entity = BlueCastPtr( m_firingEffect ) )
		{
			entity->Register( registry );
		}
	}
}
void EveChildTurret::UnRegisterComponents()
{
	EveChildMesh::UnRegisterComponents();
	const auto registry = this->GetComponentRegistry();
	if( registry )
	{
		if( EveEntityPtr entity = BlueCastPtr( m_firingEffect ) )
		{
			entity->UnRegister( registry );
		}
	}
}
void EveChildTurret::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	EveChildMesh::GetDebugOptions( options );

	if( m_firingEffect )
	{
		m_firingEffect->GetDebugOptions( options );
	}

	if( m_turretMovementObserver )
	{
		m_turretMovementObserver->GetDebugOptions( options );
	}
}
void EveChildTurret::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	EveChildMesh::RenderDebugInfo( renderer );

	if( m_firingEffect )
	{
		m_firingEffect->RenderDebugInfo( renderer );
	}

	if( m_turretMovementObserver && m_playMovementSound )
	{
		m_turretMovementObserver->RenderDebugInfo( renderer );
	}
}

void EveChildTurret::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	float deltaT = updateContext.GetDeltaT();

	if( m_firingEffect )
	{
		m_firingEffect->SetDisplaySourceObject( IsVisible( updateContext ) );
	}

	UpdateCachedGeometryData();

	// setup and update attached firing effect
	if( m_firingEffect )
	{
		// if the attached firing effect is looping, then we must recheck if active turret is still the best,
		if( m_firingEffect->IsLooping() )
		{
			if( m_state == STATE_FIRING )
			{
				// don't do it every frame, cause this will result in popping
				m_recheckTimeLeft -= deltaT;
				if( m_recheckTimeLeft < 0.f )
				{
					Vector3 source = m_worldTransform.GetTranslation();
					Vector3 position = source;
					int closestLocator = m_target->FindClosestLocator( &source, &position );
					if( closestLocator >= 0 && closestLocator != m_target->GetLocator() )
					{
						// Set up the firing states correctly
						SetupFiringState();
					}
					// recheck every 2 seconds
					m_recheckTimeLeft = 2.f;
				}
			}
		}
		m_firingEffect->UpdateSynchronous( updateContext );
	}

	// update the target locator position
	Vector3 position = m_worldTransform.GetTranslation();
	if( m_firingEffect )
	{
		m_firingEffect->GetStartPosition( position );
	}

	m_target->Update( deltaT, &position );

	if( m_mesh && m_turretMovementObserver != nullptr )
	{
		m_turretMovementObserver->Update( m_worldTransform );
	}
	EveChildMesh::UpdateSyncronous( updateContext, params );
}

void EveChildTurret::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	float deltaT = updateContext.GetDeltaT();
	// handle fading of turret tracking
	if( m_trackingInfluenceDelta != 0.f )
	{
		m_trackingInfluence += m_trackingInfluenceDelta * deltaT;
		if( m_trackingInfluence > m_maxTrackingTime )
		{
			m_trackingInfluence = m_maxTrackingTime;
			m_trackingInfluenceDelta = 0.f;
		}
		else if( m_trackingInfluence < 0.f )
		{
			m_trackingInfluence = 0.f;
			m_trackingInfluenceDelta = 0.f;
		}
	}

	if( m_delayToFadeOutTracking > 0.f )
	{
		m_delayToFadeOutTracking -= deltaT;
		if( m_delayToFadeOutTracking <= 0.f )
		{
			m_delayToFadeOutTracking = 0.f;
			m_trackingInfluenceDelta = -1.f / TRACKING_FADE_TIME;
		}
	}

	if( m_delayToFadeInTracking > 0.f )
	{
		m_delayToFadeInTracking -= deltaT;
		if( m_delayToFadeInTracking <= 0.f )
		{
			m_delayToFadeInTracking = 0.f;
			m_trackingInfluenceDelta = 1.f / TRACKING_FADE_TIME;
		}
	}

	// Should handle all the mesh data and transforms
	EveChildMesh::UpdateAsyncronous( updateContext, params );

	// setup and update attached firing effect
	if( m_firingEffect )
	{
		if( m_mesh )
		{
			// update all muzzle points in the firing effect
			for( unsigned int i = 0; i < m_firingEffect->GetPerMuzzleEffectCount(); ++i )
			{
				// get world transform of this muzzle bone
				Matrix matrix = GetFiringBoneWorldTransform( i );
				// and set it to the muzzle
				m_firingEffect->SetMuzzleTransform( i, &matrix );
			}
			m_firingEffectMuzzlePosSet = true;
		}

		m_firingEffect->SetEndPosition( m_target->GetTargetPosition() );

		// time update (return value tells us if effect is ready to fire!)
		if( m_firingEffect->UpdateAsynchronous( updateContext ) )
		{
			// if we haven't initialised muzzle positions, do it now
			// this can happen, and if we don't do this all effects originate from
			// the turret root until turret geometry is loaded and muzzle positions
			// properly set
			if( !m_firingEffectMuzzlePosSet )
			{
				for( unsigned int i = 0; i < m_firingEffect->GetPerMuzzleEffectCount(); ++i )
				{
					m_firingEffect->SetMuzzleTransform( i, &m_worldTransform );
				}

				m_firingEffectMuzzlePosSet = true;
			}
			m_firingEffect->SetDisplayDestObject( m_target->ShowDestObject() );
		}
	}
}

void EveChildTurret::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{
	EveChildMesh::UpdateVisibility( updateContext, parentTransform, parentLod );

	if( m_display && m_firingEffect )
	{
		m_firingEffect->UpdateVisibility( updateContext );
	}
}

void EveChildTurret::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	EveChildMesh::GetRenderables( renderables );

	if( m_display && m_firingEffect )
	{
		m_firingEffect->GetRenderables( renderables );
	}
}

void EveChildTurret::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	EveChildMesh::RegisterWithQuadRenderer( quadRenderer );

	if( m_firingEffect )
	{
		m_firingEffect->RegisterWithQuadRenderer( quadRenderer );
	}
}

void EveChildTurret::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	EveChildMesh::AddQuadsToQuadRenderer( frustum, quadRenderer );

	if( m_display && m_firingEffect )
	{
		m_firingEffect->AddQuadsToQuadRenderer( frustum, quadRenderer );
	}
}

void EveChildTurret::UpdateCachedGeometryData()
{
	auto* geometryRes = GetGeometryRes();
	if( geometryRes == m_cachedGeometryRes )
	{
		return;
	}
	ReleaseCachedGeometryData();
	if( geometryRes && geometryRes->IsGood() )
	{
		BuildCachedGeometryData( *geometryRes );
		m_cachedGeometryRes = geometryRes;
	}
}
void EveChildTurret::BuildCachedGeometryData( TriGeometryRes& geometryRes )
{
	if( geometryRes.GetSkeletonCount() )
	{
		if( TriGeometryResSkeletonData* skeletonData = geometryRes.GetSkeletonData( 0 ) )
		{
			for( int i = 0; i < SYSBONE_MAX; ++i )
			{
				// in case we don't find system bone, ::FindJoint() returns 0xffffffff
				m_systemBoneID[i] = skeletonData->FindJoint( EveTurretAiming::GetSystemBoneName( i ) );
			}

			InitializeFiringEffect();
		}
	}

	InitializeAnimation();

	ForceIdleAnimation();
}

void EveChildTurret::ReleaseCachedGeometryData()
{
	m_cachedGeometryRes = nullptr;
	m_firingEffectMuzzlePosSet = false;
}

void EveChildTurret::EnterStateDeactive()
{
	switch( m_state )
	{
	case STATE_DEACTIVE:
		// do nothing if we are already in this state
		break;
	case STATE_IDLE:
	case STATE_RELOADING:
		// no fadeout of tracking, just play deactive anim and then the deactive loop
		m_trackingInfluence = 0.f;
		PlayAnimation( "Pack", "Inactive" );
		m_delayToFadeOutTracking = 0.f;
		break;
	case STATE_FIRING:
		// stop shooting
		if( m_firingEffect )
		{
			m_firingEffect->StopFiring();
		}
		// DON'T break, just continue with stopping things:
	case STATE_TARGETING:
		// fadeout the tracking, play deactive anim and then the deactive loop
		m_delayToFadeOutTracking = 0.0001f;
		m_target->StopFireAtLocator();

		PlayAnimation( "Pack", "Inactive", TRACKING_FADE_TIME );
		break;

	default:
		break;
	}
	m_state = STATE_DEACTIVE;
}

void EveChildTurret::EnterStateIdle()
{
	if( !m_isOnline )
	{
		return;
	}

	switch( m_state )
	{
	case STATE_INVALID:
	case STATE_RELOADING:
		// just play active loop
		PlayAnimation( "", "Active" );
		break;
	case STATE_DEACTIVE:
		// start unpack animation, disable tracking and then into active loop
		PlayAnimation( "Deploy", "Active" );
		m_trackingInfluence = 0.f;
		break;
	case STATE_IDLE:
		// do nothing here
		break;
	case STATE_TARGETING:
	case STATE_FIRING:
		// stop shooting, fadeout tracking, then into active loop
		m_delayToFadeOutTracking = 0.0001f;
		m_target->StopFireAtLocator();
		if( m_firingEffect )
		{
			m_firingEffect->StopFiring();
		}
		PlayAnimation( "", "Active", TRACKING_FADE_TIME );

		if( m_playMovementSound && !m_targetingToIdleMovementAudioEvent.empty() )
		{
			SendEventToAudEmitter( m_turretMovementObserver, m_targetingToIdleMovementAudioEvent );
		}
		break;
	}
	m_state = STATE_IDLE;
}

void EveChildTurret::EnterStateTargeting()
{
	float animLength = 0.f;
	if( !m_isOnline )
	{
		return;
	}

	// what state are we in?
	switch( m_state )
	{
	case STATE_DEACTIVE:
		// play deploy anim, then active loop and fade in tracking
		animLength = PlayAnimation( "Deploy", "Active", TRACKING_FADE_TIME );
		// fade in tracking
		m_delayToFadeInTracking = animLength + 0.0001f;
		break;
	case STATE_IDLE:
	case STATE_RELOADING:
		// fadein tracking, play active loop
		m_delayToFadeInTracking = 0.0001f;
		PlayAnimation( "", "Active", TRACKING_FADE_TIME );
		break;
	case STATE_TARGETING:
		break;
	case STATE_FIRING:
		// stop shooting, then into active loop
		m_target->StopFireAtLocator();
		if( m_firingEffect )
		{
			m_firingEffect->StopFiring();
		}
		PlayAnimation( "", "Active", 0.f );
		break;

	default:
		break;
	}
	m_state = STATE_TARGETING;
}

void EveChildTurret::EnterStateFiring()
{
	if( !SetupFiringState() )
	{
		return;
	}

	// only if we are in firing mode, call ::StopFiring() on the effect right before
	// we call ::PrepareFiring(), it'll clean things up in the effect
	if( m_firingEffect && m_state == STATE_FIRING )
	{
		if( m_firingEffect->IsLooping() )
		{
			// We don't want to start and stop the curves when the turret is looping and firing
			m_firingEffect->PrepareFiringEffectMoveObjects();
			return;
		}
		m_firingEffect->StopFiring();
	}

	if( m_firingEffect )
	{
		if( m_maxCyclingFirePos > 1 )
		{
			m_firingEffect->PrepareFiring( 0.f, m_currentCyclingFiresPos, m_cyclingFireGroupCount );
		}
		else
		{
			m_firingEffect->PrepareFiring( 0.f );
		}

		if( m_target != nullptr )
		{
			m_firingEffect->SetImpactConfiguration( m_target->GetImpactConfiguration() );
		}
	}

	// finally, we can set state
	m_state = STATE_FIRING;
}

bool EveChildTurret::SetupFiringState()
{
	if( m_state == STATE_DEACTIVE )
	{
		// this state change is forbidden!
		CCP_LOGERR( "EveChildTurret %s wants to fire but is in deactive state.", m_name.c_str() );
		return false;
	}
	int closestLocator = -1;
	{
		Vector3 source = m_worldTransform.GetTranslation();
		Vector3 position = source;
		closestLocator = m_target->FindClosestLocator( &source, &position );
	}

	// if this turret is set to cycle through the muzzles for firing, do it here
	if( m_maxCyclingFirePos > 1 )
	{
		m_currentCyclingFiresPos += m_cyclingFireGroupCount;
		if( m_currentCyclingFiresPos >= m_maxCyclingFirePos * m_cyclingFireGroupCount )
		{
			m_currentCyclingFiresPos = 0;
		}
	}

	// timing: is the length of the firing effect known?
	float effectTotalTime = m_firingEffect ? m_firingEffect->GetFiringDuration() : 0.f;
	float effectPeakTime = m_firingEffect ? m_firingEffect->GetFiringPeakTime() : 0.f;

	Vector3 source = m_parentData.transform.GetTranslation();

	// what state are we in?
	switch( m_state )
	{
	case STATE_IDLE:
	case STATE_RELOADING:
		// fadein tracking, play fire anim (only one the firing turret!) and then the active anim
		m_delayToFadeInTracking = 0.0001f;

		PlayAnimation( GetFireAnimationName(), "Active", m_maxTrackingTime );
		// assign locator and turret
		m_target->StartFireAtLocator( closestLocator, m_maxTrackingTime + effectPeakTime, effectTotalTime - effectPeakTime, &source );
		break;
	case STATE_FIRING:
	case STATE_TARGETING:
		PlayAnimation( GetFireAnimationName(), "Active", m_maxTrackingTime );
		m_target->StartFireAtLocator( closestLocator, m_maxTrackingTime + effectPeakTime, effectTotalTime - effectPeakTime, &source );
		break;

	default:
		break;
	}

	return true;
}

void EveChildTurret::EnterStateReloading()
{
	// what state are we in?
	switch( m_state )
	{
	case STATE_DEACTIVE:
		// ignore this state change: when the turret is inactive, no reload state can be shown!
		break;
	case STATE_INVALID:
	case STATE_IDLE:
	case STATE_RELOADING:
		// just play reloading anim and then loop
		PlayAnimation( "Reload", "Active", 0.f );
		break;
	case STATE_TARGETING:
	case STATE_FIRING:
		// stop shooting, fadeout tracking, then into active loop
		m_delayToFadeOutTracking = 0.0001f;
		m_target->StopFireAtLocator();
		if( m_firingEffect )
		{
			m_firingEffect->StopFiring();
		}

		PlayAnimation( "Reload", "Active", TRACKING_FADE_TIME );
		break;

	default:
		break;
	}
	m_state = STATE_RELOADING;
}

void EveChildTurret::ForceStateDeactive()
{
	// turn it all off
	m_trackingInfluence = 0.f;
	m_delayToFadeOutTracking = 0.f;
	m_target->StopFireAtLocator();
	if( m_firingEffect )
	{
		m_firingEffect->StopFiring();
	}
	// finally, we can set state
	m_state = STATE_DEACTIVE;

	// now force-play the deactive anim for this state
	ForceIdleAnimation();
}

void EveChildTurret::ForceIdleAnimation()
{
	std::string idleAnimName = "";
	// what state?
	switch( m_state )
	{
	case STATE_DEACTIVE:
		idleAnimName = "Inactive";
		break;
	case STATE_IDLE:
	case STATE_RELOADING:
	case STATE_TARGETING:
	case STATE_FIRING:
		idleAnimName = "Active";
		break;

	default:
		break;
	}

	if( idleAnimName.length() > 0 )
	{
		PlayAnimation( "", idleAnimName, 0.f );
	}
}

void EveChildTurret::ForceStateTargeting()
{
	m_trackingInfluence = m_maxTrackingTime;
	m_trackingInfluenceDelta = 0.f;

	m_state = STATE_TARGETING;

	// now force-play the deactive anim for this state
	PlayAnimation( "", "Active", 0.f );
}

Matrix EveChildTurret::GetFiringBoneWorldTransform( unsigned int muzzle ) const
{
	if( !m_mesh )
	{
		return m_worldTransform;
	}

	Matrix matrix = m_worldTransform;

	// get the boneID for that muzzle from firing effect
	if( !m_firingEffect )
	{
		return matrix;
	}
	unsigned int boneID = m_firingEffect->GetPerMuzzleBoneID( muzzle );
	return GetTurretBoneTransform( boneID );
}

void EveChildTurret::InitializeFiringEffect()
{
	m_firingEffectMuzzlePosSet = false;
	if( !m_firingEffect )
	{
		return;
	}
	m_firingEffect->RegisterWithQuadRenderer( *Tr2QuadRenderer::Instance() );

	auto geometryResource = GetGeometryRes();
	if( geometryResource && geometryResource->GetSkeletonCount() )
	{
		if( TriGeometryResSkeletonData* skeletonData = geometryResource->GetSkeletonData( 0 ) )
		{
			const auto muzzleCount = m_firingEffect->GetPerMuzzleEffectCount();
			if( muzzleCount > EveTurretFiringFX::MUZZLECOUNT_MAX )
			{
				CCP_LOGERR( "Upper limit of firing bones is %d, this turret has %d", EveTurretFiringFX::MUZZLECOUNT_MAX, muzzleCount );
			}

			const unsigned int boneCount = std::min( muzzleCount, static_cast<unsigned int>( EveTurretFiringFX::MUZZLECOUNT_MAX ) );
			// firing bones should always be on the format Pos_FireXX where XX can range form 01 to 99
			for( unsigned int i = 0; i < boneCount; ++i )
			{
				char boneNameBuffer[32];
				unsigned int boneNameIndex = i + 1;
				sprintf_s( boneNameBuffer, "%s%.2d", m_firingEffect->GetFiringBoneName(), boneNameIndex );

				// in case we don't find positional bone, ::FindJoint() returns 0xffffffff
				m_firingEffect->SetMuzzleBoneID( i, skeletonData->FindJoint( boneNameBuffer ) );
			}
		}
	}
}

void EveChildTurret::InitializeAnimation()
{
	if( !m_animationUpdater )
	{
		m_animationUpdater.CreateInstance();
	}
	EveChildMesh::InitializeAnimation();

	if( m_hookedUpdater != m_animationUpdater )
	{
		if( m_hookedUpdater && m_hookedUpdater->GetPoseModifier() == this )
		{
			m_hookedUpdater->SetPoseModifier( nullptr );
		}
		m_animationUpdater->SetPoseModifier( this );
		m_hookedUpdater = m_animationUpdater;
	}
}

void EveChildTurret::ModifyPose( const cmf::Skeleton& skeleton, cmf::SkeletonPose& pose )
{
	if( m_trackingInfluence == 0.f )
	{
		return;
	}

	Vector3 targetPosOS = TransformCoord( *m_target->GetTrackingPosition(), Inverse( m_worldTransform ) );

	for( unsigned int bone = 0; bone < SYSBONE_MAX; ++bone )
	{
		// covers Invalid since INVALID_BONE_INDEX is max
		if( m_systemBoneID[bone] < pose.boneTransforms.size() )
		{
			cmf::Transform& boneTransform = pose.boneTransforms[m_systemBoneID[bone]];
			m_aiming.ModifySystemBoneTransform(
				static_cast<SystemBones>( bone ),
				&targetPosOS,
				nullptr,
				m_trackingInfluence,
				boneTransform.position,
				boneTransform.rotation );
		}
	}
}

Matrix EveChildTurret::GetTurretBoneTransform( uint32_t boneID ) const
{
	Matrix matrix = m_worldTransform;
	if( m_animationUpdater )
	{
		const auto& worldTransforms = m_animationUpdater->GetWorldTransforms();
		// covers Invalid since INVALID_BONE_INDEX is max_float
		if( boneID < worldTransforms.size() )
		{
			return worldTransforms[boneID] * matrix;
		}
	}
	return matrix;
}

TriGeometryRes* EveChildTurret::GetGeometryRes() const
{
	return m_mesh ? m_mesh->GetGeometryResource() : nullptr;
}

float EveChildTurret::PlayAnimation( const std::string& animName, const std::string& animNameIdle, float delay )
{
	if( !m_animationUpdater )
	{
		return 0.f;
	}

	m_animationUpdater->StopAnimations( delay );

	float animLength = 0.f;
	if( !animName.empty() )
	{
		if( m_animationUpdater->PlayAnimation( animName.c_str(), false, 1, 0.f, 1.f, false ) )
		{
			animLength = m_animationUpdater->FindAnimationDurationByName( animName.c_str() );
		}
	}
	if( !animNameIdle.empty() )
	{
		m_animationUpdater->PlayAnimation( animNameIdle.c_str(), false, 0, 0.f, 1.f, false );
	}

	return animLength;
}

std::string EveChildTurret::GetFireAnimationName() const
{
	// if m_currentCyclingFiresPos is 0, it's just "Fire"
	std::string res = "Fire";
	if( m_currentCyclingFiresPos > 0 )
	{
		res.push_back( '0' );
		res.push_back( '0' + m_currentCyclingFiresPos / m_cyclingFireGroupCount );
	}

	return res;
}

EveTurretFiringFX* EveChildTurret::GetFiringEffect()
{
	return m_firingEffect;
}

void EveChildTurret::SetFiringEffect( EveTurretFiringFX* firingEffect )
{
	auto registry = GetComponentRegistry();
	if( EveEntityPtr entity = BlueCastPtr( m_firingEffect ) )
	{
		entity->UnRegister( registry );
	}
	m_firingEffect = firingEffect;
	if( EveEntityPtr entity = BlueCastPtr( m_firingEffect ) )
	{
		entity->Register( registry );
	}
	InitializeFiringEffect();
}

void EveChildTurret::SetTargetObject( IRoot* target )
{
	if( !target )
	{
		return;
	}
	ITriTargetablePtr oldTargetPtr = m_target->GetTargetable();

	// attach to target
	m_target->SetTargetable( target );

	if( m_playMovementSound && !m_idleToTargetingMovementAudioEvent.empty() )
	{
		// Always trigger movement sounds if coming from IDLE state, otherwise trigger it only if you're targeting a new object.
		if( m_state == STATE_IDLE || !oldTargetPtr.IsEqualObject( m_target->GetTargetable() ) )
		{
			SendEventToAudEmitter( m_turretMovementObserver, m_idleToTargetingMovementAudioEvent );
		}
	}

	// update the firing effect we have one
	SetTargetScale();
}

ITriTargetablePtr EveChildTurret::GetTargetObject()
{
	return m_target->GetTargetable();
}

void EveChildTurret::SetTargetScale()
{
	if( m_firingEffect )
	{
		float radius = m_target->GetRadius();
		m_firingEffect->SetScaleByRadius( radius );
	}
}
