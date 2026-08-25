// Copyright © 2015 CCP ehf.

#pragma once
#ifndef EveImpactOverlay_H
#define EveImpactOverlay_H

//#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "ITr2Renderable.h"
#include "include/ITriTargetable.h"
#include "Resources/Tr2LodResource.h"
#include "EveDamageOverlay.h"

class EveUpdateContext;

BLUE_DECLARE( TriPerlinCurve );
BLUE_DECLARE( Tr2ScalarFader );
BLUE_DECLARE( TriFrustum );
BLUE_DECLARE( Tr2MeshBase );
BLUE_DECLARE( EveSpaceObject2 );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2GpuUniqueEmitter );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );


BLUE_CLASS( EveImpactOverlay ) :
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	EveImpactOverlay( IRoot* lockobj = NULL );
	~EveImpactOverlay();

	// shield impacts
	struct ShieldImpactData
	{
		int damageLocatorIndex;
		Vector3 interceptPosition;
		Vector3 direction;
		float lifeTime;
		float timeLeft;
		float size;
		float intensity;
	};

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// Updates
	void UpdateSyncronous( const EveUpdateContext& updateContext, EveSpaceObject2* parent );
	void UpdateAsyncronous( const EveUpdateContext& updateContext, EveSpaceObject2* parent );

	/////////////////////////////////////////////////////////////////////////////////////
	// Rendering
	void GetBatches( ITriRenderBatchAccumulator * accumulator, TriBatchType batchType, const Tr2PerObjectData* perObjectData, float screenSize );
	Tr2Effect* GetArmorDamageShader( TriBatchType batchType ) const;

	// setup
	void Set( TriPerlinCurvePtr hullDamageFlickerCurve, Tr2GpuUniqueEmitterPtr armorDamageEmitter, Tr2GpuUniqueEmitterPtr hullImpactEmitter, Tr2EffectPtr armorDamageShader, Tr2MeshBase * shieldImpactMesh, bool shieldIsEllipsoid );

	// getters
	int32_t GetDataTextureOffset() const;
	ITriTargetable::ImpactConfiguration GetImpactConfiguration() const;
	bool HasShieldEllipsoid() const;
	float GetActivationStrength( const EveUpdateContext& updateContext ) const;
	float GetArmorImpactLifeTime() const;
	Vector3 GetLastDamageState() const;
	EveDamageOverlayPtr GetDamageOverlay() const;

	// legacy blue property forwarding into the damage overlay
	unsigned int GetSeed() const;
	int GetImpactDataNextIdx() const;
	size_t GetArmorImpactGoalCount() const;
	float GetArmorImpactParentSize() const;
	bool GetDebugForceSpawnDebris() const;
	void SetDebugForceSpawnDebris( bool value );
	float GetRenderPriority() const;
	int32_t GetDataTextureBlockID() const;
	float GetHullDamageFactor() const;
	void SetHullDamageFactor( float factor );
	Tr2Effect* GetArmorDamageShaderEffect() const;
	void SetArmorDamageShaderEffect( Tr2Effect * shader );
	TriPerlinCurve* GetHullDamageFlickerCurve() const;
	void SetHullDamageFlickerCurve( TriPerlinCurve * curve );
	Tr2ScalarFader* GetArmorRepairing() const;
	void SetArmorRepairing( Tr2ScalarFader * fader );
	Tr2ScalarFader* GetArmorHardening() const;
	void SetArmorHardening( Tr2ScalarFader * fader );
	Tr2ScalarFader* GetHullRepairing() const;
	void SetHullRepairing( Tr2ScalarFader * fader );

	// setters
	void SetSeed( const unsigned int seed );
	void SetDamageLocatorCount( unsigned int count );

	// control animation
	void ToggleEffect( const std::string& name, bool on, float duration );

	// set the damages
	void SetDamageState( float shield, float armor, float hull, bool doCreateArmorImpacts );
	void Clear();

	// control impacts
	int CreateImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size, float intensity, Tr2Lod lod, EveSpaceObject2* parent );
	bool UpdateImpact( Vector3 & out, const Vector3& direction, int impactIndex );

	// helper for checking activity
	bool HasGeneralActivity() const;
	bool HasShieldActivity() const;
	bool HasArmorActivity() const;
	bool HasHullActivity() const;

private:
	// helper functions to create the different types of impacts
	int CreateShieldImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size, float intensity, EveSpaceObject2* parent );

	Vector3 GetShieldImpactPosition( const Matrix& parentInverseWorldTransform, const Vector3& damageLocatorPosWS, const Vector3& impactDirection, const Vector3& shieldEllipsoidCenter, const Vector3& shieldEllipsoidRadii );

	// general data
	BlueSharedString m_name;
	bool m_display;

	// armor/hull damage lives here, shield writes into its block
	EveDamageOverlayPtr m_damageOverlay;

	// non-directional impacts
	float m_overallShieldImpact;

	// a map of all impacts going on at the moment
	std::map<int, ShieldImpactData> m_shieldImpactData;

	// shield damage
	Tr2MeshBasePtr m_mesh;
	bool m_shieldIsEllipsoid;
	uint32_t m_maxShieldImpacts;
	float m_shieldImpactColorFade;
	float m_shieldImpactParentSize;

	// armor damage
	Tr2GpuUniqueEmitterPtr m_armorImpactEmitter;

	// hull damage
	Tr2GpuUniqueEmitterPtr m_hullImpactEmitter;

	// extenders
	Tr2ScalarFaderPtr m_shieldHardening;
	Tr2ScalarFaderPtr m_shieldBoosting;
};

TYPEDEF_BLUECLASS( EveImpactOverlay );

#endif
