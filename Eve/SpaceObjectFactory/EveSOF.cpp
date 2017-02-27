////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "Utilities/StringUtils.h"
#include "EveSOF.h"
#include "EveSOFDNA.h"
#include "EveSOFUtils.h"
#include "Tr2ExternalParameter.h"
#include "Eve/EveTransform.h"
#include "Eve/Turret/EveTurretSet.h"
#include "Eve/SpaceObject/EveShip2.h"
#include "Eve/SpaceObject/EveStation2.h"
#include "Eve/SpaceObject/EveSwarm.h"
#include "Eve/SpaceObject/Utils/EveCustomMask.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteLineSet.h"
#include "Eve/SpaceObject/Attachments/EveTrailsSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpotlightSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EvePlaneSet.h"
#include "Eve/SpaceObject/Attachments/EveBoosterSet2.h"
#include "Eve/SpaceObject/Attachments/EveSpaceObjectDecal.h"
#include "Eve/SpaceObject/Children/EveChildMesh.h"
#include "Eve/SpaceObject/Children/EveChildContainer.h"
#include "Eve/SpaceObject/Children/EveChildParticleSystem.h"
#include "Eve/SpaceObject/Utils/EveLocator2.h"
#include "Tr2InstancedMesh.h"
#include "Tr2MeshLod.h"
#include "Tr2MeshArea.h"
#include "Tr2RuntimeInstanceData.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Parameter/TriTextureParameter.h"
#include "Shader/Parameter/Tr2Vector4Parameter.h"
#include "Shader/Parameter/Tr2FloatParameter.h"
#include "include/ITriFunction.h"
#include "Curves/Tr2QuaternionCurve.h"
#include "Curves/Tr2ScalarCurve.h"
#include "Curves/TriCurveSet.h"
#include "Curves/TriRotationCurve.h"
#include "TriValueBinding.h"
#include "Particle/Tr2DynamicEmitter.h"
#include "Particle/Tr2GpuUniqueEmitter.h"
#include "TriSequencer.h"


// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveSOF::EveSOF( IRoot* lockobj ) :
	PARENTLOCK( m_dataMgr )
{
	// pre-register some really needed vars in the global variable store
	Tr2Variable var1( "DepthMap", (ITr2TextureProvider*)nullptr );
	Tr2Variable var2( "DepthMapMsaa", (ITr2TextureProvider*)nullptr );

	BlueSharedString mainIntensity( "MainIntensity" );
	BlueSharedString gradientMap( "GradientMap" );

	// some shared shaders here
	m_spriteSetEffect.CreateInstance();
	m_spriteSetEffect->StartUpdate();
	m_spriteSetEffect->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/fx/blinkinglightspool.fx" );
	m_spriteSetEffect->AddParameterFloat( mainIntensity, 1.f );
	m_spriteSetEffect->AddResourceTexture2D( gradientMap, "res:/texture/particle/whitesharp_gradient.dds" );
	m_spriteSetEffect->EndUpdate();

	m_shadowEffect.CreateInstance();
	m_shadowEffect->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/shadow/shadow.fx" );
	m_shadowEffectSkinned.CreateInstance();
	m_shadowEffectSkinned->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/shadow/skinned_shadow.fx" );
}

// --------------------------------------------------------------------------------
// Description:
//   tear down
// --------------------------------------------------------------------------------
EveSOF::~EveSOF()
{

}

// --------------------------------------------------------------------------------
// Description:
//   Route this call through to the data manager
// --------------------------------------------------------------------------------
bool EveSOF::LoadData( const char* filePath )
{
	return m_dataMgr.LoadData( filePath );
}

// --------------------------------------------------------------------------------
// Description:
//   Build a ship from a dna string
// --------------------------------------------------------------------------------
IRootPtr EveSOF::BuildFromDNA( const char* dnaString )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// create a temporary(!) DNA object
	EveSOFDNAPtr dna;
	dna.CreateInstance();

	// init it with given dna string
	dna->Setup( dnaString, &m_dataMgr );
	if( !dna->IsValid() )
	{
		return nullptr;
	}

	// create what we need to build
	EveSpaceObject2Ptr newObj = CreateSpaceObject( dna );
	if( newObj == nullptr )
	{
		return nullptr;
	}

	// set all easy consts
	SetupConsts( newObj, dna );

	// get us the base geometry
	SetupMesh( newObj, dna );
	SetupCustomMask( newObj, dna );

	// decals
	SetupDecals( newObj, dna );

	// effects on ships
	SetupSpriteSets( newObj, dna );
	SetupSpotlightSets( newObj, dna );
	SetupPlaneSets( newObj, dna );
	SetupSpriteLineSets( newObj, dna );
	SetupEffects( newObj, dna );

	// attachments to ship
	SetupLocators( newObj, dna );

	// children, animations and particles
	SetupChildrenAndAnimations( newObj, dna );

	// model curves
	SetupModelCurves( newObj, dna );

	// instanced meshes
	SetupInstancedMeshes( newObj, dna );

	// EveShip2-specific setups
	EveShip2Ptr newShip;
	if( newObj->GetRawRoot()->QueryInterface( BlueInterfaceIID<EveShip2>(), (void**)&newShip, BEQI_SILENT ) )
	{
		SetupBoosters( newShip, dna );
	}

	// EveSwarm-specific setups
	EveSwarmPtr newSwarm;
	if( newObj->GetRawRoot()->QueryInterface( BlueInterfaceIID<EveSwarm>(), (void**)&newSwarm, BEQI_SILENT ) )
	{
		newSwarm->SetBehavior( dna->GetGenericSwarmProperties() );
	}

	// ships needs a final ::Initialize call
	newObj->Initialize();

	return newObj->GetRawRoot();
}

// --------------------------------------------------------------------------------
// Description:
//   This is where it is all going to happen: building a ship with the three
//   main infos
// --------------------------------------------------------------------------------
IRootPtr EveSOF::Build( const char* hullName, const char* factionName, const char* raceName )
{
	// make a SOF ship dna string
	std::string dnaString = std::string( hullName ) + ":" + std::string( factionName ) + ":" + std::string( raceName );

	// pass on to real build function
	return BuildFromDNA( dnaString.c_str() );
}

// --------------------------------------------------------------------------------
// Description:
//   Validate a given DNA string. This is slow and should be used only for offline
//   validation!
// --------------------------------------------------------------------------------
bool EveSOF::ValidateDNA( const char* dnaString )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// create a DNA object
	EveSOFDNAPtr dna;
	dna.CreateInstance();

	// init it with given dna string
	dna->Setup( dnaString, &m_dataMgr );
	if( !dna->IsValid() )
	{
		return false;
	}

	// let the dna validate itself. this is the slow part
	if( !dna->ValidateContent() )
	{
		return false;
	}

	// if we made it this far, this DNA is valid
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Create the spaceobject class dependent on requested size
// --------------------------------------------------------------------------------
EveSpaceObject2Ptr EveSOF::CreateSpaceObject( const EveSOFDNAPtr dna ) const
{
	EveSpaceObject2Ptr spaceObject = nullptr;

	switch( dna->GetBuildClass() )
	{
	case EveSOFDataHull::BUILDCLASS_STATIONARY:
		{
			EveStation2Ptr newStation;
			newStation.CreateInstance();
			spaceObject = newStation;
		}
		break;
	case EveSOFDataHull::BUILDCLASS_MOBILE:
		{
			EveMobilePtr newMobile;
			newMobile.CreateInstance();
			spaceObject = newMobile;
		}
		break;
	case EveSOFDataHull::BUILDCLASS_SHIP:
		{
			EveShip2Ptr newShip;
			newShip.CreateInstance();
			spaceObject = newShip;
		}
		break;
	case EveSOFDataHull::BUILDCLASS_SWARM:
		{
			EveSwarmPtr newSwarm;
			newSwarm.CreateInstance();
			spaceObject = newSwarm;
		}
		break;
    default:
        break;
	}

	return spaceObject;
}

// --------------------------------------------------------------------------------
// Description:
//   Set mostly simple constants values to the ship
// --------------------------------------------------------------------------------
void EveSOF::SetupConsts( EveSpaceObject2Ptr ship, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// dna dirt
	ship->SetDnaString( dna->GetDnaString() );
}

