// Copyright © 2015 CCP ehf.

#pragma once
#ifndef EveImpactOverlay_H
#define EveImpactOverlay_H

//#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "ITr2Renderable.h"
#include "Include/ITriTargetable.h"
#include "Resources/Tr2LodResource.h"

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

	enum
	{
		IMPACT_DATA_ROW_0 = 0,
		IMPACT_DATA_ROW_1,
		IMPACT_DATA_ROW_2,
		IMPACT_DATA_ROW_3,
		IMPACT_DATA_ROW_COUNT
	};

	// a row in the data texture
	struct DataRow
	{
		Vector4 v[IMPACT_DATA_ROW_COUNT];
	};

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

	// armor impacts
	struct ArmorImpactData
	{
		int damageLocatorIndex;
		float size;
		bool requestSpawnDebris;
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
	int CreateArmorImpact( int damageLocatorIndex, float size, bool spawnEffects );

	Vector3 GetShieldImpactPosition( const Matrix& parentInverseWorldTransform, const Vector3& damageLocatorPosWS, const Vector3& impactDirection, const Vector3& shieldEllipsoidCenter, const Vector3& shieldEllipsoidRadii );

	// general data
	BlueSharedString m_name;
	bool m_display;
	ITriTargetable::ImpactConfiguration m_configuration;
	int m_impactDataNextIdx;
	bool m_debugForceSpawnDebris;
	float m_armorImpactLifeTime;
	unsigned int m_seed;
	unsigned int m_damageLocatorCount;
	Vector3 m_lastDamageState;

	// priotiy
	float m_renderPriority;
	bool m_isVisibleLast;

	// non-directional impacts
	float m_overallShieldImpact;

	// all the data used in the data texture
	DataRow m_impactTexelHeader;
	std::vector<DataRow> m_impactTexelData;

	// the data texture block ID
	int32_t m_dataTextureBlockID;
	int32_t m_dataTextureOffset;

	// a map of all impacts going on at the moment
	std::map<int, ShieldImpactData> m_shieldImpactData;
	std::map<int, ArmorImpactData> m_armorImpactData;

	// shield damage
	Tr2MeshBasePtr m_mesh;
	bool m_shieldIsEllipsoid;
	uint32_t m_maxShieldImpacts;
	float m_shieldImpactColorFade;
	float m_shieldImpactParentSize;

	// armor damage
	Tr2EffectPtr m_armorDamageShader;
	size_t m_armorImpactGoalCount;
	float m_armorImpactParentSize;
	Tr2GpuUniqueEmitterPtr m_armorImpactEmitter;

	// hull damage
	float m_hullDamageFactor;
	TriPerlinCurvePtr m_hullDamageFlickerCurve;
	Tr2GpuUniqueEmitterPtr m_hullImpactEmitter;

	// extenders
	Tr2ScalarFaderPtr m_shieldHardening;
	Tr2ScalarFaderPtr m_shieldBoosting;
	Tr2ScalarFaderPtr m_armorRepairing;
	Tr2ScalarFaderPtr m_armorHardening;
	Tr2ScalarFaderPtr m_hullRepairing;
};

TYPEDEF_BLUECLASS( EveImpactOverlay );

#endif