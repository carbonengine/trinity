////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "Utilities/StringUtils.h"
#include "Include/TriMath.h"
#include "EveSOF.h"
#include "EveSOFDNA.h"
#include "EveSOFUtils.h"
#include "Tr2ExternalParameter.h"
#include "Audio/ITr2AudEmitter.h"
#include "Eve/EveTransform.h"
#include "Eve/Turret/EveTurretSet.h"
#include "Eve/SpaceObject/EveShip2.h"
#include "Eve/SpaceObject/EveStation2.h"
#include "Eve/SpaceObject/EveSwarm.h"
#include "Eve/SpaceObject/Utils/EveCustomMask.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteLineSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EveHazeSet.h"
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
#include "Curves/Tr2CurveQuaternion.h"
#include "Curves/Tr2CurveScalar.h"
#include "Curves/TriCurveSet.h"
#include "Curves/Tr2RotationAdapter.h"
#include "TriValueBinding.h"
#include "Particle/Tr2DynamicEmitter.h"
#include "Particle/Tr2GpuUniqueEmitter.h"
#include "TriSequencer.h"
#include "Lights/Tr2PointLight.h"
#include "Lights/Tr2TexturedPointLight.h"
#include "Lights/Tr2SpotLight.h"


namespace
{
	const float MIN_DECAL_SCREEN_SIZE = 10.f;
	const uint8_t PICKABLE_HANGARVIEO_BUFFER_ID = 100;
}


// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveSOF::EveSOF( IRoot* lockobj ) :
	PARENTLOCK( m_dataMgr )
{
	// hard-coded names
	m_depthOnlyEffectName = BlueSharedString( "depthonlyv5.fx" );
	m_decalsEffectName[EveSOFDataHullDecalSetItem::USAGE_STANDARD] = BlueSharedString( "decalv5.fx" );
	m_decalsEffectName[EveSOFDataHullDecalSetItem::USAGE_KILLCOUNTER] = BlueSharedString( "decalcounterv5.fx" );
	m_decalsEffectName[EveSOFDataHullDecalSetItem::USAGE_HOLE] = BlueSharedString( "decalholev5.fx" );
	m_decalsEffectName[EveSOFDataHullDecalSetItem::USAGE_CYLINDRICAL] = BlueSharedString( "decalcylindricv5.fx" );
	m_decalsEffectName[EveSOFDataHullDecalSetItem::USAGE_GLOWCYLINDRICAL] = BlueSharedString( "decalglowcylindricv5.fx" );
	m_decalsEffectName[EveSOFDataHullDecalSetItem::USAGE_GLOWSTANDARD] = BlueSharedString( "decalglowv5.fx" );
	m_decalsEffectName[EveSOFDataHullDecalSetItem::USAGE_LOGO] = BlueSharedString( "decalv5.fx" );

	// pre-register some really needed vars in the global variable store
	Tr2Variable var1( "DepthMap", (ITr2TextureProvider*)nullptr );
	Tr2Variable var2( "DepthMapMsaa", (ITr2TextureProvider*)nullptr );

	BlueSharedString gradientMap( "GradientMap" );

	// some shared shaders here
	m_spriteSetEffect.CreateInstance();
	m_spriteSetEffect->StartUpdate();
	m_spriteSetEffect->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/fx/blinkinglightspool.fx" );
	m_spriteSetEffect->EndUpdate();

	m_hazeSetEffectSpherical.CreateInstance();
	m_hazeSetEffectSpherical->StartUpdate();
	m_hazeSetEffectSpherical->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/fx/hazespherical.fx" );
	m_hazeSetEffectSpherical->EndUpdate();

	m_hazeSetEffectHalfSpherical.CreateInstance();
	m_hazeSetEffectHalfSpherical->StartUpdate();
	m_hazeSetEffectHalfSpherical->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/fx/hazehalfspherical.fx" );
	m_hazeSetEffectHalfSpherical->EndUpdate();

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

	InheritableTextures inheritableTextures;

	// get us the base geometry
	SetupMesh( newObj, dna, inheritableTextures );
	SetupCustomMask( newObj, dna );

	// decals
	SetupDecalSets( newObj, dna, inheritableTextures );

	// effects on ships
	SetupSpriteSets( newObj, dna );
	SetupSpotlightSets( newObj, dna );
	SetupPlaneSets( newObj, dna );
	SetupSpriteLineSets( newObj, dna );
	SetupHazeSets( newObj, dna );
	SetupBannerSets( newObj, dna );
	SetupEffects( newObj, dna );
	SetupLights( newObj, dna );

	// attachments to ship
	SetupLocators( newObj, dna );

	
	if( !dna->IsUsingExperimentalFeatures() )
	{
		// children, animations and particles
		SetupChildrenAndAnimations( newObj, dna );	
	}
	else
	{
		// children from childsets
		SetupEffectChildren( newObj, dna );
	}

	SetupAudio( newObj, dna );
	SetupControllers( newObj, dna );

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
void EveSOF::SetupMesh( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna, InheritableTextures& inheritableTextures ) const
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

	// shadow caster?
	if( dna->CastShadow() )
	{
		if( dna->IsHullAnimated() )
		{
			obj->SetShadowEffect( m_shadowEffectSkinned );
		}
		else
		{
			obj->SetShadowEffect( m_shadowEffect );
		}
	}

	// setup mesh areas, try sharing as many Tr2LodResources as possible
	std::map<std::string, Tr2LodResourcePtr> lodResPerTexture;

	// multi-hull! so mesh index must be tracked
	size_t meshIndexOffset = 0;
	// cycle over all hulls in the multi-hull list
	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		size_t cntr = 0;
		cntr += FillMeshAreaVector( lodResPerTexture, mesh->GetAreas( TRIBATCHTYPE_OPAQUE ), TRIBATCHTYPE_OPAQUE, dna, hullIdx, meshIndexOffset, &inheritableTextures );
		cntr += FillMeshAreaVector( lodResPerTexture, mesh->GetAreas( TRIBATCHTYPE_OPAQUE ), TRIBATCHTYPE_DECAL, dna, hullIdx, meshIndexOffset, nullptr );
		cntr += FillMeshAreaVector( lodResPerTexture, mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT ), TRIBATCHTYPE_TRANSPARENT, dna, hullIdx, meshIndexOffset, nullptr );
		cntr += FillMeshAreaVector( lodResPerTexture, mesh->GetAreas( TRIBATCHTYPE_ADDITIVE ), TRIBATCHTYPE_ADDITIVE, dna, hullIdx, meshIndexOffset, nullptr );
		cntr += FillMeshAreaVector( lodResPerTexture, mesh->GetAreas( TRIBATCHTYPE_DISTORTION ), TRIBATCHTYPE_DISTORTION, dna, hullIdx, meshIndexOffset, nullptr );

		auto distortionAreas = mesh->GetAreas( TRIBATCHTYPE_DISTORTION );
		for( auto ait = begin( *distortionAreas ); ait != end( *distortionAreas ); ++ait )
		{
			( *ait )->SetMinLod( TR2_LOD_HIGH );
		}
		meshIndexOffset += cntr;
	}

	// some areas need an accompanying depth area
	GenerateDepthFromAreaVector( mesh, mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT ), dna );

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
void EveSOF::GenerateDepthFromAreaVector( Tr2MeshLodPtr mesh, const Tr2MeshAreaVector* meshAreaVector, const EveSOFDNAPtr dna ) const
{
	Tr2MeshAreaVector* depthAreas = mesh->GetAreas( TRIBATCHTYPE_DEPTH );
	if( depthAreas ) 
	{
		for( auto srcMeshArea = meshAreaVector->begin(); srcMeshArea != meshAreaVector->end(); ++srcMeshArea )
		{
			if( (*srcMeshArea)->GetGenerateDepthArea() )
			{
				Tr2MeshAreaPtr destMeshArea;
				BeClasses->CopyTo( *srcMeshArea, (IRoot**)&destMeshArea );

				Tr2Effect* destShader = dynamic_cast<Tr2Effect*>(destMeshArea->GetMaterialInterface());
				Tr2Effect* srcShader = dynamic_cast<Tr2Effect*>((*srcMeshArea)->GetMaterialInterface());
				if( destShader && srcShader )
				{
					const EveSOFDataMgr::GenericShaderData* depthOnlyShaderData = dna->GetGenericAreaShaderData( m_depthOnlyEffectName );
					if( depthOnlyShaderData )
					{
						destShader->SetEffectPathName( dna->GetCompleteShaderPath( m_depthOnlyEffectName.c_str() ).c_str() );
						destShader->ClearAllParameters();
						destShader->ClearAllResources();

						ITriEffectParameter* p = srcShader->GetResourceByName( depthOnlyShaderData->transparencyTextureName.c_str() );
						destShader->AddResource( p );
					}
				}
				depthAreas->Append( destMeshArea );
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Fill up mesh area vector given the hull and faction area data provided.
// --------------------------------------------------------------------------------
size_t EveSOF::FillMeshAreaVector( std::map<std::string, Tr2LodResourcePtr>& lodResCollector, Tr2MeshAreaVector* meshAreaVector, TriBatchType areaType, const EveSOFDNAPtr dna, size_t hullIdx, size_t meshIndexOffset, InheritableTextures* inheritableTextures ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	const std::vector<EveSOFDataMgr::HullAreas>* hullAreas = dna->GetHullMeshAreas( areaType, hullIdx );
	for( auto area = hullAreas->begin(); area != hullAreas->end(); ++area )
	{
		// find data on this shader from generics, we need it!
		const EveSOFDataMgr::GenericShaderData* shaderData = dna->GetGenericAreaShaderData( area->shader );
		if( !shaderData )
		{
			CCP_LOGERR( "EveSOF::FillMeshAreaVector: couldn't find generic info for shader %s", area->shader.c_str() );
			return 0;
		}

		// every area has it's own shader, nothing we can share here
		Tr2EffectPtr newShader;
		newShader.CreateInstance();
		newShader->StartUpdate();

		bool castsShadows = false;
		switch( areaType )
		{
		case TRIBATCHTYPE_DECAL:
			newShader->SetOption( BlueSharedString( "SPACE_OBJECT_TRANSPARENCY" ), BlueSharedString( "SOT_CLIP" ) );
			break;
		case TRIBATCHTYPE_TRANSPARENT:
			newShader->SetOption( BlueSharedString( "SPACE_OBJECT_TRANSPARENCY" ), BlueSharedString( "SOT_TRANSPARENT" ) );
			break;
		case TRIBATCHTYPE_OPAQUE:
			newShader->SetOption( BlueSharedString( "SPACE_OBJECT_TRANSPARENCY" ), BlueSharedString( "SOT_OPAQUE" ) );
			castsShadows = true;
			break;
		default:
			break;
		}


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
			dna->ModifyTextureResPath( highResPath );
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
					lodResCollector[highResPath] = lodResource;


					if( inheritableTextures )
					{
						InheritableTextureKey key;
						key.hullIndex = hullIdx;
						key.meshIndex = area->index;
						key.name = it->first;

						inheritableTextures->insert( std::make_pair( key, lodResource ) );
					}
				}
			}
			else
			{
				newShader->AddResourceTexture2D( it->first, highResPath.c_str() );
			}
		}

		// pattern textures
		size_t patternLayerCount = dna->GetPatternLayerCount();

		if( patternLayerCount )
		{
			newShader->SetOption( BlueSharedString( "SPACE_OBJECT_PPT_ENABLED" ), BlueSharedString( "SOPPT_ENABLED" ) );
		}
		else
		{
			newShader->SetOption( BlueSharedString( "SPACE_OBJECT_PPT_ENABLED" ), BlueSharedString( "SOPPT_DISABLED" ) );
		}

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
		newMeshArea->SetGenerateDepthArea( shaderData->doGenerateDepthArea );
		newMeshArea->SetMaterial( newShader );
		newMeshArea->SetIndex( area->index + (unsigned int)meshIndexOffset );
		newMeshArea->SetCount( area->count );
		newMeshArea->SetCastsShadows( castsShadows );
		
		meshAreaVector->Append( newMeshArea );
	}

	return hullAreas->size();
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
		if( !strcmp( usage, "PmdgMap" ) || !strcmp( usage, "ArMap" ) || !strcmp( usage, "NoMap" ) )
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

	// cycle over all hulls in the multi-hull list
	Vector3 hullOffset( 0.f, 0.f, 0.f );
	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		// cycle over all spritesets of this hull
		const std::vector<EveSOFDataMgr::HullSpriteSetData>& hullSpriteSets = dna->GetHullSpriteSets( hullIdx );
		for( auto ssit = hullSpriteSets.begin(); ssit != hullSpriteSets.end(); ++ssit )
		{
			const EveSOFDataMgr::HullSpriteSetData* spriteSetData = &(*ssit);

			// vivible?
			if( dna->IsInVisibilityData( spriteSetData->visibilityGroup ) )
			{
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

					// color data?
					const Color* colorSet = dna->GetColorSet();

					// create spriteset items
					EveSpriteSetItemPtr spriteSetItem;
					spriteSetItem.CreateInstance();

					// set it up the colorset data
					spriteSetItem->m_color = itemData->intensity * colorSet[itemData->colorType];

					// set it up the per-hull data
					spriteSetItem->m_blinkPhase = itemData->blinkPhase;
					spriteSetItem->m_blinkRate = itemData->blinkRate;
					spriteSetItem->m_boneIndex = itemData->boneIndex;
					spriteSetItem->m_falloff = itemData->falloff;
					spriteSetItem->m_maxScale = itemData->maxScale;
					spriteSetItem->m_minScale = itemData->minScale;
					spriteSetItem->m_position = itemData->position + hullOffset;

					// put it into spriteset
					spriteSet->Add( spriteSetItem );
				}
				// spriteset needs internal rebuild
				spriteSet->Rebuild();
				// put set onto ship
				obj->AddAttachment( spriteSet );
			}
		}

		// next hull needs offset update from hull's locator
		const Vector3* nextSubsystemOffset = dna->GetHullNextSubsystemOffset( hullIdx );
		if( nextSubsystemOffset )
		{
			hullOffset += *nextSubsystemOffset;
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   This is where it is all going to happen
// --------------------------------------------------------------------------------
void EveSOF::SetupSpotlightSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// cycle over all hulls in the multi-hull list
	Vector3 hullOffset( 0.f, 0.f, 0.f );
	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		// cycle over all spritesets of this hull
		const std::vector<EveSOFDataMgr::HullSpotlightSetData>& hullSpotlightSets = dna->GetHullSpotlightSets( hullIdx );
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
			BlueSharedString textureMap( "TextureMap" );
			glowEffect->AddResourceTexture2D( textureMap, spotlightSetData->glowTextureResPath.c_str() );
			coneEffect->AddResourceTexture2D( textureMap, spotlightSetData->coneTextureResPath.c_str() );

			// parameters
			coneEffect->AddParameterFloat( BlueSharedString( "zOffset" ), spotlightSetData->zOffset );

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
				TriMatrixTranslate( &spotlightSetItem->m_transform, &ssiit->transform, &hullOffset );

				// add it
				spotlightSet->AddSpotlightItem( spotlightSetItem );
			}
			// spotlightset needs internal rebuild
			spotlightSet->Rebuild();
			// add to ship
			obj->AddAttachment( spotlightSet );
		}

		// next hull needs offset update from hull's locator
		const Vector3* nextSubsystemOffset = dna->GetHullNextSubsystemOffset( hullIdx );
		if( nextSubsystemOffset )
		{
			hullOffset += *nextSubsystemOffset;
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   This is where it is all going to happen
// --------------------------------------------------------------------------------
void EveSOF::SetupPlaneSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// cycle over all hulls in the multi-hull list
	Vector3 hullOffset( 0.f, 0.f, 0.f );
	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		// cycle over all spritesets of this hull
		const std::vector<EveSOFDataMgr::HullPlaneSetData>& hullPlaneSets = dna->GetHullPlaneSets( hullIdx );
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
			float angularFadeOut = 0.f;
			switch( planeSetData->usage )
			{
			case EveSOFDataHullPlaneSet::USAGE_STANDARD:
				effectResPath = planeSetData->skinned ? "res:/graphics/effect/managed/space/spaceobject/fx/skinned_planeglow.fx" : "res:/graphics/effect/managed/space/spaceobject/fx/planeglow.fx";
				break;
			case EveSOFDataHullPlaneSet::USAGE_HAZE:
				angularFadeOut = 1.f;
				effectResPath = planeSetData->skinned ? "res:/graphics/effect/managed/space/spaceobject/fx/skinned_planeglow.fx" : "res:/graphics/effect/managed/space/spaceobject/fx/planeglow.fx";
				break;
			case EveSOFDataHullPlaneSet::USAGE_SPACE_VIDEO:
				effectResPath = planeSetData->skinned ? "res:/graphics/effect/managed/space/spaceobject/fx/skinned_planehologram.fx" : "res:/graphics/effect/managed/space/spaceobject/fx/planehologram.fx";
				imageMapResPath = "dynamic:/inspacevideos";
				break;
			case EveSOFDataHullPlaneSet::USAGE_HANGAR_VIDEO:
				effectResPath = planeSetData->skinned ? "res:/graphics/effect/managed/space/spaceobject/fx/skinned_planehologram.fx" : "res:/graphics/effect/managed/space/spaceobject/fx/planehologram.fx";
				imageMapResPath = "dynamic:/hangarvideos";
				// Set the pickbuffer so we can pick the videos in the client
				planeSet->SetPickBufferID( PICKABLE_HANGARVIEO_BUFFER_ID );
				break;
			}

			planeEffect->SetEffectPathName( effectResPath.c_str() );

			// textures
			planeEffect->AddResourceTexture2D( BlueSharedString( "Layer1Map" ), planeSetData->layer1MapResPath.c_str() );
			planeEffect->AddResourceTexture2D( BlueSharedString( "Layer2Map" ), planeSetData->layer2MapResPath.c_str() );
			planeEffect->AddResourceTexture2D( BlueSharedString( "MaskMap" ), planeSetData->maskMapResPath.c_str() );

			// Imagemap texture?
			if( !imageMapResPath.empty() || !externalParamName.empty() )
			{
				planeEffect->AddResourceTexture2D( BlueSharedString( "ImageMap" ), imageMapResPath.c_str() );

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
			Vector4 planeData( angularFadeOut, (float)planeSetData->atlasSize, 0.f, 0.f );
			planeEffect->AddParameterVector4( BlueSharedString( "PlaneData" ), &planeData );

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
				planeSetItem->m_position = psiit->position + hullOffset;
				planeSetItem->m_rotation = psiit->rotation;
				planeSetItem->m_scaling = psiit->scaling;
				planeSetItem->m_color = psiit->color;
				planeSetItem->m_layer1Transform = psiit->layer1Transform;
				planeSetItem->m_layer1Scroll = psiit->layer1Scroll;
				planeSetItem->m_layer2Transform = psiit->layer2Transform;
				planeSetItem->m_layer2Scroll = psiit->layer2Scroll;
				planeSetItem->m_boneIndex = psiit->boneIndex;
				planeSetItem->m_maskAtlasID = psiit->maskMapAtlasIndex;
				planeSetItem->m_blinkData = Vector4( psiit->rate, psiit->phase, psiit->dutyCycle, (float)psiit->blinkMode );

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
			obj->AddAttachment( planeSet );
		}

		// next hull needs offset update from hull's locator
		const Vector3* nextSubsystemOffset = dna->GetHullNextSubsystemOffset( hullIdx );
		if( nextSubsystemOffset )
		{
			hullOffset += *nextSubsystemOffset;
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   This is where it is all going to happen
// --------------------------------------------------------------------------------
void EveSOF::SetupSpriteLineSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// cycle over all hulls in the multi-hull list
	Vector3 hullOffset( 0.f, 0.f, 0.f );
	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		// cycle over all spritelinesets of this hull
		const std::vector<EveSOFDataMgr::HullSpriteLineSetData>& hullSpriteLineSets = dna->GetHullSpriteLineSets( hullIdx );
		for( auto slsit = hullSpriteLineSets.begin(); slsit != hullSpriteLineSets.end(); ++slsit )
		{
			const EveSOFDataMgr::HullSpriteLineSetData* spriteLineSetData = &(*slsit);

			// vivible?
			if( dna->IsInVisibilityData( spriteLineSetData->visibilityGroup ) )
			{
				// create a spriteset for this ship
				EveSpriteLineSetPtr spriteLineSet;
				spriteLineSet.CreateInstance();
				// set shader
				spriteLineSet->Setup( m_spriteSetEffect, spriteLineSetData->skinned );
				// add all the individual items
				for( auto slsiit = spriteLineSetData->items.begin(); slsiit != spriteLineSetData->items.end(); ++slsiit )
				{
					const EveSOFDataMgr::HullSpriteLineSetItemData* itemData = &(*slsiit);

					// color data?
					const Color* colorSet = dna->GetColorSet();

					// create spritelineset items
					EveSpriteLineSetItemPtr spriteLineSetItem;
					spriteLineSetItem.CreateInstance();

					// set it up the colorset data
					spriteLineSetItem->m_color = itemData->intensity * colorSet[itemData->colorType];

					// set it up the per-hull data
					spriteLineSetItem->m_blinkPhaseShift = itemData->blinkPhaseShift;
					spriteLineSetItem->m_blinkPhase = itemData->blinkPhase;
					spriteLineSetItem->m_blinkRate = itemData->blinkRate;
					spriteLineSetItem->m_boneIndex = itemData->boneIndex;
					spriteLineSetItem->m_falloff = itemData->falloff;
					spriteLineSetItem->m_maxScale = itemData->maxScale;
					spriteLineSetItem->m_minScale = itemData->minScale;
					spriteLineSetItem->m_position = itemData->position + hullOffset;
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
				obj->AddAttachment( spriteLineSet );
			}
		}

		// next hull needs offset update from hull's locator
		const Vector3* nextSubsystemOffset = dna->GetHullNextSubsystemOffset( hullIdx );
		if( nextSubsystemOffset )
		{
			hullOffset += *nextSubsystemOffset;
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   This is where it is all going to happen
// --------------------------------------------------------------------------------
void EveSOF::SetupHazeSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// cycle over all hulls in the multi-hull list
	Vector3 hullOffset( 0.f, 0.f, 0.f );
	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		// cycle over all hazesets of this hull
		const std::vector<EveSOFDataMgr::HullHazeSetData>& hullHazeSets = dna->GetHullHazeSets( hullIdx );
		for( auto hhsit = hullHazeSets.begin(); hhsit != hullHazeSets.end(); ++hhsit )
		{
			const EveSOFDataMgr::HullHazeSetData* hazeSetData = &(*hhsit);

			// vivible?
			if( dna->IsInVisibilityData( hazeSetData->visibilityGroup ) )
			{
				// create a hazeset for this ship
				EveHazeSetPtr hazeSet;
				hazeSet.CreateInstance();
				// set shader, depends on type
				switch( hazeSetData->hazeType )
				{
				case EveSOFDataHullHazeSet::TYPE_SPHERICAL:
					hazeSet->Setup( m_hazeSetEffectSpherical );
					break;
				case EveSOFDataHullHazeSet::TYPE_HALFSPHERICAL:
					hazeSet->Setup( m_hazeSetEffectHalfSpherical );
					break;
				}
				// add all the individual items
				for( auto hhsiit = hazeSetData->items.begin(); hhsiit != hazeSetData->items.end(); ++hhsiit )
				{
					const EveSOFDataMgr::HullHazeSetItemData* itemData = &(*hhsiit);

					// color data from colorset
					const Color* colorSet = dna->GetColorSet();
					Color color = colorSet[itemData->colorType];

					// create spritelineset items
					EveHazeSetItemPtr hazeSetItem;
					hazeSetItem.CreateInstance();

					// set it up the colorset data
					hazeSetItem->m_color = itemData->hazeBrightness * color;

					// set it up the per-hull data
					hazeSetItem->m_position = itemData->position + hullOffset;
					hazeSetItem->m_rotation = itemData->rotation;
					hazeSetItem->m_scaling = itemData->scaling;
					hazeSetItem->m_boneIndex = -1;
					hazeSetItem->m_hazeData = Vector4( itemData->hazeFalloff, itemData->sourceSize, itemData->sourceBrightness, itemData->boosterGainInfluence ? 1.f : 0.f );

					// put it into hazeset
					hazeSet->AddHazeItem( hazeSetItem );
				}
				// spriteset needs internal rebuild
				hazeSet->Rebuild();
				// put set onto ship
				obj->AddAttachment( hazeSet );
			}
		}

		// next hull needs offset update from hull's locator
		const Vector3* nextSubsystemOffset = dna->GetHullNextSubsystemOffset( hullIdx );
		if( nextSubsystemOffset )
		{
			hullOffset += *nextSubsystemOffset;
		}
	}
}

// --------------------------------------------------------------------------------
void EveSOF::SetupBannerSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// cycle over all hulls in the multi-hull list
	Vector3 hullOffset( 0.f, 0.f, 0.f );
	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		auto& hullBannerSets = dna->GetHullBannerSets( hullIdx );
		for( auto hbsit = begin( hullBannerSets ); hbsit != end( hullBannerSets ); ++hbsit )
		{
			auto& bannerSetData = *hbsit;

			EveBannerSetPtr bannerSet;

			for( auto hhsiit = begin( bannerSetData.items ); hhsiit != end( bannerSetData.items ); ++hhsiit )
			{
				auto& itemData = *hhsiit;

				if( !dna->IsInVisibilityData( itemData.visibilityGroup ) )
				{
					continue;
				}

				if( !bannerSet )
				{
					bannerSet.CreateInstance();
				}

				bannerSet->AddBanner( itemData.item );
				// create the light
				Tr2PointLightPtr light;
				light.CreateInstance();

				LightData data;
				data.position = itemData.item.position;

				float rad = itemData.bannerLight.radiusMultiplier *
					max( itemData.item.scaling.x, max( itemData.item.scaling.y, itemData.item.scaling.z ) );
				float innerRad = ( rad * itemData.bannerLight.innerRadiusMultiplier );
				data.radius = rad;
				data.innerRadius = innerRad;
				
				data.color = Color( 0.0, 0.0, 0.0, 0.0 );
				data.brightness = itemData.bannerLight.brightness;
				data.noiseAmplitude = itemData.bannerLight.noiseAmplitude;
				data.noiseFrequency = itemData.bannerLight.noiseFrequency;
				data.noiseOctaves = itemData.bannerLight.noiseOctaves;

				light->SetLightData( data );
				bannerSet->AddLight( light );

				bannerSet->SetLightColorSaturation( itemData.bannerLight.saturation );

			}

			if( !bannerSet )
			{
				continue;
			}

			auto& shaderData = dna->GetGenericBannerShaderData();

			Tr2EffectPtr effect;
			effect.CreateInstance();

			effect->StartUpdate();
			effect->SetEffectPathName( shaderData.shader.c_str() );

			for( auto pit = begin( shaderData.defaultParameters ); pit != end( shaderData.defaultParameters ); ++pit )
			{
				effect->AddParameterVector4( pit->first, &pit->second );
			}
			for( auto tit = begin( shaderData.defaultTextures ); tit != end( shaderData.defaultTextures ); ++tit )
			{
				effect->AddResourceTexture2D( tit->first, tit->second.resFilePath.c_str() );
			}

			Tr2LodResourcePtr imageMap;
			imageMap.CreateInstance();
			effect->AddResourceTexture2DLod( BlueSharedString( "ImageMap" ), imageMap );
			bannerSet->AddLodResource( imageMap );

			effect->EndUpdate();
			bannerSet->SetEffect( effect );
			bannerSet->SetKey( int32_t( bannerSetData.usage ) );
			bannerSet->Rebuild();
			obj->AddAttachment( bannerSet );

			static const char* names[] = { 
				"AllianceLogoResPath", 
				"CorpLogoResPath", 
				"CeoPortraitResPath", 
				"VerticalBannerResPath", 
				"HorizontalBannerResPath", 
				"TargetSystemAllianceLogoResPath",
				"TargetSystemVerticalBannerResPath",
				"TargetSystemHorizontalBannerResPath",
				"TargetSystemInfo0ResPath",
				"TargetSystemInfo1ResPath",
				"TargetSystemInfo2ResPath",
				"TargetSystemInfo3ResPath",
				"TargetSystemInfo4ResPath",
				"TargetSystemStatusResPath",
				"CurrentSystemAllianceLogoResPath",
				"CurrentSystemVerticalBannerResPath",
				"CurrentSystemHorizontalBannerResPath",
				"PublicityPosterResPath",
				"PublicityPortraitResPath",
				"RecruitmentInformation0ResPath",
				"RecruitmentInformation1ResPath",
				"RecruitmentInformation2ResPath",
				"RecruitmentInformation3ResPath",
				"RecruitmentInformation4ResPath",
			};
			static_assert( sizeof( names ) / sizeof( names[0] ) == EveSOFDataHullBanner::_USAGE_COUNT, "Banner usage names mismatch" );

			const char* externalParamName = nullptr;
			if( bannerSetData.usage >= 0 && bannerSetData.usage < EveSOFDataHullBanner::_USAGE_COUNT )
			{
				externalParamName = names[bannerSetData.usage];
			}

			Tr2ExternalParameterPtr externalParameter;
			externalParameter.CreateInstance();
			externalParameter->SetName( externalParamName );
			externalParameter->SetDestinationObject( imageMap );
			externalParameter->SetDestinationAttribute( "highDetailResPath" );
			externalParameter->Initialize();
			obj->AddExternalParameter( externalParameter );
		}

		// next hull needs offset update from hull's locator
		const Vector3* nextSubsystemOffset = dna->GetHullNextSubsystemOffset( hullIdx );
		if( nextSubsystemOffset )
		{
			hullOffset += *nextSubsystemOffset;
		}
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

void RecursiveBindParticleEmitters( EveTransform* transform, TriCurveSet* curveSet, Tr2CurveScalar* curve )
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

void RecursiveBindParticleEmitters( IEveSpaceObjectChild* child, TriCurveSet* curveSet, Tr2CurveScalar* curve )
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
			effectChild->SetInheritProperties( dna->GetColorSet() );
			effectChild->SetOrigin( IEveSpaceObjectChild::SOF );
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
			Tr2CurveScalarPtr scalarCurve;
			scalarCurve.CreateInstance();

			scalarCurve->AddKey( 0.0f, animIt->startRate, Tr2CurveInterpolation::LINEAR, 0, 0, Tr2CurveTangentType::AUTO );
			scalarCurve->AddKey( 1.0f, animIt->endRate, Tr2CurveInterpolation::LINEAR, 0, 0, Tr2CurveTangentType::AUTO );

			std::vector<EveTransformPtr> transformVector = childrenToBindTo[animIt->id];
			for( auto transformIt = transformVector.begin(); transformIt != transformVector.end(); ++transformIt )
			{
				RecursiveBindParticleEmitters( ( *transformIt ), curveSet, scalarCurve );
			}
			std::vector<IEveSpaceObjectChildPtr> childVector = soChildrenToBindTo[animIt->id];
			for( auto childIt = childVector.begin(); childIt != childVector.end(); ++childIt )
			{
				RecursiveBindParticleEmitters( ( *childIt ), curveSet, scalarCurve );
			}

			curveSet->AddCurve( (ITriFunctionPtr)scalarCurve );
		}
		// Do we have valid rotation info?
		if( animIt->startRotationTime != -1.0 )
		{
			// Create the rotations curve
			Tr2CurveQuaternionPtr curve;
			curve.CreateInstance();
			curve->AddKey( animIt->startRotationTime, animIt->startRotationValue, Tr2CurveInterpolation::LINEAR );
			curve->AddKey( animIt->endRotationTime, animIt->endRotationValue, Tr2CurveInterpolation::LINEAR );

			curveSet->AddCurve( (ITriFunctionPtr)curve );

			// Create the binding to the model rotation curve
			TriValueBindingPtr binding;
			binding.CreateInstance();
			binding->SetSource( "currentValue", curve->GetRawRoot() );
			if( !obj->GetModelRotationCurve() )
			{
				Tr2RotationAdapterPtr modelRotationcurve;
				modelRotationcurve.CreateInstance();
				obj->SetModelRotationCurve( modelRotationcurve.p );
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
//   add effect children to the space object
// --------------------------------------------------------------------------------
void EveSOF::SetupEffectChildren( EveSpaceObject2Ptr newObj, const EveSOFDNAPtr dna ) const
{
	// Experimental features for phase 6
	// Note that this will ignore the child id and the animation id thing... don't know what will happen with the rorqual...
	const std::vector<EveSOFDataMgr::HullChildSetData>& hullChildSets = dna->GetHullChildSets();

	for( auto childSet : hullChildSets )
	{
		if( !dna->IsInVisibilityData( childSet.visibilityGroup ) )
		{
			continue;
		}

		for( auto childSetItem : childSet.items )
		{
			IRootPtr p;
			IRoot* tmp = BeResMan->LoadObject( childSetItem.redFilePath.c_str() );
			if( !tmp )
			{
				CCP_LOGERR( "resource file %s is invalid!", childSetItem.redFilePath.c_str() );
				continue;
			}
			p.Attach( tmp );

			// is it of right type?
			EveTransformPtr child;
			IEveSpaceObjectChildPtr effectChild;
			if( p->QueryInterface( BlueInterfaceIID<EveTransform>(), (void**)&child, BEQI_SILENT ) )
			{
				child->SetRotation( childSetItem.rotation );
				child->SetScaling( childSetItem.scaling );
				child->SetTranslation( childSetItem.translation );
				newObj->AddToChildrenList( child );
			}
			else if( p->QueryInterface( BlueInterfaceIID<IEveSpaceObjectChild>(), (void**)&effectChild, BEQI_SILENT ) )
			{
				effectChild->Setup( &childSetItem.scaling, &childSetItem.rotation, &childSetItem.translation, childSetItem.lowestLodVisible );
				effectChild->SetInheritProperties( dna->GetColorSet() );
				effectChild->SetOrigin( IEveSpaceObjectChild::SOF );
				newObj->AddToEffectChildrenList( effectChild );
			}
			else
			{
				CCP_LOGERR( "resource file %s is not of correct type!", childSetItem.redFilePath.c_str() );
				return;
			}
		}
	}
}


// --------------------------------------------------------------------------------
// Description:
//   add controllers to the space object
// --------------------------------------------------------------------------------
void EveSOF::SetupControllers( EveSpaceObject2Ptr newObj, const EveSOFDNAPtr dna ) const
{
	auto& hullControllers = dna->GetHullControllers();
	for( auto cit = begin( hullControllers ); cit != end( hullControllers ); ++cit )
	{
		if( auto controller = BeResMan->LoadObject<ITr2Controller>( cit->c_str() ) )
		{
			newObj->AddController( controller );
		}
		else
		{
			CCP_LOGERR( "controller resource file %s is invalid!", cit->c_str() );
		}
	}
}


// --------------------------------------------------------------------------------
// Description:
//   add audio children to the space object
// --------------------------------------------------------------------------------
void EveSOF::SetupAudio( EveSpaceObject2Ptr newObj, const EveSOFDNAPtr dna ) const
{
	auto& hullEmitters = dna->GetHullSoundEmitters();
	for( auto cit = begin( hullEmitters ); cit != end( hullEmitters ); ++cit )
	{
		TriObserverLocalPtr observer;
		observer.CreateInstance();
		observer->m_name = cit->name;
		observer->SetPosition( cit->position );

		IBluePlacementObserverPtr emitter;
		BeClasses->CreateInstanceFromName( "AudEmitter", BlueInterfaceIID<IBluePlacementObserver>(), reinterpret_cast<void**>( &emitter.p ) );
		if( !emitter )
		{
			CCP_LOGERR( "EveSOF: failed to create an audio emitter" );
			continue;
		}
		observer->SetObserver( emitter );
		newObj->AddObserver( observer );

		ITr2AudEmitterPtr theRealEmitter;
		if( emitter->QueryInterface( BlueInterfaceIID<ITr2AudEmitter>(), reinterpret_cast<void**>( &theRealEmitter.p ), BEQI_SILENT ) && theRealEmitter )
		{
			theRealEmitter->Initialize( cit->name.c_str(), cit->prefix.c_str(), cit->position );
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
		instanceDef.Add( Tr2VertexDefinition::BYTE_4, Tr2VertexDefinition::TEXCOORD, 3 );

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
				dna->ModifyTextureResPath( resPath );
				newShader->AddResourceTexture2D( it->first, resPath.c_str() );
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
			newMeshArea->SetName( "hull" );
			newMeshArea->SetMaterial( newShader );
			areas->Append( newMeshArea );
		}
		EveChildMeshPtr childMesh;
		childMesh.CreateInstance();
		childMesh->SetMesh( mesh );
		childMesh->Setup( nullptr, nullptr, nullptr, him->lowestLodVisible );
		newObj->AddToEffectChildrenList( static_cast<IEveSpaceObjectChild*>(childMesh) );
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
					customMask->Setup( 
						patternProjectionData->position, 
						patternProjectionData->scaling, 
						patternProjectionData->rotation, 
						patternProjectionData->isMirrored, 
						patternLayerData->projectionAddressModeU == Tr2RenderContextEnum::TA_CLAMP,
						patternLayerData->projectionAddressModeV == Tr2RenderContextEnum::TA_CLAMP,
						patternLayerData->materialSourceID,
						patternLayerData->materialTargets );
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
					lodResource->SetResourcePath( TR2_LOD_LOW, genericDamageData->shieldGeometryResFilePath.c_str() );
					lodResource->SetResourcePath( TR2_LOD_MEDIUM, genericDamageData->shieldGeometryResFilePath.c_str() );
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
//   Sets up lights for the new space object
// --------------------------------------------------------------------------------
void EveSOF::SetupLights( EveSpaceObject2Ptr spaceObject, const EveSOFDNAPtr dna ) const
{
	// cycle over all hulls in the multi-hull list
	Vector3 hullOffset( 0.f, 0.f, 0.f );
	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		const std::vector<EveSOFDataMgr::HullLightSetData>& hullLightSets = dna->GetHullLightSets( hullIdx );

		for( auto hls = hullLightSets.begin(); hls != hullLightSets.end(); ++hls )
		{
			if( !dna->IsInVisibilityData( hls->visibilityGroup ) )
			{
				continue;
			}

			for( auto hlsi = hls->items.begin(); hlsi != hls->items.end(); ++hlsi )
			{
				auto lightSetItem = *hlsi;
				Tr2LightPtr light;

				if( lightSetItem.type == EveSOFDataHullLightSetItem::POINT_LIGHT )
				{
					Tr2PointLightPtr pointLight;
					pointLight.CreateInstance();
					light = pointLight;
				}
				else if( lightSetItem.type == EveSOFDataHullLightSetItem::TEXTURED_POINT_LIGHT )
				{
					Tr2TexturedPointLightPtr texuredPointLight;
					texuredPointLight.CreateInstance();
					light = texuredPointLight;
				}
				else if( lightSetItem.type == EveSOFDataHullLightSetItem::SPOT_LIGHT )
				{
					Tr2SpotLightPtr spotLight;
					spotLight.CreateInstance();
					light = spotLight;
				}
				else 
				{
					CCP_LOGERR( "EveSOF::SetupLights: Invalid light set item type %s for hull %s", lightSetItem.type, dna->GetDnaString() );
					continue;
				}
				
				LightData data;
				data.position = lightSetItem.position;
				data.radius = lightSetItem.radius;
				data.innerRadius = lightSetItem.innerRadius;
				data.color = dna->GetColorSet()[lightSetItem.lightColor];
				data.brightness = lightSetItem.brightness;
				data.noiseAmplitude = lightSetItem.noiseAmplitude;
				data.noiseFrequency = lightSetItem.noiseFrequency;
				data.noiseOctaves = lightSetItem.noiseOctaves;
				data.rotation = lightSetItem.rotation;
				data.innerAngle = lightSetItem.innerAngle;
				data.outerAngle = lightSetItem.outerAngle;
				data.texturePath = lightSetItem.texturePath;
				data.boneIndex = lightSetItem.boneIndex;
				light->SetLightData( data );

				spaceObject->AddLight( light );
			}
		}

		// next hull needs offset update from hull's locator
		const Vector3* nextSubsystemOffset = dna->GetHullNextSubsystemOffset( hullIdx );
		if( nextSubsystemOffset )
		{
			hullOffset += *nextSubsystemOffset;
		}
	}
}


Tr2EffectPtr EveSOF::CreateBoosterEffect( const EveSOFDataMgr::RaceBoosterData* rdata, const BlueSharedString& lodOption ) const
{
	Tr2EffectPtr effect;
	effect.CreateInstance();
	effect->StartUpdate();

	effect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/Booster/BoosterVolumetric.fx" );
	effect->SetOption( BlueSharedString( "BOOSTER_LOD" ), lodOption );
	effect->AddParameterFloat( BlueSharedString( "NoiseSpeed0" ), rdata->shape0.noiseSpeed );
	effect->AddParameterVector4( BlueSharedString( "NoiseAmplitudeStart0" ), &rdata->shape0.noiseAmplitureStart );
	effect->AddParameterVector4( BlueSharedString( "NoiseAmplitudeEnd0" ), &rdata->shape0.noiseAmplitureEnd );
	effect->AddParameterVector4( BlueSharedString( "NoiseFrequency0" ), &rdata->shape0.noiseFrequency );
	effect->AddParameterColor( BlueSharedString( "Color0" ), &rdata->shape0.color );
	effect->AddParameterFloat( BlueSharedString( "NoiseSpeed1" ), rdata->shape1.noiseSpeed );
	effect->AddParameterVector4( BlueSharedString( "NoiseAmplitudeStart1" ), &rdata->shape1.noiseAmplitureStart );
	effect->AddParameterVector4( BlueSharedString( "NoiseAmplitudeEnd1" ), &rdata->shape1.noiseAmplitureEnd );
	effect->AddParameterColor( BlueSharedString( "Color1" ), &rdata->shape1.color );

	effect->AddParameterFloat( BlueSharedString( "WarpNoiseSpeed0" ), rdata->warpShape0.noiseSpeed );
	effect->AddParameterVector4( BlueSharedString( "WarpNoiseAmplitudeStart0" ), &rdata->warpShape0.noiseAmplitureStart );
	effect->AddParameterVector4( BlueSharedString( "WarpNoiseAmplitudeEnd0" ), &rdata->warpShape0.noiseAmplitureEnd );
	effect->AddParameterVector4( BlueSharedString( "WarpNoiseFrequency0" ), &rdata->warpShape0.noiseFrequency );
	effect->AddParameterColor( BlueSharedString( "WarpColor0" ), &rdata->warpShape0.color );
	effect->AddParameterFloat( BlueSharedString( "WarpNoiseSpeed1" ), rdata->warpShape1.noiseSpeed );
	effect->AddParameterVector4( BlueSharedString( "WarpNoiseAmplitudeStart1" ), &rdata->warpShape1.noiseAmplitureStart );
	effect->AddParameterVector4( BlueSharedString( "WarpNoiseAmplitudeEnd1" ), &rdata->warpShape1.noiseAmplitureEnd );
	effect->AddParameterColor( BlueSharedString( "WarpColor1" ), &rdata->warpShape1.color );

	Vector4 shapeAtlasSize( float( rdata->shapeAtlasHeight ), float( rdata->shapeAtlasCount ), 0, 0 );
	effect->AddParameterVector4( BlueSharedString( "ShapeAtlasSize" ), &shapeAtlasSize );
	effect->AddParameterVector4( BlueSharedString( "BoosterScale" ), &rdata->scale );

	effect->AddResourceTexture2D( BlueSharedString( "ShapeMap" ), rdata->shapeAtlasResPath.c_str() );
	effect->AddResourceTexture2D( BlueSharedString( "GradientMap0" ), rdata->gradient0ResPath.c_str() );
	effect->AddResourceTexture2D( BlueSharedString( "GradientMap1" ), rdata->gradient1ResPath.c_str() );
	effect->AddResourceTexture2D( BlueSharedString( "NoiseMap" ), "res:/Texture/Global/noise32cube_volume.dds" );

	// finish effect and set it
	effect->EndUpdate();
	return effect;
}


// --------------------------------------------------------------------------------
// Description:
//   add the booster to the new ship
// --------------------------------------------------------------------------------
void EveSOF::SetupBoosters( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// does this hull have boosters at all?
	if( dna->GetHullBoosterCount() == 0 )
	{
		return;
	}

	// create
	EveBoosterSet2Ptr set;
	set.CreateInstance();

	// per-race data
	const EveSOFDataMgr::RaceBoosterData* rdata = dna->GetRaceBoosterData();
	// each booster is a locator with an index, keep track
	uint32_t boosterLocatorIndex = 0;
	// cycle over all hulls in the multi-hull list
	Vector3 hullOffset( 0.f, 0.f, 0.f );
	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		// per-hull data
		const EveSOFDataMgr::HullBoosterData* hdata = dna->GetHullBoosterData( hullIdx );

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

		Tr2EffectPtr effect = CreateBoosterEffect( rdata, BlueSharedString( "BOOSTER_LOD_HIGH" ) );
		Tr2EffectPtr effectFar = CreateBoosterEffect( rdata, BlueSharedString( "BOOSTER_LOD_LOW" ) );

		set->SetEffect( effect, effectFar );

		// create and setup glows
		EveSpriteSetPtr glow;
		glow.CreateInstance();

		Tr2EffectPtr glowEffect;
		glowEffect.CreateInstance();
		glowEffect->StartUpdate();
		glowEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/Booster/BoosterGlowAnimated.fx" );
		glowEffect->AddResourceTexture2D( BlueSharedString( "NoiseMap" ), "res:/Texture/global/noise.dds" );
		glowEffect->AddResourceTexture2D( BlueSharedString( "DiffuseMap" ), "res:/Texture/Particle/whitesharp.dds" );
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
			trailEffect->AddParameterVector4( BlueSharedString( "TrailSize" ), &rdata->trailSize );
			trailEffect->AddParameterColor( BlueSharedString( "TrailColor" ), &rdata->trailColor );
			trailEffect->EndUpdate();
			trail->SetEffect( trailEffect );
			trail->SetMeshResPath( "res:/dx9/model/ship/booster/volumetricTrail.gr2" );
			set->SetTrail( trail );
		}

		// add all the indiviual items
		for( auto biit = hdata->items.begin(); biit != hdata->items.end(); ++biit )
		{
			EveLocator2Ptr locator;
			locator.CreateInstance();

			// locator naming is important
			char name[128];
			sprintf_s( name, "locator_booster_%i", ++boosterLocatorIndex );
			locator->SetName( name );

			// locator position tells where it is
			Matrix m;
			TriMatrixTranslate( &m, &biit->transform, &hullOffset );
			locator->SetTransform( m );

			ship->AddLocator( locator );
			set->Add( &m, &biit->functionality, biit->hasTrail, biit->atlasIndex0, biit->atlasIndex1, biit->lightScale );
		}
		glow->Rebuild();

		// next hull needs offset update from hull's locator
		const Vector3* nextSubsystemOffset = dna->GetHullNextSubsystemOffset( hullIdx );
		if( nextSubsystemOffset )
		{
			hullOffset += *nextSubsystemOffset;
		}
	}
	// add it to ship
	set->PrepareResources();
	ship->SetBoosterSet( set );
}

// --------------------------------------------------------------------------------
// Description:
//   add the hull decals to the new ship based off the decal sets
// --------------------------------------------------------------------------------
void EveSOF::SetupDecalSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna, const InheritableTextures& inheritableTextures ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// cycle over all hulls in the multi-hull list
	Vector3 hullOffset( 0.f, 0.f, 0.f );
	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		const std::vector<EveSOFDataMgr::HullDecalSetData>& hullDecalSets = dna->GetHullDecalSets( hullIdx );

		for( auto hdsit = hullDecalSets.begin(); hdsit != hullDecalSets.end(); ++hdsit )
		{
			if( !dna->IsInVisibilityData( hdsit->visibilityGroup ) )
			{
				continue;
			}

			for( auto hdsiit = hdsit->items.begin(); hdsiit != hdsit->items.end(); ++hdsiit )
			{
				// if we have a logo decal but the faction doesn't have a that logo in the logo set then we can skip this decal
				if( hdsiit->usage == EveSOFDataHullDecalSetItem::USAGE_LOGO && !dna->HasLogoSet( hdsiit->logoType ) )
				{
					continue;
				}

				// create
				EveSpaceObjectDecalPtr decal;
				decal.CreateInstance();
				
				// set general datas
				decal->SetPosition( hdsiit->position + hullOffset );
				decal->SetRotation( hdsiit->rotation );
				decal->SetScaling( hdsiit->scaling );
				decal->SetBoneIndex( hdsiit->boneIndex );
				decal->SetMinScreenSize( MIN_DECAL_SCREEN_SIZE );

				// pre-calculated index buffer is only valid for first multi-hull
				if( hdsiit->indexBuffer.empty() || hullIdx != 0 )
				{
					decal->SetIndices( nullptr, 0 );
				}
				else
				{
					decal->SetIndices( &hdsiit->indexBuffer[0], hdsiit->indexBuffer.size() );
				}

				// the decal effect
				Tr2EffectPtr shader;
				shader.CreateInstance();
				shader->StartUpdate();


				// construct shader path and set it on the Tr2Effect
				std::string shaderPath = dna->GetDecalShaderLocationResPath() + std::string( "/" ) + dna->GetShaderPrefix( false ) + m_decalsEffectName[hdsiit->usage].c_str();
				shader->SetEffectPathName( shaderPath.c_str() );
				
				// Set the glow color based on the colors in the colorsetuS
				if( hdsiit->usage != EveSOFDataHullDecalSetItem::USAGE_LOGO && hdsiit->usage != EveSOFDataHullDecalSetItem::USAGE_STANDARD )
				{
					Color decalGlowColor = dna->GetColorSet()[hdsiit->glowColorType];
					shader->AddParameterColor( BlueSharedString( "DecalGlowColor" ), &decalGlowColor );
				}

				// always set hull parameters & textures for this decal
				for( auto hdpit = hdsiit->parameters.begin(); hdpit != hdsiit->parameters.end(); ++hdpit )
				{
					shader->AddParameterVector4( hdpit->first, &hdpit->second );
				}
				for( auto hdtit = hdsiit->textures.begin(); hdtit != hdsiit->textures.end(); ++hdtit )
				{
					shader->AddResourceTexture2D( hdtit->first, hdtit->second.resFilePath.c_str() );
				}

				// find data on this shader from generics, we need it!
				const EveSOFDataMgr::GenericDecalShaderData* shaderData = dna->GetGenericDecalShaderData( m_decalsEffectName[hdsiit->usage] );
				if( shaderData )
				{
					// default shader textures & parameters from the generic data
					for( auto gtit = shaderData->defaultTextures.begin(); gtit != shaderData->defaultTextures.end(); ++gtit )
					{
						shader->AddResourceTexture2D( gtit->first, gtit->second.resFilePath.c_str() );
					}

					// parent hull textures
					for( auto ptit = shaderData->parentTextures.begin(); ptit != shaderData->parentTextures.end(); ++ptit )
					{
						if( hdsiit->meshIndex != -1 )
						{
							InheritableTextureKey key;
							key.hullIndex = hullIdx;
							key.meshIndex = hdsiit->meshIndex;
							key.name = *ptit;
							auto found = inheritableTextures.find( key );
							if( found != inheritableTextures.end() )
							{
								shader->AddResourceTexture2DLod( *ptit, found->second );
							}
							else
							{
								// get the filepath from the hull
								std::string resFilePath;
								if( dna->GetHullTextureWithMeshIndex( resFilePath, *ptit, hdsiit->meshIndex, hullIdx ) )
								{
									shader->AddResourceTexture2D( *ptit, resFilePath.c_str() );
								}
							}
						}
					}
				}
				
				// Set the logo from the logoset
				if( hdsiit->usage == EveSOFDataHullDecalSetItem::USAGE_LOGO )
				{
					const EveSOFDataMgr::LogoData* logo = dna->GetLogo( hdsiit->logoType );
					if( logo ) {
						for( auto textureit = logo->textures.begin(); textureit != logo->textures.end(); ++textureit )
						{
							shader->AddResourceTexture2D( (*textureit).first, (*textureit).second.resFilePath.c_str() );
						}
					}
				}


				// init and add
				shader->EndUpdate();
				decal->SetEffect( shader );
				decal->Initialize();
				obj->AddDecal( decal );
			}
		}

		// next hull needs offset update from hull's locator
		const Vector3* nextSubsystemOffset = dna->GetHullNextSubsystemOffset( hullIdx );
		if( nextSubsystemOffset )
		{
			hullOffset += *nextSubsystemOffset;
		}
	}
}