// --------------------------------------------------------------------------------
// Description:
//   This is where it is all going to happen
// --------------------------------------------------------------------------------
void EveSOF::SetupMesh( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// need a mesh
	Tr2MeshLodPtr mesh;
	mesh.CreateInstance();

	std::string highDetail = dna->GetHullGeometryResPath();
	std::string mediumDetail = highDetail;
	std::string lowDetail = highDetail;
	StringInsertStubBefore( mediumDetail, ".gr2", "_mediumDetail" );
	StringInsertStubBefore( lowDetail, ".gr2", "_lowDetail" );

	Tr2LodResourcePtr lodResource;
	lodResource.CreateInstance();
	lodResource->SetName( BlueSharedString( "Geometry" ) );
	lodResource->SetResourcePath( TR2_LOD_LOW, lowDetail.c_str() );
	lodResource->SetResourcePath( TR2_LOD_MEDIUM, mediumDetail.c_str() );
	lodResource->SetResourcePath( TR2_LOD_HIGH, highDetail.c_str() );
	lodResource->SelectLod( TR2_LOD_LOW );
	
	// gr2 res path
	mesh->SetGeometryResource( lodResource );

	// bounding sphere comes from data, is faster
	obj->SetBoundingSphereInformation( dna->GetHullBoundingSphere() );
	obj->SetShapeEllipsoid( dna->GetHullShapeEllipsoidCenter(), dna->GetHullShapeEllipsoidRadius() );
	obj->EnableDynamicBoundingSphere( dna->DynamicBoundingSphereEnabled() );

	// shadow
	if( dna->IsHullAnimated() )
	{
		obj->SetShadowEffect( m_shadowEffectSkinned );
	}
	else
	{
		obj->SetShadowEffect( m_shadowEffect );
	}

	// setup mesh areas, try sharing as many Tr2LodResources as possible
	std::map<std::string, Tr2LodResourcePtr> lodResPerTexture;
	FillMeshAreaVector( lodResPerTexture, mesh->GetAreas( TRIBATCHTYPE_OPAQUE ), TRIBATCHTYPE_OPAQUE, dna );
	FillMeshAreaVector( lodResPerTexture, mesh->GetAreas( TRIBATCHTYPE_DECAL ), TRIBATCHTYPE_DECAL, dna );
	FillMeshAreaVector( lodResPerTexture, mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT ), TRIBATCHTYPE_TRANSPARENT, dna );
	FillMeshAreaVector( lodResPerTexture, mesh->GetAreas( TRIBATCHTYPE_ADDITIVE ), TRIBATCHTYPE_ADDITIVE, dna );
	FillMeshAreaVector( lodResPerTexture, mesh->GetAreas( TRIBATCHTYPE_DEPTH ), TRIBATCHTYPE_DEPTH, dna );
	FillMeshAreaVector( lodResPerTexture, mesh->GetAreas( TRIBATCHTYPE_DISTORTION ), TRIBATCHTYPE_DISTORTION, dna );

	// register all used lodresource objects with the new mesh
	for( auto it = lodResPerTexture.begin(); it != lodResPerTexture.end(); ++it )
	{
		mesh->AddAssociatedResource( it->second );
	}

	// preselect a lod
	mesh->SelectLod( TR2_LOD_LOW );

	// assign mesh to ship
	obj->SetMeshLod( mesh );
}

