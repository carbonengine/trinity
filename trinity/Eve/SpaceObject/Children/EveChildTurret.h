// Copyright © 2026 Fenris Creations ehf.

#pragma once
#ifndef EveChildTurret_H
#define EveChildTurret_H

#include "EveChildMesh.h"
#include "Eve/Turret/EveTurretAiming.h"
#include "Eve/Turret/EveTurretTarget.h"
#include "Include/ITr2PoseModifier.h"

BLUE_DECLARE( EveTurretFiringFX );
BLUE_DECLARE( EveTurretTarget );

BLUE_CLASS( EveChildTurret ) :
	public EveChildMesh,
	public ITr2PoseModifier
{
public:
	EXPOSE_TO_BLUE();

	EveChildTurret( IRoot* lockobj = nullptr );
	~EveChildTurret();

	bool Initialize() override;
	bool OnModified( Be::Var * value ) override;
	void RegisterComponents() override;
	void UnRegisterComponents() override;

	void GetDebugOptions( Tr2DebugRendererOptions & options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer ) override;

	// EveSpaceObjectChild
	void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod ) override;
	void GetRenderables( std::vector<ITr2Renderable*> & renderables ) override;
	void RegisterWithQuadRenderer( Tr2QuadRenderer & quadRenderer ) override;
	void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const override;

	void UpdateCachedGeometryData();
	void BuildCachedGeometryData( TriGeometryRes & geometryRes );
	void ReleaseCachedGeometryData();

	// action
	void EnterStateDeactive();
	void EnterStateIdle();
	void EnterStateTargeting();
	void EnterStateFiring();
	bool SetupFiringState();
	void EnterStateReloading();

	void ForceStateDeactive();
	void ForceIdleAnimation();
	void ForceStateTargeting();

	Matrix GetFiringBoneWorldTransform( unsigned int muzzle ) const;

	// turret set states
	enum State
	{
		STATE_INVALID = 0,
		STATE_DEACTIVE,
		STATE_IDLE,
		STATE_TARGETING,
		STATE_FIRING,
		STATE_RELOADING,
	};

	void ModifyPose( const cmf::Skeleton& skeleton, cmf::SkeletonPose& pose ) override;

protected:
	// setup the attached firing effect
	void InitializeFiringEffect();

	void InitializeAnimation() override;

	Matrix GetTurretBoneTransform( uint32_t boneID ) const;

	TriGeometryRes* GetGeometryRes() const;

	// animation
	float PlayAnimation( const std::string& animName, const std::string& animNameIdle, float delay = 0.f );
	std::string GetFireAnimationName() const;

	EveTurretFiringFX* GetFiringEffect();
	void SetFiringEffect( EveTurretFiringFX * firingEffect );

	// Assign the target object
	void SetTargetObject( IRoot * target );
	ITriTargetablePtr GetTargetObject();
	void SetTargetScale();

	// target (object we are tracking)
	EveTurretTargetPtr m_target;

	bool m_isOnline = true;

	// impacts
	float m_impactSize = 0.f;
	ImpactBehaviour::Type m_impactBehaviour = ImpactBehaviour::DAMAGE_LOCATOR;

	// tracking
	float m_trackingInfluence = 0.f;
	float m_trackingInfluenceDelta = 0.f;
	float m_delayToFadeOutTracking = 0.f;
	float m_delayToFadeInTracking = 0.f;
	float m_maxTrackingTime = 1.f;

	// animation: updater we last hooked our pose modifier into (to unhook on swap)
	Tr2GrannyAnimationPtr m_hookedUpdater;

	uint32_t m_maxCyclingFirePos = 1;
	uint32_t m_cyclingFireGroupCount = 1;
	uint32_t m_currentCyclingFiresPos = 0;

	// system bones
	unsigned int m_systemBoneID[EveTurretAiming::SYSBONE_MAX];
	// sysbone aiming math + tuning (shared with EveTurretSet)
	EveTurretAiming m_aiming;

	// state of turret set
	State m_state = STATE_IDLE;

	float m_recheckTimeLeft = -1.f;

	// firing effect redfile path
	std::string m_firingEffectResPath;

	// firing effect
	EveTurretFiringFXPtr m_firingEffect;
	bool m_firingEffectMuzzlePosSet = false;

	// Audio specific attributes
	bool m_playMovementSound = true;
	TriObserverLocalPtr m_turretMovementObserver;
	std::wstring m_idleToTargetingMovementAudioEvent;
	std::wstring m_targetingToIdleMovementAudioEvent;

	TriGeometryResPtr m_cachedGeometryRes;
};

TYPEDEF_BLUECLASS( EveChildTurret );

#endif
