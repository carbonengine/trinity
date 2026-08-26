// Copyright © 2026 CCP ehf.

#pragma once
#ifndef EveDamageOverlay_H
#define EveDamageOverlay_H

#include "ITr2Renderable.h"
#include "Include/ITriTargetable.h"

#include <functional>

class EveUpdateContext;
class Tr2DataTextureManager;

BLUE_DECLARE( TriPerlinCurve );
BLUE_DECLARE( Tr2ScalarFader );
BLUE_DECLARE( Tr2Effect );

// armor impact sizing, shared with the debris emitter scaling in EveImpactOverlay
static const float IMPACT_ARMOR_SIZE_FACTOR = 0.0129f;
static const float IMPACT_ARMOR_SIZE_MAX = 10.f;


// ---------------------------------------------------------------------------------------
//  Description:
//    Armor/hull surface damage, self-contained so both a space object (via its
//    EveImpactOverlay) and a child mesh can own one. Owns the shared data-texture block;
//    shield rows are written into it by the owning EveImpactOverlay on the ship path.
// ---------------------------------------------------------------------------------------
BLUE_CLASS( EveDamageOverlay ) :
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	EveDamageOverlay( IRoot* lockobj = NULL );
	~EveDamageOverlay();

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

	// armor impacts
	struct ArmorImpactData
	{
		int damageLocatorIndex;
		float size;
		bool requestSpawnDebris;
	};

	// what the overlay needs to know about whoever owns it
	struct OwnerInfo
	{
		Vector4 boundingSphere = Vector4( 0.f, 0.f, 0.f, -1.f );
		float estimatedPixelDiameter = 0.f;
		bool isInFrustum = false;
		std::function<bool( int, Vector3& )> getDamageLocatorPositionOS;
	};

	//////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// Updates
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const OwnerInfo& info, size_t minTexelRows, bool hasExternalActivity );
	void UpdateSyncronous( const EveUpdateContext& updateContext );
	void UpdateBlockData( Tr2DataTextureManager * dataTextureMgr, bool hasActivity );

	/////////////////////////////////////////////////////////////////////////////////////
	// Rendering
	Tr2Effect* GetArmorDamageShader( TriBatchType batchType ) const;

	// getters
	int32_t GetDataTextureOffset() const;
	int32_t GetDataTextureBlockID() const;
	ITriTargetable::ImpactConfiguration GetImpactConfiguration() const;
	float GetActivationStrength( const EveUpdateContext& updateContext ) const;
	float GetArmorImpactLifeTime() const;
	Vector3 GetLastDamageState() const;
	float GetRenderPriority() const;
	size_t GetArmorImpactGoalCount() const;
	float GetArmorImpactParentSize() const;
	int GetImpactDataNextIdx() const;
	unsigned int GetSeed() const;
	bool GetDebugForceSpawnDebris() const;
	float GetHullDamageFactor() const;
	Tr2Effect* GetArmorDamageShaderEffect() const;
	TriPerlinCurve* GetHullDamageFlickerCurve() const;
	Tr2ScalarFader* GetArmorRepairing() const;
	Tr2ScalarFader* GetArmorHardening() const;
	Tr2ScalarFader* GetHullRepairing() const;

	// setters
	void SetSeed( const unsigned int seed );
	void SetDamageLocatorCount( unsigned int count );
	void SetEnabledDamageLocators( std::vector<bool>::iterator begin, std::vector<bool>::iterator end );
	void SetDebugForceSpawnDebris( bool value );
	void SetHullDamageFactor( float factor );
	void SetArmorDamageShaderEffect( Tr2Effect * shader );
	void SetHullDamageFlickerCurve( TriPerlinCurve * curve );
	void SetArmorRepairing( Tr2ScalarFader * fader );
	void SetArmorHardening( Tr2ScalarFader * fader );
	void SetHullRepairing( Tr2ScalarFader * fader );
	void SetImpactIndexSource( EveDamageOverlay * source );

	// control animation
	void ToggleEffect( const char* name, bool on, float duration );

	// set the damages
	void SetDamageState( float shield, float armor, float hull, bool doCreateArmorImpacts );
	void Clear();

	// control impacts
	int CreateImpact( int damageLocatorIndex, float size, bool spawnEffects );
	bool HasImpact( int impactIndex ) const;
	int AllocateImpactIndex();

	// helper for checking activity
	bool HasGeneralActivity() const;
	bool HasArmorActivity() const;
	bool HasHullActivity() const;

	// shared block access for the owning EveImpactOverlay's shield rows
	DataRow& HeaderRow();
	DataRow& TexelRow( size_t index );
	std::map<int, ArmorImpactData>& ArmorImpacts();

private:
	// general data
	bool m_display;
	ITriTargetable::ImpactConfiguration m_configuration;
	int m_impactDataNextIdx;
	// when set, impact indices come from this overlay's counter instead, so a ship and all
	// of its parts hand out indices from one namespace and UpdateImpact stays unambiguous
	EveDamageOverlayPtr m_impactIndexSource;
	bool m_debugForceSpawnDebris;
	float m_armorImpactLifeTime;
	unsigned int m_seed;
	unsigned int m_damageLocatorCount;
	std::vector<uint32_t> m_enabledDamageLocators;
	Vector3 m_lastDamageState;

	// priority
	float m_renderPriority;
	bool m_isVisibleLast;

	// all the data used in the data texture
	DataRow m_impactTexelHeader;
	std::vector<DataRow> m_impactTexelData;

	// the data texture block ID
	int32_t m_dataTextureBlockID;
	int32_t m_dataTextureOffset;

	// a map of all impacts going on at the moment
	std::map<int, ArmorImpactData> m_armorImpactData;

	// armor damage
	Tr2EffectPtr m_armorDamageShader;
	size_t m_armorImpactGoalCount;
	float m_armorImpactParentSize;

	// hull damage
	float m_hullDamageFactor;
	TriPerlinCurvePtr m_hullDamageFlickerCurve;

	// extenders
	Tr2ScalarFaderPtr m_armorRepairing;
	Tr2ScalarFaderPtr m_armorHardening;
	Tr2ScalarFaderPtr m_hullRepairing;
};

TYPEDEF_BLUECLASS( EveDamageOverlay );

#endif