// --------------------------------------------------------------------------------
// Description:
//   Fill up mesh area vector given the hull and faction area data provided.
// --------------------------------------------------------------------------------
void EveSOF::FillMeshAreaVector( std::map<std::string, Tr2LodResourcePtr>& lodResCollector, Tr2MeshAreaVector* meshAreaVector, TriBatchType areaType, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	const std::vector<EveSOFDataMgr::HullAreas>* hullAreas = dna->GetHullMeshAreas( areaType );
	for( auto area = hullAreas->begin(); area != hullAreas->end(); ++area )
	{
		// find data on this shader from generics, we need it!
		const EveSOFDataMgr::GenericShaderData* shaderData = dna->GetGenericAreaShaderData( area->shader );
		if( shaderData )
		{

			// every area has it's own shader, nothing we can share here
			Tr2EffectPtr newShader;
			newShader.CreateInstance();
			newShader->StartUpdate();

			// construct res path of the shader
			newShader->SetEffectPathName( dna->GetCompleteShaderPath( area->shader.c_str() ).c_str() );

			// parameters
			for( auto shaderParamIt = shaderData->parameters.begin(); shaderParamIt != shaderData->parameters.end(); ++shaderParamIt )
			{
				const Vector4* paramValue = dna->GetMeshAreaParameter( area->areaType, *shaderParamIt, &area->parameters, area->blockedMaterials );
				if( paramValue )
				{
					newShader->AddParameterVector4( *shaderParamIt, paramValue );
				}
			}

			// shader textures from the hull data
			for( auto it = area->textures.begin(); it != area->textures.end(); ++it )
			{
				CCP_STATS_ZONE( __FUNCTION__ " texture" );

				// res path how it is from hull data
				std::string highResPath = it->second.resFilePath;
				// get's modified by the faction data
				dna->ModifyTextureResPath( highResPath, it->first.c_str() );
				// make three paths for the three LODs
				std::string mediumResPath, lowResPath, ultraResPath;
				if( GenerateLodResourcePaths( mediumResPath, lowResPath, ultraResPath, highResPath.c_str(), it->first.c_str() ) )
				{
					// now we need a lod resource object, maybe we already have one?
					auto finder = lodResCollector.find( highResPath );
					if( finder != lodResCollector.end() )
					{
						// yeah, we have one: use it!
						newShader->AddResourceTexture2DLod( it->first, finder->second );
					}
					else
					{
						// not found, so we have to make a new one
						Tr2LodResourcePtr lodResource;
						lodResource.CreateInstance();

						CCP_STATS_ZONE( __FUNCTION__ " lodResource" );

						lodResource->SetName( it->first );
						lodResource->SetResourcePath( TR2_LOD_LOW, lowResPath.c_str() );
						lodResource->SetResourcePath( TR2_LOD_MEDIUM, mediumResPath.c_str() );
						lodResource->SetResourcePath( TR2_LOD_HIGH, highResPath.c_str() );
						lodResource->SetResourcePath( TR2_LOD_ULTRA, ultraResPath.c_str() );
						newShader->AddResourceTexture2DLod( it->first, lodResource );
						// also add it to the mesh for updating
						lodResCollector[ highResPath ] = lodResource;
					}
				}
				else
				{
					newShader->AddResourceTexture2D( it->first, highResPath.c_str() );
				}
			}

			// pattern textures
			size_t patternLayerCount = dna->GetPatternLayerCount();
			for( size_t i = 0; i < patternLayerCount; ++i )
			{
				const EveSOFDataMgr::PatternLayerData* patternLayerData = dna->GetPatternLayerData( i );
				if( patternLayerData )
				{
					newShader->AddResourceTexture2D( patternLayerData->textureName, patternLayerData->textureResFilePath.c_str() );

					// pattern textures almost always require a sampler change: repeat is boring....
					newShader->AddSamplerOverride( BlueSharedString( std::string( patternLayerData->textureName.c_str() ) + "Sampler" ), patternLayerData->projectionAddressModeU, patternLayerData->projectionAddressModeV );
				}
			}

			// default shader textures & parameters from the generic data
			for( auto gtit = shaderData->defaultTextures.begin(); gtit != shaderData->defaultTextures.end(); ++gtit )
			{
				newShader->AddResourceTexture2D( gtit->first, gtit->second.resFilePath.c_str() );
			}
			for( auto gpit = shaderData->defaultParameters.begin(); gpit != shaderData->defaultParameters.end(); ++gpit )
			{
				newShader->AddParameterVector4( gpit->first, &gpit->second );
			}

			// that's it for setting up this shader, must rebuild cache on it!
			newShader->EndUpdate();

			// new mesharea
			Tr2MeshAreaPtr newMeshArea;
			newMeshArea.CreateInstance();
			newMeshArea->SetMaterial( newShader );
			newMeshArea->SetIndex( area->index );
			newMeshArea->SetCount( area->count );
			meshAreaVector->Append( newMeshArea );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Helper function to build two med and low level texture res paths
// --------------------------------------------------------------------------------
bool EveSOF::GenerateLodResourcePaths( std::string& mediumResPath, std::string& lowResPath, std::string& ultraResPath, const char* resPath, const char* usage ) const
{
	// only dds files will ever have lods
	if( StringFind( resPath, ".dds" ) )
	{
		if( !strcmp( usage, "PmdgMap" ) || !strcmp( usage, "ArMap" )|| !strcmp( usage, "NoMap" )  )
		{
			ultraResPath = resPath;
			StringInsertStubBefore( ultraResPath, ".dds", "_ultraDetail" );
			mediumResPath = resPath;
			StringInsertStubBefore( mediumResPath, ".dds", "_mediumDetail" );
			lowResPath = resPath;
			StringInsertStubBefore( lowResPath, ".dds", "_lowDetail" );
			return true;
		}
	}
	return false;
}

// --------------------------------------------------------------------------------
// Description:
//   This is where it is all going to happen
// --------------------------------------------------------------------------------
void EveSOF::SetupSpriteSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// cycle over all spritesets of this hull
	const std::vector<EveSOFDataMgr::HullSpriteSetData>& hullSpriteSets = dna->GetHullSpriteSets();
	for( auto ssit = hullSpriteSets.begin(); ssit != hullSpriteSets.end(); ++ssit )
	{
		const EveSOFDataMgr::HullSpriteSetData* spriteSetData = &(*ssit);

		// create a spriteset for this ship
		EveSpriteSetPtr spriteSet;
		spriteSet.CreateInstance();
		// set skinned or unskinned shader
		spriteSet->SetEffect( m_spriteSetEffect );
		spriteSet->SetSkinned( spriteSetData->skinned );
		// add all the individual items
		for( auto ssiit = spriteSetData->items.begin(); ssiit != spriteSetData->items.end(); ++ssiit )
		{
			const EveSOFDataMgr::HullSpriteSetItemData* itemData = &(*ssiit);

			// faction data?
			const EveSOFDataMgr::FactionSpriteSetColorData* factionSpriteData = dna->GetFactionSpriteSetData( itemData->groupIndex );
			if( !factionSpriteData )
			{
				// This spriteset item is not used for this faction.
				continue;
			}

			// create spriteset items
			EveSpriteSetItemPtr spriteSetItem;
			spriteSetItem.CreateInstance();

			// set it up the per-faction data
			spriteSetItem->m_color = factionSpriteData->color;

			// set it up the per-hull data
			spriteSetItem->m_blinkPhase = itemData->blinkPhase;
			spriteSetItem->m_blinkRate = itemData->blinkRate;
			spriteSetItem->m_boneIndex = itemData->boneIndex;
			spriteSetItem->m_falloff = itemData->falloff;
			spriteSetItem->m_maxScale = itemData->maxScale;
			spriteSetItem->m_minScale = itemData->minScale;
			spriteSetItem->m_position = itemData->position;

			// put it into spriteset
			spriteSet->Add( spriteSetItem );
		}
		// spriteset needs internal rebuild
		spriteSet->Rebuild();
		// put set onto ship
		obj->AddSpriteSet( spriteSet );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   This is where it is all going to happen
// --------------------------------------------------------------------------------
void EveSOF::SetupSpotlightSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// cycle over all spritesets of this hull
	const std::vector<EveSOFDataMgr::HullSpotlightSetData>& hullSpotlightSets = dna->GetHullSpotlightSets();
	for( auto ssit = hullSpotlightSets.begin(); ssit != hullSpotlightSets.end(); ++ssit )
	{
		const EveSOFDataMgr::HullSpotlightSetData* spotlightSetData = &(*ssit);

		// create a spriteset for this ship
		EveSpotlightSetPtr spotlightSet;
		spotlightSet.CreateInstance();

		// create shaders
		Tr2EffectPtr coneEffect;
		coneEffect.CreateInstance();
		coneEffect->StartUpdate();
		Tr2EffectPtr glowEffect;
		glowEffect.CreateInstance();
		glowEffect->StartUpdate();

		coneEffect->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/fx/spotlightconepool.fx" );
		glowEffect->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/fx/spotlightglowpool.fx" );

		// textures
		BlueSharedString textureMap("TextureMap");
		glowEffect->AddResourceTexture2D( textureMap, spotlightSetData->glowTextureResPath.c_str() );
		coneEffect->AddResourceTexture2D( textureMap, spotlightSetData->coneTextureResPath.c_str() );

		// parameters
		Tr2FloatParameterPtr zOffsetParam;
		zOffsetParam.CreateInstance();
		zOffsetParam->m_name = BlueSharedString( "zOffset" );
		zOffsetParam->m_value = spotlightSetData->zOffset;
		coneEffect->m_parameters.Append( zOffsetParam->GetRawRoot() );

		// that's it for setting up these shaders, must rebuild cache on it!
		coneEffect->EndUpdate();
		glowEffect->EndUpdate();

		// set to set
		spotlightSet->SetConeEffect( coneEffect );
		spotlightSet->SetGlowEffect( glowEffect );
		spotlightSet->SetSkinned( spotlightSetData->skinned );

		// add all individual items
		for( auto ssiit = spotlightSetData->items.begin(); ssiit != spotlightSetData->items.end(); ++ssiit )
		{
			// faction data?
			const EveSOFDataMgr::FactionSpotlightSetColorData* factionSpotlightData = dna->GetFactionSpotlightSetData( ssiit->groupIndex );
			if( !factionSpotlightData )
			{
				// This spotlight item is not used for this faction.
				continue;
			}

			// create it
			EveSpotlightSetItemPtr spotlightSetItem;
			spotlightSetItem.CreateInstance();

			// set it up the per-faction data
			spotlightSetItem->m_coneColor = ssiit->coneIntensity * factionSpotlightData->coneColor;
			spotlightSetItem->m_flareColor = ssiit->flareIntensity * factionSpotlightData->flareColor;
			spotlightSetItem->m_spriteColor = ssiit->spriteIntensity * factionSpotlightData->spriteColor;

			// set it up the per-hull data
			spotlightSetItem->m_boneIndex = ssiit->boneIndex;
			spotlightSetItem->m_boosterGainInfluence = ssiit->boosterGainInfluence;
			spotlightSetItem->m_spriteScale = ssiit->spriteScale;
			spotlightSetItem->m_transform = ssiit->transform;

			// add it
			spotlightSet->AddSpotlightItem( spotlightSetItem );
		}
		// spotlightset needs internal rebuild
		spotlightSet->Rebuild();
		// add to ship
		obj->AddSpotlightSet( spotlightSet );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   This is where it is all going to happen
// --------------------------------------------------------------------------------
void EveSOF::SetupPlaneSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// cycle over all spritesets of this hull
	const std::vector<EveSOFDataMgr::HullPlaneSetData>& hullPlaneSets = dna->GetHullPlaneSets();
	for( auto psit = hullPlaneSets.begin(); psit != hullPlaneSets.end(); ++psit )
	{
		const EveSOFDataMgr::HullPlaneSetData* planeSetData = &(*psit);

		// create a planeset for this ship
		EvePlaneSetPtr planeSet;
		planeSet.CreateInstance();

		// create shader
		Tr2EffectPtr planeEffect;
		planeEffect.CreateInstance();
		planeEffect->StartUpdate();

		// Select the planeset's data based on the usage
		std::string effectResPath, imageMapResPath, externalParamName;
		switch (planeSetData->usage)
		{
		case EveSOFDataHullPlaneSet::USAGE_STANDARD:
			effectResPath = planeSetData->skinned ? "res:/graphics/effect/managed/space/spaceobject/fx/skinned_planeglow.fx" : "res:/graphics/effect/managed/space/spaceobject/fx/planeglow.fx";
			break;
		case EveSOFDataHullPlaneSet::USAGE_VIDEO:
			effectResPath = planeSetData->skinned ? "res:/graphics/effect/managed/space/spaceobject/fx/skinned_planehologram.fx" : "res:/graphics/effect/managed/space/spaceobject/fx/planehologram.fx";
			imageMapResPath = "dynamic:/hangarvideos";
			break;
		case EveSOFDataHullPlaneSet::USAGE_ALLIANCE_LOGO:
			effectResPath = planeSetData->skinned ? "res:/graphics/effect/managed/space/spaceobject/fx/skinned_planehologram.fx" : "res:/graphics/effect/managed/space/spaceobject/fx/planehologram.fx";
			externalParamName = "AllianceLogoResPath";
			imageMapResPath = m_dataMgr.GetGenericData()->resPathDefaultAlliance;
			break;
		case EveSOFDataHullPlaneSet::USAGE_CORP_LOGO:
			effectResPath = planeSetData->skinned ? "res:/graphics/effect/managed/space/spaceobject/fx/skinned_planehologram.fx" : "res:/graphics/effect/managed/space/spaceobject/fx/planehologram.fx";
			externalParamName = "CorpLogoResPath";
			imageMapResPath = m_dataMgr.GetGenericData()->resPathDefaultCorp;
			break;
		case EveSOFDataHullPlaneSet::USAGE_CEO_PORTRAIT:
			effectResPath = planeSetData->skinned ? "res:/graphics/effect/managed/space/spaceobject/fx/skinned_planehologram.fx" : "res:/graphics/effect/managed/space/spaceobject/fx/planehologram.fx";
			externalParamName = "CeoPortraitResPath";
			imageMapResPath = m_dataMgr.GetGenericData()->resPathDefaultCeo;
			break;
		}

		planeEffect->SetEffectPathName( effectResPath.c_str() );

		// textures
		planeEffect->AddResourceTexture2D( BlueSharedString("Layer1Map"), planeSetData->layer1MapResPath.c_str() );
		planeEffect->AddResourceTexture2D( BlueSharedString("Layer2Map"), planeSetData->layer2MapResPath.c_str() );
		planeEffect->AddResourceTexture2D( BlueSharedString("MaskMap"), planeSetData->maskMapResPath.c_str() );
		
		// Imagemap texture?
		if( !imageMapResPath.empty() || !externalParamName.empty() )
		{
			planeEffect->AddResourceTexture2D( BlueSharedString("ImageMap"), imageMapResPath.c_str() );

			if( !externalParamName.empty() )
			{
				ITriEffectParameter* imageMapParameter = planeEffect->GetParameterByName( "ImageMap" );
				Tr2ExternalParameterPtr externalParameter;
				externalParameter.CreateInstance();
				externalParameter->SetName( externalParamName );
				externalParameter->SetDestinationObject( imageMapParameter );
				externalParameter->SetDestinationAttribute( "resourcePath" );
				externalParameter->Initialize();
				obj->AddExternalParameter( externalParameter );
			}
		}
				
		// parameters
		planeEffect->AddParameterVector4( BlueSharedString("PlaneData"), &planeSetData->planeData );

		// finish up shader and set it
		planeEffect->EndUpdate();
		planeSet->SetEffect( planeEffect );

		// add all individual items
		for( auto psiit = planeSetData->items.begin(); psiit != planeSetData->items.end(); ++psiit )
		{
			// create it
			EvePlaneSetItemPtr planeSetItem;
			planeSetItem.CreateInstance();
			// fill it up
			planeSetItem->m_position = psiit->position;
			planeSetItem->m_rotation = psiit->rotation;
			planeSetItem->m_scaling = psiit->scaling;
			planeSetItem->m_color = psiit->color;
			planeSetItem->m_layer1Transform = psiit->layer1Transform;
			planeSetItem->m_layer1Scroll = psiit->layer1Scroll;
			planeSetItem->m_layer2Transform = psiit->layer2Transform;
			planeSetItem->m_layer2Scroll = psiit->layer2Scroll;
			planeSetItem->m_boneIndex = psiit->boneIndex;
			planeSetItem->m_maskAtlasID = psiit->maskMapAtlasIndex;

			// groupindex allows to overwrite color
			const EveSOFDataMgr::FactionPlaneSetColorData* factionalData = dna->GetFactionPlaneSetData( psiit->groupIndex );
			if( factionalData )
			{
				planeSetItem->m_color = factionalData->color;
			}

			// add it
			planeSet->AddPlaneItem( planeSetItem );
		}
		// rebuild it internally
		planeSet->Rebuild();
		// add to ship
		obj->AddPlaneSet( planeSet );
	}

}

// --------------------------------------------------------------------------------
// Description:
//   This is where it is all going to happen
// --------------------------------------------------------------------------------
void EveSOF::SetupSpriteLineSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// cycle over all spritelinesets of this hull
	const std::vector<EveSOFDataMgr::HullSpriteLineSetData>& hullSpriteLineSets = dna->GetHullSpriteLineSets();
	for( auto slsit = hullSpriteLineSets.begin(); slsit != hullSpriteLineSets.end(); ++slsit )
	{
		const EveSOFDataMgr::HullSpriteLineSetData* spriteLineSetData = &( *slsit );

		// create a spriteset for this ship
		EveSpriteLineSetPtr spriteLineSet;
		spriteLineSet.CreateInstance();
		// set shader
		spriteLineSet->Setup( m_spriteSetEffect, spriteLineSetData->skinned );
		// add all the individual items
		for( auto slsiit = spriteLineSetData->items.begin(); slsiit != spriteLineSetData->items.end(); ++slsiit )
		{
			const EveSOFDataMgr::HullSpriteLineSetItemData* itemData = &( *slsiit );

			// faction data?
			const EveSOFDataMgr::FactionSpriteSetColorData* factionSpriteData = dna->GetFactionSpriteSetData( itemData->groupIndex );
			if( !factionSpriteData )
			{
				// This spritelineset item is not used for this faction.
				continue;
			}

			// create spritelineset items
			EveSpriteLineSetItemPtr spriteLineSetItem;
			spriteLineSetItem.CreateInstance();

			// set it up the per-faction data
			spriteLineSetItem->m_color = factionSpriteData->color;

			// set it up the per-hull data
			spriteLineSetItem->m_blinkPhaseShift = itemData->blinkPhaseShift;
			spriteLineSetItem->m_blinkPhase = itemData->blinkPhase;
			spriteLineSetItem->m_blinkRate = itemData->blinkRate;
			spriteLineSetItem->m_boneIndex = itemData->boneIndex;
			spriteLineSetItem->m_falloff = itemData->falloff;
			spriteLineSetItem->m_maxScale = itemData->maxScale;
			spriteLineSetItem->m_minScale = itemData->minScale;
			spriteLineSetItem->m_position = itemData->position;
			spriteLineSetItem->m_rotation = itemData->rotation;
			spriteLineSetItem->m_scaling = itemData->scaling;
			spriteLineSetItem->m_spacing = itemData->spacing;
			spriteLineSetItem->m_isCircle = itemData->isCircle;

			// put it into spriteset
			spriteLineSet->Add( spriteLineSetItem );
		}
		// spriteset needs internal rebuild
		spriteLineSet->Rebuild();
		// put set onto ship
		obj->AddSpriteLineSet( spriteLineSet );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Load Model Curves for rotation and translation, if there are any
// --------------------------------------------------------------------------------
void EveSOF::SetupModelCurves( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Model rotation curve
	const char* rotationCurvePath = dna->GetModelRotationCurvePath();
	if( rotationCurvePath )
	{
		IRootPtr p;
		IRoot* tmp = BeResMan->LoadObject( rotationCurvePath );
		if( tmp )
		{
			p.Attach( tmp );
			ITriQuaternionFunctionPtr rotationCurve;
			if( p->QueryInterface( BlueInterfaceIID<ITriQuaternionFunction>(), (void**)&rotationCurve ) )
			{
				obj->SetModelRotationCurve( rotationCurve );
			}
		}
	}

	// Model translation curve
	const char* translationCurvePath = dna->GetModelTranslationCurvePath();
	if( translationCurvePath )
	{
		IRootPtr p;
		IRoot* tmp = BeResMan->LoadObject( translationCurvePath );
		if( tmp )
		{
			p.Attach( tmp );
			ITriVectorFunctionPtr translationCurve;
			if( p->QueryInterface( BlueInterfaceIID<ITriVectorFunction>(), (void**)&translationCurve ) )
			{
				obj->SetModelTranslationCurve( translationCurve );
			}
		}
	}
}

void RecursiveBindParticleEmitters( EveTransformPtr transform, TriCurveSetPtr curveSet, Tr2ScalarCurvePtr curve )
{
	for( auto emitterIt = transform->m_particleEmitters.begin(); emitterIt != transform->m_particleEmitters.end(); ++emitterIt )
	{
		Tr2DynamicEmitterPtr dynamicEmitter;
		if( !(*emitterIt)->QueryInterface( BlueInterfaceIID<Tr2DynamicEmitter>(), (void**)&dynamicEmitter ) )
		{
			continue;
		}

		TriValueBindingPtr binding;
		binding.CreateInstance();
		binding->SetSource( "currentValue", curve->GetRawRoot() );
		binding->SetDestination( "rate", dynamicEmitter->GetRawRoot() );
		binding->Initialize();
		curveSet->AddBinding( (ITr2ValueBindingPtr)binding );
	}

	for( auto childIt = transform->m_children.begin(); childIt != transform->m_children.end(); ++childIt )
	{
		EveTransformPtr childTransform;
		if( !(*childIt)->QueryInterface( BlueInterfaceIID<EveTransform>(), (void**)&childTransform ) )
		{
			continue;
		}
		RecursiveBindParticleEmitters( childTransform, curveSet, curve );
	}
}

void RecursiveBindParticleEmitters( IEveSpaceObjectChild* child, TriCurveSetPtr curveSet, Tr2ScalarCurvePtr curve )
{
	if( EveChildContainerPtr container = BlueCastPtr( child ) )
	{
		for( auto childIt = container->m_objects.begin(); childIt != container->m_objects.end(); ++childIt )
		{
			RecursiveBindParticleEmitters( *childIt, curveSet, curve );
		}
	}
	else if( EveChildParticleSystemPtr psys = BlueCastPtr( child ) )
	{
		for( auto emitterIt = psys->m_particleEmitters.begin(); emitterIt != psys->m_particleEmitters.end(); ++emitterIt )
		{
			if( Tr2DynamicEmitterPtr dynamicEmitter = BlueCastPtr( *emitterIt ) )
			{
				TriValueBindingPtr binding;
				binding.CreateInstance();
				binding->SetSource( "currentValue", curve->GetRawRoot() );
				binding->SetDestination( "rate", dynamicEmitter->GetRawRoot() );
				binding->Initialize();
				curveSet->AddBinding( (ITr2ValueBindingPtr)binding );
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Add Children and Animations to the ship
//   
// --------------------------------------------------------------------------------
void EveSOF::SetupChildrenAndAnimations( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	std::map<int, std::vector<EveTransformPtr>> childrenToBindTo;
	std::map<int, std::vector<IEveSpaceObjectChildPtr>> soChildrenToBindTo;

	const std::vector<EveSOFDataMgr::HullChild>& hullChildren = dna->GetHullChildren();
	for( auto childIt = hullChildren.begin(); childIt != hullChildren.end(); ++childIt )
	{
		const EveSOFDataMgr::FactionChildData* fcd = dna->GetFactionChildData( childIt->groupIndex );

		// child can be invisibe for this faction
		if( fcd && !fcd->isVisible )
		{
			continue;
		}

		IRootPtr p;
		IRoot* tmp = BeResMan->LoadObject( childIt->redFilePath.c_str() );
		if( !tmp )
		{
			CCP_LOGERR( "resource file %s is invalid!", childIt->redFilePath.c_str() );
			continue;
		}
		p.Attach( tmp );

		// is it of right type?
		EveTransformPtr child;
		IEveSpaceObjectChildPtr effectChild;
		if( p->QueryInterface( BlueInterfaceIID<EveTransform>(), (void**)&child, BEQI_SILENT ) )
		{
			child->SetRotation( childIt->rotation );
			child->SetScaling( childIt->scaling );
			child->SetTranslation( childIt->translation );
			if( childIt->id != -1 )
			{
				childrenToBindTo[childIt->id].push_back( child );
			}

			obj->AddToChildrenList( child );
		}
		else if( p->QueryInterface( BlueInterfaceIID<IEveSpaceObjectChild>(), (void**)&effectChild, BEQI_SILENT ) )
		{
			effectChild->Setup( &childIt->scaling, &childIt->rotation, &childIt->translation, childIt->lowestLodVisible );
			obj->AddToEffectChildrenList( effectChild );
			if( childIt->id != -1 )
			{
				soChildrenToBindTo[childIt->id].push_back( effectChild );
			}
		}
		else
		{
			CCP_LOGERR( "resource file %s is not of correct type!", childIt->redFilePath.c_str() );
			return;
		}
	}


	const std::vector<EveSOFDataMgr::HullAnimation>& hullAnimations = dna->GetHullAnimations();
	for( auto animIt = hullAnimations.begin(); animIt != hullAnimations.end(); ++animIt )
	{
		TriCurveSetPtr curveSet;
		curveSet.CreateInstance();
		curveSet->SetName( animIt->name );

		// Do we control particle systems?
		if( animIt->id != -1 && animIt->startRate != -1.0 )
		{
			Tr2ScalarCurvePtr scalarCurve;
			scalarCurve.CreateInstance();

			scalarCurve->AddKey( 0.0f, animIt->startRate );
			scalarCurve->AddKey( 1.0f, animIt->endRate );

			std::vector<EveTransformPtr> transformVector = childrenToBindTo[animIt->id];
			for( auto transformIt = transformVector.begin(); transformIt != transformVector.end(); ++transformIt )
			{
				RecursiveBindParticleEmitters( (*transformIt), curveSet, scalarCurve );
			}
			std::vector<IEveSpaceObjectChildPtr> childVector = soChildrenToBindTo[animIt->id];
			for( auto childIt = childVector.begin(); childIt != childVector.end(); ++childIt )
			{
				RecursiveBindParticleEmitters( (*childIt), curveSet, scalarCurve );
			}

			curveSet->AddCurve( (ITriFunctionPtr)scalarCurve );
		}
		// Do we have valid rotation info?
		if( animIt->startRotationTime != -1.0 )
		{
			// Create the rotations curve
			Tr2QuaternionCurvePtr curve;
			curve.CreateInstance();
			curve->SetLength( animIt->endRotationTime );
			curve->SetEndValue( animIt->endRotationValue );
			curve->SetStartValue( animIt->startRotationValue );
			if( animIt->startRotationTime != 0.0 )
			{
				curve->SetStartValue( animIt->startRotationValue );
				curve->AddKey( animIt->startRotationTime, animIt->startRotationValue );
			}
			curveSet->AddCurve( (ITriFunctionPtr)curve );
			
			// Create the binding to the model rotation curve
			TriValueBindingPtr binding;
			binding.CreateInstance();
			binding->SetSource( "currentValue", curve->GetRawRoot() );
			if( !obj->GetModelRotationCurve() )
			{
				TriRotationCurvePtr modelRotationcurve;
				modelRotationcurve.CreateInstance();
				obj->SetModelRotationCurve( (ITriQuaternionFunctionPtr)modelRotationcurve );
			}
			binding->SetDestination( "value", obj->GetModelRotationCurve()->GetRootObject() );
			binding->Initialize();

			curveSet->AddBinding( (ITr2ValueBindingPtr)binding );
		}

		// Append the curveSet
		obj->AddCurveSet( curveSet );

		// Do we have valid translation info?
		if( animIt->startTranslationTime != -1.0 )
		{
			// TODO
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   add instanced meshes to the ship
// --------------------------------------------------------------------------------
void EveSOF::SetupInstancedMeshes( EveSpaceObject2Ptr newObj, const EveSOFDNAPtr dna ) const
{
	const std::vector<EveSOFDataMgr::HullInstancedMesh>& hullInstanced = dna->GetHullInstancedMeshes();
	for( auto instIt = hullInstanced.begin(); instIt != hullInstanced.end(); ++instIt )
	{
		const EveSOFDataMgr::HullInstancedMesh* him = &(*instIt);
		if( him->instances.empty() )
		{
			continue;
		}

		Tr2InstancedMeshPtr mesh;
		mesh.CreateInstance();

		Tr2VertexDefinition instanceDef;
		instanceDef.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 0 );
		instanceDef.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 1 );
		instanceDef.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 2 );
		instanceDef.Add( Tr2VertexDefinition::INT32_1, Tr2VertexDefinition::TEXCOORD, 3 );

		Tr2RuntimeInstanceDataPtr instanceData;
		instanceData.CreateInstance();
		instanceData->SetLayout( instanceDef );
		auto dest = instanceData->GetData( unsigned( him->instances.size() ) );
		memcpy( dest, &him->instances[0], sizeof( him->instances[0] ) * him->instances.size() );
		instanceData->UpdateData();
		mesh->SetInstanceGeometryRes( instanceData );

		mesh->SetMeshResPath( him->geometryResPath.c_str() );

		Tr2MeshAreaVector* areas = mesh->GetAreas( TRIBATCHTYPE_OPAQUE );

		// find data on this shader from generics, we need it!
		const EveSOFDataMgr::GenericShaderData* shaderData = dna->GetGenericAreaShaderData( him->shader );
		if( shaderData )
		{
			// every area has it's own shader, nothing we can share here
			Tr2EffectPtr newShader;
			newShader.CreateInstance();
			newShader->StartUpdate();

			// construct res path of the shader
			newShader->SetEffectPathName( dna->GetCompleteShaderPath( him->shader.c_str() ).c_str() );

			// parameters
			for( auto shaderParamIt = shaderData->parameters.begin(); shaderParamIt != shaderData->parameters.end(); ++shaderParamIt )
			{
				const Vector4* paramValue = dna->GetMeshAreaParameter( EveSOFDataArea::TYPE_PRIMARY, *shaderParamIt );
				if( paramValue )
				{
					newShader->AddParameterVector4( *shaderParamIt, paramValue );
				}
			}

			// shader textures from the hull data
			for( auto it = him->textures.begin(); it != him->textures.end(); ++it )
			{
				std::string resPath = it->second.resFilePath;
				dna->ModifyTextureResPath( resPath, it->first.c_str() );
				newShader->AddResourceTexture2D( it->first, resPath.c_str() );
			}

			// default shader textures from the generic data
			for( auto gtit = shaderData->defaultTextures.begin(); gtit != shaderData->defaultTextures.end(); ++gtit )
			{
				newShader->AddResourceTexture2D( gtit->first, gtit->second.resFilePath.c_str() );
			}

			// that's it for setting up this shader, must rebuild cache on it!
			newShader->EndUpdate();

			// new mesharea
			Tr2MeshAreaPtr newMeshArea;
			newMeshArea.CreateInstance();
			newMeshArea->SetName( "hull" );
			newMeshArea->SetMaterial( newShader );
			areas->Append( newMeshArea );
		}
		EveChildMeshPtr childMesh;
		childMesh.CreateInstance();
		childMesh->SetMesh( (Tr2MeshBase*)mesh );
		childMesh->Setup( nullptr, nullptr, nullptr, him->lowestLodVisible );
		newObj->AddToEffectChildrenList( (IEveSpaceObjectChild*)childMesh );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Add the ppt to the ship
// --------------------------------------------------------------------------------
void EveSOF::SetupCustomMask( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	size_t layerCount = dna->GetPatternLayerCount();
	for( size_t i = 0; i < layerCount; ++i )
	{
		const EveSOFDataMgr::PatternProjectionData* patternProjectionData = dna->GetPatternProjectionData( i );
		if( patternProjectionData )
		{
			if( patternProjectionData->enabled )
			{
				const EveSOFDataMgr::PatternLayerData* patternLayerData = dna->GetPatternLayerData( i );
				if( patternLayerData )
				{
					EveCustomMaskPtr customMask;
					customMask.CreateInstance();
					customMask->Setup( patternProjectionData->position, patternProjectionData->scaling, patternProjectionData->rotation, patternProjectionData->isMirrored, patternLayerData->materialSourceID, patternLayerData->materialTargets );
					obj->AddCustomMask( customMask );
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Add all kinds of effects to the ship
// --------------------------------------------------------------------------------
void EveSOF::SetupEffects( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	EveSOFDataHull::ImpactEffectType impactType = dna->GetImpactEffectType();
	if( impactType != EveSOFDataHull::IMPACTEFFECT_NONE )
	{
		const EveSOFDataMgr::GenericDamageData* genericDamageData = dna->GetGenericDamageData();
		const EveSOFDataMgr::GenericHullDamageData* genericHullDamageData = dna->GetGenericHullDamageData();
		const EveSOFDataMgr::RaceDamageData* raceDamageData = dna->GetRaceDamageData();
		if( genericDamageData && raceDamageData )
		{
			// create impact effect
			EveImpactOverlayPtr impactOverlay;
			impactOverlay.CreateInstance();

			// shield impact effect via Tr2Mesh with LOD
			Tr2MeshLodPtr shieldMesh;

			if( impactType == EveSOFDataHull::IMPACTEFFECT_ELLIPSOID )
			{
				// shield shader
				Tr2EffectPtr shieldShader;
				shieldShader.CreateInstance();
				shieldShader->StartUpdate();
				std::string shaderPath = dna->GetAreaShaderLocationResPath() + std::string( "/" ) + dna->GetImpactShieldShader();
				shieldShader->SetEffectPathName( shaderPath.c_str() );
				for( auto it = raceDamageData->shieldDamageParameters.begin(); it != raceDamageData->shieldDamageParameters.end(); ++it )
				{
					shieldShader->AddParameterVector4( it->first, &it->second );
				}
				for( auto it = raceDamageData->shieldDamageTextures.begin(); it != raceDamageData->shieldDamageTextures.end(); ++it )
				{
					shieldShader->AddResourceTexture2D( it->first, it->second.resFilePath.c_str() );
				}
				shieldShader->EndUpdate();
				Tr2MeshAreaPtr meshArea;
				meshArea.CreateInstance();
				meshArea->SetMaterial( shieldShader );

				// what type of shield effect determines the geometry resource
				Tr2LodResourcePtr lodResource;
				lodResource.CreateInstance();
				if( impactType == EveSOFDataHull::IMPACTEFFECT_ELLIPSOID )
				{
					// only the ellpisoid geometry
					lodResource->SetResourcePath( TR2_LOD_HIGH, genericDamageData->shieldGeometryResFilePath.c_str() );
				}
				else if( impactType == EveSOFDataHull::IMPACTEFFECT_HULL )
				{
					// use the ships main geometry incl LODs
					std::string highDetail = dna->GetHullGeometryResPath();
					std::string mediumDetail = highDetail;
					std::string lowDetail = highDetail;
					StringInsertStubBefore( mediumDetail, ".gr2", "_mediumDetail" );
					StringInsertStubBefore( lowDetail, ".gr2", "_lowDetail" );
					lodResource->SetResourcePath( TR2_LOD_LOW, lowDetail.c_str() );
					lodResource->SetResourcePath( TR2_LOD_MEDIUM, mediumDetail.c_str() );
					lodResource->SetResourcePath( TR2_LOD_HIGH, highDetail.c_str() );

					// adjust mesharea count to that from the mesh
					meshArea->SetCount( 1 + dna->GetHighestMeshAreaIndex( TRIBATCHTYPE_OPAQUE ) );
				}

				shieldMesh.CreateInstance();
				shieldMesh->SetGeometryResource( lodResource );
				shieldMesh->GetAreas( TRIBATCHTYPE_ADDITIVE )->Append( meshArea );
			}

			// armor damage impact via shader
			Tr2EffectPtr armorDamageShader;
			armorDamageShader.CreateInstance();
			armorDamageShader->StartUpdate();
			armorDamageShader->SetEffectPathName( dna->GetCompleteShaderPath( genericDamageData->armorShader.c_str() ).c_str() );
			for( auto it = raceDamageData->armorDamageParameters.begin(); it != raceDamageData->armorDamageParameters.end(); ++it )
			{
				armorDamageShader->AddParameterVector4( it->first, &it->second );
			}
			for( auto it = raceDamageData->armorDamageTextures.begin(); it != raceDamageData->armorDamageTextures.end(); ++it )
			{
				armorDamageShader->AddResourceTexture2D( it->first, it->second.resFilePath.c_str() );
			}
			armorDamageShader->EndUpdate();

			// armor damage impact via particlesystem
			Tr2GpuParticleSystem::Emitter psEmitter;
			memset( &psEmitter, 0, sizeof( Tr2GpuParticleSystem::Emitter ) );
			psEmitter.angle = genericDamageData->armorParticleAngle;
			psEmitter.minSpeed = genericDamageData->armorParticleMinMaxSpeed[0];
			psEmitter.maxSpeed = genericDamageData->armorParticleMinMaxSpeed[1];
			Tr2GpuParticleSystem::EmitterParams psParams;
			memset( &psParams, 0, sizeof( Tr2GpuParticleSystem::EmitterParams ) );
			psParams.minLifeTime = genericDamageData->armorParticleMinMaxLifeTime[0];
			psParams.maxLifeTime = genericDamageData->armorParticleMinMaxLifeTime[1];
			psParams.sizes = Vector3( genericDamageData->armorParticleSizes[0], genericDamageData->armorParticleSizes[1], genericDamageData->armorParticleSizes[2] );
			psParams.sizeVariance = genericDamageData->armorParticleSizes[3];
			memcpy( psParams.colors, genericDamageData->armorParticleColors, 4 * sizeof( Color ) );
			psParams.textureIndex = genericDamageData->armorParticleTextureIndex;
			psParams.velocityStretchRotation = genericDamageData->armorParticleVelocityStretchRotation;
			psParams.drag = genericDamageData->armorParticleDrag;
			psParams.turbulenceAmplitude = genericDamageData->armorParticleTurbulenceAmplitude;
			psParams.turbulenceFrequency = genericDamageData->armorParticleTurbulenceFrequency;
			psParams.colorMidpoint = genericDamageData->armorParticleColorMidPoint;
			Tr2GpuUniqueEmitterPtr impactEmitter;
			impactEmitter.CreateInstance();
			impactEmitter->Setup( genericDamageData->armorParticleRate, &psEmitter, &psParams );

			Tr2GpuUniqueEmitterPtr hullImpactEmitter = nullptr;
			if( genericHullDamageData )
			{
				// hull damage impact via particlesystem
				Tr2GpuParticleSystem::Emitter hullDamagePsEmitter;
				memset( &hullDamagePsEmitter, 0, sizeof( Tr2GpuParticleSystem::Emitter ) );
				hullDamagePsEmitter.angle = genericHullDamageData->hullParticleAngle;
				hullDamagePsEmitter.innerAngle = genericHullDamageData->hullParticleInnerAngle;
				hullDamagePsEmitter.minSpeed = genericHullDamageData->hullParticleMinMaxSpeed[0];
				hullDamagePsEmitter.maxSpeed = genericHullDamageData->hullParticleMinMaxSpeed[1];
				Tr2GpuParticleSystem::EmitterParams hullDamagePsParams;
				memset( &hullDamagePsParams, 0, sizeof( Tr2GpuParticleSystem::EmitterParams ) );
				hullDamagePsParams.minLifeTime = genericHullDamageData->hullParticleMinMaxLifeTime[0];
				hullDamagePsParams.maxLifeTime = genericHullDamageData->hullParticleMinMaxLifeTime[1];
				hullDamagePsParams.sizes = Vector3( genericHullDamageData->hullParticleSizes[0], genericHullDamageData->hullParticleSizes[1], genericHullDamageData->hullParticleSizes[2] );
				hullDamagePsParams.sizeVariance = genericHullDamageData->hullParticleSizes[3];
				memcpy( hullDamagePsParams.colors, genericHullDamageData->hullParticleColors, 4 * sizeof( Color ) );
				hullDamagePsParams.textureIndex = genericHullDamageData->hullParticleTextureIndex;
				hullDamagePsParams.velocityStretchRotation = genericHullDamageData->hullParticleVelocityStretchRotation;
				hullDamagePsParams.drag = genericHullDamageData->hullParticleDrag;
				hullDamagePsParams.turbulenceAmplitude = genericHullDamageData->hullParticleTurbulenceAmplitude;
				hullDamagePsParams.turbulenceFrequency = genericHullDamageData->hullParticleTurbulenceFrequency;
				hullDamagePsParams.colorMidpoint = genericHullDamageData->hullParticleColorMidpoint;
				
				hullImpactEmitter.CreateInstance();
				hullImpactEmitter->Setup( genericHullDamageData->hullParticleRate, &hullDamagePsEmitter, &hullDamagePsParams );
			}

			// hull damage flicker via perlin curve
			TriPerlinCurvePtr flickerCurve;
			flickerCurve.CreateInstance();
			flickerCurve->mAlpha = genericDamageData->flickerPerlinAlpha;
			flickerCurve->mBeta = genericDamageData->flickerPerlinBeta;
			flickerCurve->mN = genericDamageData->flickerPerlinN;
			flickerCurve->mSpeed = genericDamageData->flickerPerlinSpeed;
			// These parameters are dynamically set based on the hullDamageFactor in the impactOverlay
			flickerCurve->mOffset = 1.f;
			flickerCurve->mScale = 0.f;

			// setup the overlay effect and add it the object
			impactOverlay->Set( flickerCurve, impactEmitter, hullImpactEmitter, armorDamageShader, shieldMesh, impactType == EveSOFDataHull::IMPACTEFFECT_ELLIPSOID );
			obj->SetImpactOverlay( impactOverlay );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   add the booster to the new ship
// --------------------------------------------------------------------------------
void EveSOF::SetupBoosters( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// per-race data
	const EveSOFDataMgr::RaceBoosterData* rdata = dna->GetRaceBoosterData();
	// pre-hull data
	const EveSOFDataMgr::HullBoosterData* hdata = dna->GetHullBoosterData();

	// does this hull have boosters at all?
	if( hdata->items.empty() )
	{
		return;
	}

	// create
	EveBoosterSet2Ptr set;
	set.CreateInstance();

	// set the booster set's internal data
	set->SetData( 
		rdata->glowScale, 
		&rdata->glowColor, 
		&rdata->warpGlowColor,
		rdata->symHaloScale, 
		rdata->haloScaleX, 
		rdata->haloScaleY, 
		&rdata->haloColor, 
		&rdata->warpHaloColor,
		hdata->alwaysOn );
	set->SetLightData( rdata->lightOffset, rdata->lightFlickerAmplitude, rdata->lightFlickerFrequency, rdata->lightRadius, rdata->lightColor, rdata->lightWarpRadius, rdata->lightWarpColor );

	// create and setup booster effect
	Tr2EffectPtr effect;
	effect.CreateInstance();
	effect->StartUpdate();

	effect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/Booster/BoosterVolumetric.fx" );
	effect->AddParameterFloat( BlueSharedString("NoiseFunction0"), rdata->shape0.noiseFunction );
	effect->AddParameterFloat( BlueSharedString("NoiseSpeed0"), rdata->shape0.noiseSpeed );
	effect->AddParameterVector4( BlueSharedString("NoiseAmplitudeStart0"), &rdata->shape0.noiseAmplitureStart );
	effect->AddParameterVector4( BlueSharedString("NoiseAmplitudeEnd0"), &rdata->shape0.noiseAmplitureEnd );
	effect->AddParameterVector4( BlueSharedString("NoiseFrequency0"), &rdata->shape0.noiseFrequency );
	effect->AddParameterColor( BlueSharedString("Color0"), &rdata->shape0.color );
	effect->AddParameterFloat( BlueSharedString("NoiseFunction1"), rdata->shape1.noiseFunction );
	effect->AddParameterFloat( BlueSharedString("NoiseSpeed1"), rdata->shape1.noiseSpeed );
	effect->AddParameterVector4( BlueSharedString("NoiseAmplitudeStart1"), &rdata->shape1.noiseAmplitureStart );
	effect->AddParameterVector4( BlueSharedString("NoiseAmplitudeEnd1"), &rdata->shape1.noiseAmplitureEnd );
	effect->AddParameterVector4( BlueSharedString("NoiseFrequency1"), &rdata->shape1.noiseFrequency );
	effect->AddParameterColor( BlueSharedString("Color1"), &rdata->shape1.color );

	effect->AddParameterFloat( BlueSharedString("WarpNoiseFunction0"), rdata->warpShape0.noiseFunction );
	effect->AddParameterFloat( BlueSharedString("WarpNoiseSpeed0"), rdata->warpShape0.noiseSpeed );
	effect->AddParameterVector4( BlueSharedString("WarpNoiseAmplitudeStart0"), &rdata->warpShape0.noiseAmplitureStart );
	effect->AddParameterVector4( BlueSharedString("WarpNoiseAmplitudeEnd0"), &rdata->warpShape0.noiseAmplitureEnd );
	effect->AddParameterVector4( BlueSharedString("WarpNoiseFrequency0"), &rdata->warpShape0.noiseFrequency );
	effect->AddParameterColor( BlueSharedString("WarpColor0"), &rdata->warpShape0.color );
	effect->AddParameterFloat( BlueSharedString("WarpNoiseFunction1"), rdata->warpShape1.noiseFunction );
	effect->AddParameterFloat( BlueSharedString("WarpNoiseSpeed1"), rdata->warpShape1.noiseSpeed );
	effect->AddParameterVector4( BlueSharedString("WarpNoiseAmplitudeStart1"), &rdata->warpShape1.noiseAmplitureStart );
	effect->AddParameterVector4( BlueSharedString("WarpNoiseAmplitudeEnd1"), &rdata->warpShape1.noiseAmplitureEnd );
	effect->AddParameterVector4( BlueSharedString("WarpNoiseFrequency1"), &rdata->warpShape1.noiseFrequency );
	effect->AddParameterColor( BlueSharedString("WarpColor1"), &rdata->warpShape1.color );

	Vector4 shapeAtlasSize( float( rdata->shapeAtlasHeight ), float( rdata->shapeAtlasCount ), 0, 0 );
	effect->AddParameterVector4( BlueSharedString("ShapeAtlasSize"), &shapeAtlasSize );
	effect->AddParameterVector4( BlueSharedString("BoosterScale"), &rdata->scale );

	effect->AddResourceTexture2D( BlueSharedString("ShapeMap"), rdata->shapeAtlasResPath.c_str() );
	effect->AddResourceTexture2D( BlueSharedString("GradientMap0"), rdata->gradient0ResPath.c_str() );
	effect->AddResourceTexture2D( BlueSharedString("GradientMap1"), rdata->gradient1ResPath.c_str() );
	effect->AddResourceTexture2D( BlueSharedString("NoiseMap"), "res:/Texture/Global/noise32cube_volume.dds" );

	// finish effect and set it
	effect->EndUpdate();
	set->SetEffect( effect );

	// create and setup glows
	EveSpriteSetPtr glow;
	glow.CreateInstance();
	Tr2EffectPtr glowEffect;
	glowEffect.CreateInstance();
	glowEffect->StartUpdate();
	if( rdata->volumetric )
	{
		glowEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/Booster/BoosterGlowAnimated.fx" );
		glowEffect->AddResourceTexture2D( BlueSharedString("NoiseMap"), "res:/Texture/global/noise.dds" );
	}
	else
	{
		glowEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/Booster/BoosterGlowPool.fx" );
	}
	glowEffect->AddResourceTexture2D( BlueSharedString("DiffuseMap"), "res:/Texture/Particle/whitesharp.dds" );
	// finish effect and set it
	glowEffect->EndUpdate();
	glow->SetEffect( glowEffect );
	set->SetGlow( glow );

	if( hdata->hasTrails )
	{
		Tr2EffectPtr trailEffect;
		trailEffect.CreateInstance();
		trailEffect->StartUpdate();
		EveTrailsSetPtr trail;
		trail.CreateInstance();
		trailEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/Booster/VolumetricTrails.fx" );
		trailEffect->AddParameterVector4( BlueSharedString("TrailSize"), &rdata->trailSize );
		trailEffect->AddParameterColor( BlueSharedString("TrailColor"), &rdata->trailColor );
		trailEffect->AddParameterFloat( BlueSharedString( "TrailFadeIn" ), rdata->volumetric ? 0.15f : 0.0f );
		trailEffect->EndUpdate();
		trail->SetEffect( trailEffect );
		trail->SetMeshResPath( "res:/dx9/model/ship/booster/volumetricTrail.gr2" );
		set->SetTrail( trail );
	}

	// add all the indiviual items
	int index = 0;
	for( auto biit = hdata->items.begin(); biit != hdata->items.end(); ++biit )
	{
		EveLocator2Ptr locator;
		locator.CreateInstance();
		char name[128];
		sprintf_s( name, "locator_booster_%i", ++index );
		locator->SetName( name );
		locator->SetTransform( biit->transform );
		ship->AddLocator( locator );
		set->Add( &biit->transform, &biit->functionality, biit->hasTrail, biit->atlasIndex0, biit->atlasIndex1 );
	}
	glow->Rebuild();

	// add it to ship
	set->PrepareResources();
	ship->SetBoosterSet( set );
}

// --------------------------------------------------------------------------------
// Description:
//   add the hull decals to the new ship
// --------------------------------------------------------------------------------
void EveSOF::SetupDecals( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// create and setup all hull decals
	const std::vector<EveSOFDataMgr::HullDecalData>& hullDecals = dna->GetHullDecals();
	for( auto hdit = hullDecals.begin(); hdit != hullDecals.end(); ++hdit )
	{
		const EveSOFDataMgr::FactionDecalData* fdd = nullptr;

		// do we have faction data for this decal? comes from groupIndex
		if( hdit->groupIndex != -1 )
		{
			fdd = dna->GetFactionDecalData( hdit->groupIndex );
			if( !( fdd && fdd->isVisible ) )
			{
				continue;
			}
		}

		// create
		EveSpaceObjectDecalPtr decal;
		decal.CreateInstance();
		// set general datas
		decal->SetPosition( hdit->position );
		decal->SetRotation( hdit->rotation );
		decal->SetScaling( hdit->scaling );
		decal->SetBoneIndex( hdit->boneIndex );
		if( hdit->indexBuffer.empty() )
		{
			decal->SetIndices( nullptr, 0 );
		}
		else
		{
			decal->SetIndices( &hdit->indexBuffer[0], hdit->indexBuffer.size() );
		}

		// the decal effect
		Tr2EffectPtr shader;
		shader.CreateInstance();
		shader->StartUpdate();

		// shader is hull-only and MUST exist!
		if( hdit->shader.empty() )
		{
			continue;
		}

		// construct shader path and set it on the Tr2Effect
		std::string shaderPath = dna->GetDecalShaderLocationResPath() + std::string("/") + dna->GetShaderPrefix( false ) + hdit->shader;
		shader->SetEffectPathName( shaderPath.c_str() );

		// always set hull parameters & textures for this decal
		for( auto hdpit = hdit->parameters.begin(); hdpit != hdit->parameters.end(); ++hdpit )
		{
			shader->AddParameterVector4( hdpit->first, &hdpit->second );
		}
		for( auto hdtit = hdit->textures.begin(); hdtit != hdit->textures.end(); ++hdtit )
		{
			shader->AddResourceTexture2D( hdtit->first, hdtit->second.resFilePath.c_str() );
		}

		// then set the factional
		if( fdd )
		{
			for( auto fdpit = fdd->parameters.begin(); fdpit != fdd->parameters.end(); ++fdpit )
			{
				shader->AddParameterVector4( fdpit->first, &fdpit->second );
			}
			for( auto fdtit = fdd->textures.begin(); fdtit != fdd->textures.end(); ++fdtit )
			{
				shader->AddResourceTexture2D( fdtit->first, fdtit->second.resFilePath.c_str() );
			}
		}

		// find data on this shader from generics, we need it!
		const EveSOFDataMgr::GenericShaderData* shaderData = dna->GetGenericDecalShaderData( BlueSharedString( hdit->shader ) );
		if( shaderData )
		{
			// default shader textures from the generic data
			for( auto gtit = shaderData->defaultTextures.begin(); gtit != shaderData->defaultTextures.end(); ++gtit )
			{
				shader->AddResourceTexture2D( gtit->first, gtit->second.resFilePath.c_str() );
			}
			for( auto gpit = shaderData->defaultParameters.begin(); gpit != shaderData->defaultParameters.end(); ++gpit )
			{
				shader->AddParameterVector4( gpit->first, &gpit->second );
			}
		}

		// init and add
		shader->EndUpdate();
		decal->SetEffect( shader );
		decal->Initialize();
		obj->AddDecal( decal );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   add the hull locators to the new ship
// --------------------------------------------------------------------------------
void EveSOF::SetupLocators( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// create and setup all turret locators
	const std::vector<EveSOFDataMgr::LocatorData>& turretLocators = dna->GetHullTurretLocators();
	for( auto tlit = turretLocators.begin(); tlit != turretLocators.end(); ++tlit )
	{
		// create a new locator
		EveLocator2Ptr loc;
		loc.CreateInstance();

		// set it up
		loc->SetName( tlit->name );
		loc->SetTransform( tlit->transform );

		// add it to the new ship
		obj->AddLocator( loc );
	}

	// set whole block of damage locators, they are a structered list
	const std::vector<EveSOFDataMgr::LocatorDirectionData>* damageLocators = dna->GetHullLocators( "damage" );
	if( damageLocators )
	{
		obj->SetDamageLocators( (const EveDamageLocator*)&(*damageLocators)[0], damageLocators->size() );
	}

	// set a special block of optional locators (this will be more generic when we have moved over the damage locators!!!)
	const std::vector<EveSOFDataMgr::LocatorDirectionData>* pdbLocators = dna->GetHullLocators( "defensebattery" );
	if( pdbLocators )
	{
		obj->AddLocatorSet( "defensebattery", (const Locator*)&( *pdbLocators )[0], pdbLocators->size() );
	}
	const std::vector<EveSOFDataMgr::LocatorDirectionData>* explosionsLocators = dna->GetHullLocators( "explosions" );
	if( explosionsLocators )
	{
		obj->AddLocatorSet( "explosions", (const Locator*)&( *explosionsLocators )[0], explosionsLocators->size() );
	}
	const std::vector<EveSOFDataMgr::LocatorDirectionData>* globalExplosionOffsetLocators = dna->GetHullLocators( "globalExplosionOffset" );
	if( globalExplosionOffsetLocators )
	{
		obj->AddLocatorSet( "globalExplosionOffset", (const Locator*)&( *globalExplosionOffsetLocators )[0], globalExplosionOffsetLocators->size() );
	}

	// create and setup the audio locator
	EveLocator2Ptr loc;
	loc.CreateInstance();
	loc->SetName( "locator_audio_booster" );
	const Vector3* pos = dna->GetHullAudioPosition();
	loc->SetTransform( Matrix( 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, pos->x, pos->y, pos->z, 1.f ) );

	// add it to the new ship
	obj->AddLocator( loc );
}

// --------------------------------------------------------------------------------
// Description:
//   setup the material of a turret with the data from faction
// --------------------------------------------------------------------------------
void EveSOF::SetupTurretMaterialFromFaction( EveTurretSet* turretSet, const char* factionName )
{
	// get generic data
	const EveSOFDataMgr::GenericData* genericData = m_dataMgr.GetGenericData();
	// get faction data
	const EveSOFDataMgr::FactionData* factionData = m_dataMgr.GetFactionData( factionName );
	// get area data
	const EveSOFDataMgr::AreaMaterialData* areaMaterialData = &factionData->areaMaterials;

	// start modifying the parameters of the turret's shader
	Tr2Effect* shader = turretSet->GetShader();
	if( shader )
	{
		// try override shader's parameter, const's first
		if( !shader->m_constParameters.empty() )
		{
			shader->StartUpdate();
			for( auto it = shader->m_constParameters.begin(); it != shader->m_constParameters.end(); ++it )
			{
				// build the parameter
				EveSOFUtilsParameterName param( genericData->materialPrefixes, it->name.c_str() );
				if( param.IsMaterialIdxValid() )
				{
					param.ChangeMaterialIdx( genericData, factionData->materialUsageList[param.GetMaterialIdx()] );
				}
				// find data
				const Vector4* res = EveSOFUtils::SearchForParameterData( &m_dataMgr, areaMaterialData, EveSOFDataArea::TYPE_PRIMARY, &param );
				if( res )
				{
					it->value = *res;
				}
			}
			shader->EndUpdate();
		}
		else
		{
			// then non-const parameters
			for( auto it = shader->m_parameters.begin(); it != shader->m_parameters.end(); ++it )
			{
				// build the parameter
				EveSOFUtilsParameterName param( genericData->materialPrefixes, ( *it )->GetParameterName() );
				if( param.IsMaterialIdxValid() )
				{
					param.ChangeMaterialIdx( genericData, factionData->materialUsageList[param.GetMaterialIdx()] );
				}
				// find data
				const Vector4* res = EveSOFUtils::SearchForParameterData( &m_dataMgr, areaMaterialData, EveSOFDataArea::TYPE_PRIMARY, &param );
				if( res )
				{
					Tr2Vector4ParameterPtr p;
					if( ( *it )->QueryInterface( BlueInterfaceIID<Tr2Vector4Parameter>(), (void**)&p, BEQI_SILENT ) )
					{
						p->SetValue( *res );
					}
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   setup the material of a turret with the data from SOF faction
// --------------------------------------------------------------------------------
void EveSOF::SetupTurretMaterialFromDNA( EveTurretSet* turretSet, const char* dnaString )
{
	// get parent ship's DNA
	EveSOFDNAPtr dna;
	dna.CreateInstance();
	dna->Setup( dnaString, &m_dataMgr );
	if( !dna->IsValid() )
	{
		CCP_LOGERR( "EveSOF: SetupTurretMaterialFromDNA failed, wrong dna string %s!", dnaString );
		return;
	}

	// start modifying the parameters of the turret's shader
	Tr2Effect* shader = turretSet->GetShader();
	if( shader )
	{
		// try override shader's parameter, const's first
		if( !shader->m_constParameters.empty() )
		{
			shader->StartUpdate();
			for( auto it = shader->m_constParameters.begin(); it != shader->m_constParameters.end(); ++it )
			{
				const Vector4* parentValue = dna->GetFactionTurretParameters( it->name );
				if( parentValue )
				{
					it->value = *parentValue;
				}
			}
			shader->EndUpdate();
		}
		else
		{
			// then non-const parameters
			for( auto it = shader->m_parameters.begin(); it != shader->m_parameters.end(); ++it )
			{
				const Vector4* parentValue = dna->GetFactionTurretParameters( BlueSharedString( (*it)->GetParameterName() ) );
				if( parentValue )
				{
					Tr2Vector4ParameterPtr param;
					if( (*it)->QueryInterface( BlueInterfaceIID<Tr2Vector4Parameter>(), (void**)&param, BEQI_SILENT ) )
					{
						param->SetValue( *parentValue );
					}
				}
			}
		}
	}
}











