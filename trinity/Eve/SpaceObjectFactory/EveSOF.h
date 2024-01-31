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
BLUE_DECLARE_INTERFACE( IEveSpaceObjectDecalOwner );
BLUE_DECLARE( EveSpaceObject2 );
BLUE_DECLARE( EveShip2 );
BLUE_DECLARE( EveSOF );
BLUE_DECLARE( EveSOFDNA );
BLUE_DECLARE( Tr2InstancedMesh );
BLUE_DECLARE( Tr2MeshArea );
BLUE_DECLARE( EveChildContainer );
BLUE_DECLARE_INTERFACE( IEveEffectChildrenOwner );
BLUE_DECLARE_INTERFACE( ITr2LightOwner );
BLUE_DECLARE_INTERFACE( IEveSpaceObjectAttachment );
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

	// change the layout data of the space object
	void RegenerateLayout(EveSpaceObject2* owner, const char* dnaString);

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

	EveSOFDNAPtr CreateDna( const char* dnaString );

	// all setup functions for the to-be-created space object
	void SetupConsts( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupMesh( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupAttachments( IEveSpaceObjectAttachmentOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const;
	void SetupSpriteSets( IEveSpaceObjectAttachmentOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const;
	void SetupSpotlightSets( IEveSpaceObjectAttachmentOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const;
	void SetupPlaneSets( IEveSpaceObjectAttachmentOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const;
	void SetupSpriteLineSets( IEveSpaceObjectAttachmentOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const;
	void SetupHazeSets( IEveSpaceObjectAttachmentOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const;
	void SetupBanners( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets ) const; 
	void SetupBannerSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets ) const; 
	void SetupEffects( EveSpaceObject2Ptr obj, IEveEffectChildrenOwnerPtr childOwner, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const;
	void SetupChildrenAndAnimations( EveSpaceObject2Ptr obj, IEveEffectChildrenOwnerPtr childOwner, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const;
	void SetupEffectChildren( EveSpaceObject2Ptr newObj, IEveEffectChildrenOwnerPtr childOwner, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const;
	void SetupControllers( ITr2ControllerOwnerPtr newObj, const EveSOFDNAPtr dna, uint32_t buildFlags ) const;
	void SetupAudio( ITr2SoundEmitterOwnerPtr newObj, const EveSOFDNAPtr dna, const Matrix& offset = IdentityMatrix() ) const;
	void SetupInstancedMeshes( EveSpaceObject2Ptr newObj, EveSOFDNAPtr dna, const std::vector<Matrix>& offsets ) const;
	void SetupDecalSets( IEveSpaceObjectDecalOwnerPtr obj, const EveSOFDNAPtr dna ) const;
	void SetupModelCurves( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupLocators( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupLocatorSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets );
	void SetupImpactEffects( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;
	void SetupLights( ITr2LightOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets ) const;
	void SetupLayout( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t seedOverwrite = 0 );


	Tr2MeshPtr CreateMesh( const EveSOFDNAPtr dna ) const;
	Tr2InstancedMeshPtr CreateInstancedMesh( std::vector<EveSOFDataMgr::HullMeshInstance> instances, std::string resPath ) const;
	void SetupShaders( const EveSOFDNAPtr dna, Tr2MeshBase* mesh ) const;
	
	EveChildContainerPtr CreatePlacement( EveSpaceObject2Ptr parent, EveSOFDNAPtr extensionDna, EveSOFDataMgr::ExtensionPlacementData & placement, const std::vector<EveSOFDataMgr::LocatorDirectionData>& locators, const std::vector<Matrix>& nestedOffsets );

	void SetupCustomMask( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const;

	// all setup functions for ships only
	void SetupBoosters( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const;

	Tr2EffectPtr CreateBoosterEffect( const EveSOFDataMgr::RaceBoosterData* rdata, const BlueSharedString& lodOption ) const;

	bool ProcessLayoutDistributionConditions(  EveSOFDataMgr::ExtensionPlacementData& placement, const EveSOFDNAPtr dna );
	void ProcessLayoutDistributionDistribute( EveSOFDataMgr::ExtensionPlacementDistribution& distributionData, const EveSOFDNAPtr dna, std::vector<EveSOFDataMgr::LocatorDirectionData>& placementSet, std::vector<EveSOFDataMgr::LocatorDirectionData>& managedLocatorSet );
	void ProcessPlacementDistributionOrGroup( EveSOFDataMgr::ExtensionPlacementData & distributionData, EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna, std::map<BlueSharedString, std::vector<EveSOFDataMgr::LocatorDirectionData>>& managedLocatorSet, size_t& layoutIdx, size_t& placementIdx, const std::vector<Matrix>& offsets );

	// helper functions
	size_t FillMeshAreaVector( Tr2MeshAreaVector* meshAreaVector, TriBatchType areaType, const EveSOFDNAPtr dna, size_t hullIdx, size_t meshIndexOffset ) const;
	bool GenerateLodResourcePaths( std::string& mediumResPath, std::string& lowResPath, std::string& ultraResPath, const char* resPath, const char* usage ) const;
	void GenerateDepthFromAreaVector( Tr2MeshBase* mesh, const Tr2MeshAreaVector* meshAreaVector, const EveSOFDNAPtr dna ) const;
	void CreatePointLightData( const Vector3& pos, const float scale, const Color& color, const EveSOFDataMgr::PointLightAttachment* lightData ) const;
	void CreateTexturedPointLightData( const Vector3& pos, const float scale, const std::string& texturePath, const EveSOFDataMgr::PointLightAttachment* lightData ) const;

	// all the source data
	PEveSOFDataMgr m_dataMgr;

	// shared
	Tr2EffectPtr m_hazeSetEffectSpherical, m_skinnedHazeSetEffectSpherical, m_hazeSetEffectHalfSpherical;
	Tr2EffectPtr m_spriteSetEffect;
	Tr2EffectPtr m_shadowEffect, m_shadowEffectSkinned;
	BlueSharedString m_depthOnlyEffectName, m_decalsEffectName[EveSOFDataHullDecalSetItem::USAGE_MAX];

	mutable std::unordered_map<std::string, bool> m_existingFilesCache;
	bool m_allowFileCaching;

	bool m_editorMode;
};

TYPEDEF_BLUECLASS( EveSOF );

#endif // EveSOF_H