// --------------------------------------------------------------------------------
// Description:
//   add the hull locators to the new ship
// --------------------------------------------------------------------------------
void EveSOF::SetupLocators( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// cycle over all hulls in the multi-hull list
	Vector3 hullOffset( 0.f, 0.f, 0.f );
	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		// turret locators
		const std::vector<EveSOFDataMgr::LocatorData>& turretLocators = dna->GetHullTurretLocators( hullIdx );
		for( auto tlit = turretLocators.begin(); tlit != turretLocators.end(); ++tlit )
		{
			// create a new locator
			EveLocator2Ptr loc;
			loc.CreateInstance();

			// set it up
			loc->SetName( tlit->name );

			Matrix m;
			TriMatrixTranslate( &m, &tlit->transform, &hullOffset );
			loc->SetTransform( m );

			// add it to the new ship
			obj->AddLocator( loc );
		}

		// locator sets (like damage, explosions, etc.)
		const std::vector<BlueSharedString> locatorSetsNames = dna->GetHullLocatorSetNames( hullIdx );
		for( auto locatorSetName = locatorSetsNames.begin(); locatorSetName != locatorSetsNames.end(); ++locatorSetName )
		{
			auto locators = dna->GetHullLocators( locatorSetName->c_str(), hullIdx );
			if( locators )
			{
				EveLocatorSetsPtr locSets;
				locSets.CreateInstance();
				locSets->Set( locatorSetName->c_str(), (const Locator*) &(*locators)[0], locators->size() );
				locSets->Translate( hullOffset );
				obj->MergeToLocatorSet( *locSets );
			}
		}

		// audio locator
		const Vector3* pos = dna->GetHullAudioPosition( hullIdx );
		if( pos )
		{
			EveLocator2Ptr loc;
			loc.CreateInstance();
			loc->SetName( "locator_audio_booster" );
			loc->SetTransform( Matrix( 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, pos->x + hullOffset.x, pos->y + hullOffset.y, pos->z + hullOffset.z, 1.f ) );

			// add it to the new ship
			obj->AddLocator( loc );
		}

		// next hull needs offset update from hull's locator
		const Vector3* nextSubsystemOffset = dna->GetHullNextSubsystemOffset( hullIdx );
		if( nextSubsystemOffset )
		{
			hullOffset += *nextSubsystemOffset;
		}
	}
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
	if( factionData == nullptr )
	{
		return;
	}
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
				const Vector4* res = EveSOFUtils::SearchForParameterData( &m_dataMgr, factionData->colorData.colors, areaMaterialData, EveSOFDataArea::TYPE_PRIMARY, &param );
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
				EveSOFUtilsParameterName param( genericData->materialPrefixes, (*it)->GetParameterName() );
				if( param.IsMaterialIdxValid() )
				{
					param.ChangeMaterialIdx( genericData, factionData->materialUsageList[param.GetMaterialIdx()] );
				}
				// find data
				const Vector4* res = EveSOFUtils::SearchForParameterData( &m_dataMgr, factionData->colorData.colors, areaMaterialData, EveSOFDataArea::TYPE_PRIMARY, &param );
				if( res )
				{
					Tr2Vector4ParameterPtr p;
					if( (*it)->QueryInterface( BlueInterfaceIID<Tr2Vector4Parameter>(), (void**)&p, BEQI_SILENT ) )
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











