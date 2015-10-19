////////////////////////////////////////////////////////////
//
//    Created:   September 2015
//    Copyright: CCP 2015
//

#pragma once
#ifndef EveImpactOverlay_H
#define EveImpactOverlay_H

#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "ITr2Renderable.h"
#include "ITr2GeometryProvider.h"
#include "Resources/Tr2LodResource.h"

BLUE_DECLARE( TriFrustum );
BLUE_DECLARE( Tr2MeshBase );
BLUE_DECLARE( EveUpdateContext );

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
	};

	// armor impacts
	struct ArmorImpactData
	{
		int damageLocatorIndex;
		float timeLeft;
	};

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// Updates
	void UpdateSyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent );
	void UpdateAsyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent );
	
	/////////////////////////////////////////////////////////////////////////////////////
	// Rendering
	void GetBatches( ITriRenderBatchAccumulator* accumulator, TriBatchType batchType, const Tr2PerObjectData* perObjectData );
	Tr2EffectPtr GetArmorDamageShader( TriBatchType batchType ) const;

	// control animation
	void PlayCurveSet( const std::string& name );
	void StopAllCurveSets();

	// control shield impacts
	int CreateShieldImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime );
	bool UpdateShieldImpact( const Vector3& direction, int shieldImpactIndex );
	bool GetShieldImpactPosition( Vector3& out, int shieldImpactIndex ) const;

	// control armor impacts
	int CreateArmorImpact( int damageLocatorIndex );

	// helper for checking activity
	bool HasActivity() const;

private:
	// general data
	BlueSharedString m_name;
	bool m_display;

	// non-directional impacts
	float m_overallShieldImpact;

	// limits
	uint32_t m_maxShieldImpacts;

	// parent object
	Vector3 m_shieldEllipsoidRadii;
	Vector3 m_shieldEllipsoidCenter;

	// a map of all shield impacts going on at the moment
	int m_shieldImpactDataNextIdx;
	std::map<int, ShieldImpactData> m_shieldImpactData;

	// a list of data used in the data texture
	std::vector<DataRow> m_impactTexelData;

	// a map of all armor impacts going on at the moment
	int m_armorImpactDataNextIdx;
	std::map<int, ArmorImpactData> m_armorImpactData;

	// the data texture block ID
	int32_t m_dataTextureBlockID;

	// what to render
	Tr2MeshBasePtr m_mesh;

	// armor damage shader
	Tr2EffectPtr m_armorDamageShader;

	// animate
	PTriCurveSetVector m_curveSets;
};

TYPEDEF_BLUECLASS( EveImpactOverlay );

#endif