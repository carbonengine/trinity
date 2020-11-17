////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#pragma once
#ifndef EveSOF_H
#define EveSOF_H

#include "EveSOFDataMgr.h"
#include "ITr2Renderable.h"

// forwards
BLUE_DECLARE( EveTurretSet );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2LodResource );
BLUE_DECLARE( EveSpaceObject2 );
BLUE_DECLARE( EveShip2 );
BLUE_DECLARE( EveSOF );
BLUE_DECLARE( EveSOFDNA );
BLUE_DECLARE( Tr2MeshLod );
BLUE_DECLARE( Tr2MeshArea );
BLUE_DECLARE_VECTOR( Tr2MeshArea );

// --------------------------------------------------------------------------------
// Description:
//   This class is for rendering all of one ship's trails.
//   The object is part of EveBoosterSet2
// SeeAlso:
//   EveBoosterSet2
// --------------------------------------------------------------------------------
BLUE_CLASS( EveSOF ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	EveSOF( IRoot* lockobj = NULL );
	~EveSOF();

	// build a spaceship and return a EveShip2 object
	IRootPtr Build( const char* hullName, const char* factionName, const char* raceName );
	// build a spaceship from a dns string and return a EveShip2 object
	IRootPtr BuildFromDNA( const char* dnaString );

	// validate a dna string (slow!)
	bool ValidateDNA( const char* dnaString );

	// change the material of a turret with SOF data
	void SetupTurretMaterialFromDNA( EveTurretSet* turretSet, const char* dnaString );
	void SetupTurretMaterialFromFaction( EveTurretSet* turretSet, const char* factionName );

	bool LoadData( const char* filePath );
private:
	// creation
	EveSpaceObject2Ptr CreateSpaceObject( const EveSOFDNAPtr dna ) const;

	struct InheritableTextureKey
	{
		size_t hullIndex;
		int32_t meshIndex;
		BlueSharedString name;

		bool operator==( const InheritableTextureKey& other ) const
		{
			return hullIndex == other.hullIndex && meshIndex == other.meshIndex && name == other.name;
		}

		operator size_t() const
		{
			return hullIndex | ( meshIndex << 4 ) | reinterpret_cast<size_t>( name.c_str() );
		}

		// Use the object as its own hasher (needs to be explicit for non fundamental types in >VS2015)
		size_t operator() ( const InheritableTextureKey &key ) const
		{
			return key;
		}
	};

	typedef std::unordered_map<InheritableTextureKey, Tr2LodResource*, InheritableTextureKey> InheritableTextures;

	// all setup functions for the to-be-created space object
	void SetupConsts( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupMesh( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna, InheritableTextures& inheritableTextures ) const;
	void SetupSpriteSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupSpotlightSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupPlaneSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupSpriteLineSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupHazeSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupBannerSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupChildrenAndAnimations( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupEffectChildren( EveSpaceObject2Ptr newObj, const EveSOFDNAPtr dna ) const;
	void SetupControllers( EveSpaceObject2Ptr newObj, const EveSOFDNAPtr dna ) const;
	void SetupAudio( EveSpaceObject2Ptr newObj, const EveSOFDNAPtr dna ) const;
	void SetupInstancedMeshes( EveSpaceObject2Ptr newObj, EveSOFDNAPtr dna ) const;
	void SetupDecalSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna, const InheritableTextures& inheritableTextures ) const;
	void SetupModelCurves( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupLocators( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupEffects( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupLights( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;

	void SetupCustomMask( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;

	// all setup functions for ships only
	void SetupBoosters( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const;

	Tr2EffectPtr CreateBoosterEffect( const EveSOFDataMgr::RaceBoosterData* rdata, const BlueSharedString& lodOption ) const;

	// helper functions
	size_t FillMeshAreaVector( std::map<std::string, Tr2LodResourcePtr>& lodResCollector, Tr2MeshAreaVector* meshAreaVector, TriBatchType areaType, const EveSOFDNAPtr dna, size_t hullIdx, size_t meshIndexOffset, InheritableTextures* inheritableTextures ) const;
	bool GenerateLodResourcePaths( std::string& mediumResPath, std::string& lowResPath, std::string& ultraResPath, const char* resPath, const char* usage ) const;
	void GenerateDepthFromAreaVector( Tr2MeshLodPtr mesh, const Tr2MeshAreaVector* meshAreaVector, const EveSOFDNAPtr dna ) const;

	// all the source data
	PEveSOFDataMgr m_dataMgr;

	// shared
	Tr2EffectPtr m_hazeSetEffectSpherical, m_hazeSetEffectHalfSpherical;
	Tr2EffectPtr m_spriteSetEffect;
	Tr2EffectPtr m_shadowEffect, m_shadowEffectSkinned;
	BlueSharedString m_depthOnlyEffectName, m_decalsEffectName[EveSOFDataHullDecalSetItem::USAGE_MAX];
};

TYPEDEF_BLUECLASS( EveSOF );

#endif // EveSOF_H