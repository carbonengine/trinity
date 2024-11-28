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
#include "Controllers/ITr2ControllerOwner.h"
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
#include "Eve/SpaceObject/Attachments/Sets/IEveSpaceObjectAttachmentOwner.h"
#include "Eve/SpaceObject/Attachments/EveTrailsSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpotlightSet.h"
#include "Eve/SpaceObject/Attachments/Sets/EvePlaneSet.h"
#include "Eve/SpaceObject/Attachments/EveBoosterSet2.h"
#include "Eve/SpaceObject/Attachments/EveSpaceObjectDecal.h"
#include "Eve/SpaceObject/Attachments/IEveSpaceObjectDecalOwner.h"
#include "Eve/SpaceObject/Children/EveChildMesh.h"
#include "Eve/SpaceObject/Children/EveChildContainer.h"
#include "Eve/SpaceObject/Children/EveChildParticleSystem.h"
#include "Eve/SpaceObject/Utils/EveLocator2.h"
#include "Tr2InstancedMesh.h"
#include "Tr2Mesh.h"
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
#include "Lights/Tr2FactionLight.h"
#include "Lights/Tr2PointLight.h"
#include "Lights/Tr2TexturedPointLight.h"
#include "Lights/Tr2SpotLight.h"
#include "Lights/ITr2LightOwner.h"
#include "Utilities/BoundingSphere.h"
#include "BlueObjectMetadata.h"

namespace
{
const float MIN_DECAL_SCREEN_SIZE = 10.f;
const float MIN_INSTANCED_MESH_SCREEN_SIZE = 2.5f;
const uint8_t PICKABLE_HANGARVIEO_BUFFER_ID = 100;

const char* GetPlaneSetEffectPath( EveSOFDataHullPlaneSet::Usage usage, bool isSkinned )
{
	switch( usage )
	{
	case EveSOFDataHullPlaneSet::USAGE_STANDARD:
		return isSkinned ? "res:/graphics/effect/managed/space/spaceobject/fx/skinned_planeglow.fx" : "res:/graphics/effect/managed/space/spaceobject/fx/planeglow.fx";
	case EveSOFDataHullPlaneSet::USAGE_HAZE:
		return isSkinned ? "res:/graphics/effect/managed/space/spaceobject/fx/skinned_planeglow.fx" : "res:/graphics/effect/managed/space/spaceobject/fx/planeglow.fx";
	case EveSOFDataHullPlaneSet::USAGE_SPACE_VIDEO:
		return isSkinned ? "res:/graphics/effect/managed/space/spaceobject/fx/skinned_planehologram.fx" : "res:/graphics/effect/managed/space/spaceobject/fx/planehologram.fx";
	case EveSOFDataHullPlaneSet::USAGE_HANGAR_VIDEO:
		return isSkinned ? "res:/graphics/effect/managed/space/spaceobject/fx/skinned_planehologram.fx" : "res:/graphics/effect/managed/space/spaceobject/fx/planehologram.fx";
	default:
		return "";
	}
}

Color ModifyColor( const Color& color, float saturation, float brightness )
{
	CTriColor cl;
	float maxc = std::max( { color.r, color.g, color.b, 1.0f } );
	cl.SetRGB( color.r / maxc, color.g / maxc, color.b / maxc, 1 );
	float h, s, v;
	cl.GetHSV( &h, &s, &v );
	s *= saturation;
	v *= brightness;
	cl.SetHSV( h, s, v );
	return Color( cl.r, cl.g, cl.b, color.a );
}

}


// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveSOF::EveSOF( IRoot* lockobj ) :
	PARENTLOCK( m_dataMgr ),
	m_allowFileCaching( true ),
	m_editorMode( false )
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
    GlobalStore().RegisterVariable( "BoneTransforms", &Tr2BoneTransformBuffer::GetInstance() );

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
	
	m_skinnedHazeSetEffectSpherical.CreateInstance();
	m_skinnedHazeSetEffectSpherical->StartUpdate();
	m_skinnedHazeSetEffectSpherical->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/fx/skinned_hazespherical.fx" );
	m_skinnedHazeSetEffectSpherical->EndUpdate();

	m_hazeSetEffectHalfSpherical.CreateInstance();
	m_hazeSetEffectHalfSpherical->StartUpdate();
	m_hazeSetEffectHalfSpherical->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/fx/hazehalfspherical.fx" );
	m_hazeSetEffectHalfSpherical->EndUpdate();
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
	std::string s = "BuildFromDna ";
	s += std::string( dnaString );
	CCP_STATS_ZONE( s.c_str() );

	EveSOFDNAPtr dna = CreateDna(dnaString);
	if( dna == nullptr )
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

	auto centerOffset = std::vector<Matrix>( 1, IdentityMatrix() );
	if( dna->GetBuildClass() == EveSOFDataHull::BUILDCLASS_EXTENSION )
	{
		// for extension we do things differently, since it is an EveChildMesh
		newObj->SetBoundingSphereInformation( dna->GetHullBoundingSphere() );
		EveSOFDataMgr::LocatorDirectionData center;
		center.boneIndex = -1;
		center.position = Vector3( 0, 0, 0 );
		center.rotation = Quaternion( 0, 0, 0, 1 );
		center.scaling = Vector3( 1, 1, 1 );
		// Create a fake placement
		EveSOFDataMgr::ExtensionPlacementData fakePlacement;
		fakePlacement.isInstanced = false;
		fakePlacement.name = BlueSharedString( "Solo Placement" );
		fakePlacement.offset = Vector3( 0, 0, 0 );
		fakePlacement.hasDistribution = false;
		fakePlacement.extendsBoundingSphere = false;
		fakePlacement.extendsShieldEllipsoid = false;
		CreatePlacement( newObj, dna, fakePlacement, std::vector<EveSOFDataMgr::LocatorDirectionData>( 1, center ), centerOffset );

		// create an empty mesh...
		Tr2MeshPtr mesh;
		mesh.CreateInstance();
		newObj->SetMesh( mesh );

		// ships needs a final ::Initialize call
		newObj->SetInheritProperties( dna->GetColorSet() );
		newObj->Initialize();


		return newObj->GetRawRoot();
	}
	
	// get us the base geometry
	SetupMesh( newObj, dna );
	SetupCustomMask( newObj, dna );

	// decals
	SetupDecalSets( BlueCastPtr( newObj->GetRawRoot() ), dna );

	// locators to ship
	SetupLocators( newObj, dna );
	SetupLocatorSets( newObj, dna, centerOffset );
	
	// Attachments
	SetupAttachments( BlueCastPtr( newObj->GetRawRoot() ), dna, centerOffset, EveSOFDataHullBuildFilter::STANDALONE );

	SetupImpactEffects( newObj, dna );

	// Effects
	SetupEffects( newObj, BlueCastPtr( newObj->GetRawRoot() ), dna, centerOffset, EveSOFDataHullBuildFilter::STANDALONE );

	newObj->SetInheritProperties( dna->GetColorSet() );

	SetupAudio( BlueCastPtr( newObj->GetRawRoot() ), dna );
	SetupControllers( BlueCastPtr( newObj->GetRawRoot() ), dna, EveSOFDataHullBuildFilter::STANDALONE );

	// model curves
	SetupModelCurves( newObj, dna );

	// instanced meshes
	SetupInstancedMeshes( newObj, dna, centerOffset );

	// layout
	SetupLayout( newObj, dna, centerOffset );

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

void EveSOF::SetupAttachments( IEveSpaceObjectAttachmentOwnerPtr newObj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const
{
	// Add all the fluff!
	SetupSpriteSets( newObj, dna, offsets, buildFlags );
	SetupSpotlightSets( newObj, dna, offsets, buildFlags );
	SetupPlaneSets( newObj, dna, offsets, buildFlags );
	SetupSpriteLineSets( newObj, dna, offsets, buildFlags );
	SetupHazeSets( newObj, dna, offsets, buildFlags );
	
        // Banners need to have external parameters, which we don't have for anything except for EveSpaceObject2Ptr...
        // So no banners for layouts... yet
	if( EveSpaceObject2Ptr spaceObject = BlueCastPtr( newObj->GetRootObject() ) )
	{
		if( !dna->UsingSof6() )
		{
			SetupBanners( spaceObject, dna, offsets );
		}
		else
		{
			SetupBannerSets( spaceObject, dna, offsets );
		}
	}
	
	if( ITr2LightOwnerPtr lightOwner = BlueCastPtr(newObj->GetRootObject() ) )
	{
		SetupLights( lightOwner, dna, offsets );	
	}
}

void EveSOF::SetupEffects( EveSpaceObject2Ptr obj, IEveEffectChildrenOwnerPtr childContainer, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const
{
	if( !dna->UsingSof6() )
	{
		// children, animations and particles
		SetupChildrenAndAnimations( obj, childContainer, dna, offsets, buildFlags );
	}
	else
	{
		// children from childsets
		SetupEffectChildren( obj, childContainer, dna, offsets, buildFlags );
	}
}


// --------------------------------------------------------------------------------
// Description:
//   Creates a dna object from a dna string
// --------------------------------------------------------------------------------
EveSOFDNAPtr EveSOF::CreateDna( const char* dnaString )
{
	// create a temporary(!) DNA object
	EveSOFDNAPtr dna;
	dna.CreateInstance();

	// init it with given dna string
	dna->Setup( dnaString, &m_dataMgr );
	if( !dna->IsValid() )
	{
		return nullptr;
	}
	return dna;
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
	case EveSOFDataHull::BUILDCLASS_EXTENSION: 
	{
		EveMobilePtr newRoot;
		newRoot.CreateInstance();
		spaceObject = newRoot;
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

	// bounding sphere comes from data, is faster
	obj->SetBoundingSphereInformation( dna->GetHullBoundingSphere() );
	obj->SetShapeEllipsoid( dna->GetHullShapeEllipsoid() );
	obj->EnableDynamicBoundingSphere( dna->DynamicBoundingSphereEnabled() );

	obj->SetCastsShadow( dna->CastShadow() );
	obj->SetIsAnimated( dna->IsHullAnimated() );

	// assign mesh to ship
	obj->SetMesh( CreateMesh( dna ) );

	// Set the reflectionMode based on the category
	obj->SetReflectionMode( dna->GetReflectionMode() );
}


// --------------------------------------------------------------------------------
// Description:
//		Creates a meshlod based on a dna
// --------------------------------------------------------------------------------
Tr2MeshPtr EveSOF::CreateMesh( const EveSOFDNAPtr dna ) const
{
	// need a mesh
	Tr2MeshPtr mesh;
	mesh.CreateInstance();

	// gr2 res path
	mesh->SetMeshResPath( dna->GetHullGeometryResPath().c_str() );

	
	SetupShaders( dna, mesh );

	// some areas need an accompanying depth area
	GenerateDepthFromAreaVector( mesh, mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT ), dna );

	return mesh;
}

// --------------------------------------------------------------------------------
void EveSOF::GenerateDepthFromAreaVector( Tr2MeshBase* mesh, const Tr2MeshAreaVector* meshAreaVector, const EveSOFDNAPtr dna ) const
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
size_t EveSOF::FillMeshAreaVector( Tr2MeshAreaVector* meshAreaVector, TriBatchType areaType, const EveSOFDNAPtr dna, size_t hullIdx, size_t meshIndexOffset ) const
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
			CCP_STATS_ZONE( "texture" );

			// res path how it is from hull data
			std::string highResPath = it->second.resFilePath;
			// get's modified by the faction data
			dna->ModifyTextureResPath( highResPath, m_allowFileCaching ? &m_existingFilesCache : nullptr );
			// make three paths for the three LODs
			std::string mediumResPath, lowResPath, ultraResPath;
			newShader->AddResourceTexture2D( it->first, highResPath.c_str() );
		}

		size_t patternLayerCount = dna->GetPatternLayerCount();

		// pattern textures
		if( patternLayerCount > 0 )
		{
            bool deletePatterData = false;
            auto pattern = dna->GetPatternApplicationData( deletePatterData );
			newShader->SetOption( BlueSharedString( "SPACE_OBJECT_PPT_ENABLED" ), BlueSharedString( "SOPPT_ENABLED" ) );
			
			for( size_t i = 0; i < patternLayerCount; ++i )
			{
				const EveSOFDataMgr::PatternLayerData* patternLayerData = dna->GetPatternLayerData( pattern, i );
				
				if( patternLayerData && dna->IsPatternLayerApplicableToArea( patternLayerData, area->areaType ) )
				{
					newShader->AddResourceTexture2D( patternLayerData->textureName, patternLayerData->textureResFilePath.c_str() );

					// pattern textures almost always require a sampler change: repeat is boring....
					newShader->AddSamplerOverride( BlueSharedString( std::string( patternLayerData->textureName.c_str() ) + "Sampler" ), patternLayerData->projectionAddressModeU, patternLayerData->projectionAddressModeV );
				}
			}
            if( deletePatterData )
            {
                delete pattern;
            }
		}
		else 
		{
			newShader->SetOption( BlueSharedString( "SPACE_OBJECT_PPT_ENABLED" ), BlueSharedString( "SOPPT_DISABLED" ) );	
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
		if( !strcmp( usage, "AlbedoMap" ) ||
			!strcmp( usage, "DirtMap" ) ||
			!strcmp( usage, "GlowMap" ) ||
			!strcmp( usage, "MaterialMap" ) ||
			!strcmp( usage, "NormalMap" ) ||
			!strcmp( usage, "AoMap" ) ||
			!strcmp( usage, "PaintMaskMap" ) ||
			!strcmp( usage, "RoughnessMap" ) )
		{
			ultraResPath = resPath;
			StringInsertStubBefore( ultraResPath, ".dds", "_ultraDetail" );
			mediumResPath = resPath;
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
void EveSOF::SetupSpriteSets( IEveSpaceObjectAttachmentOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const
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

			// visible?
			if( dna->IsInVisibilityData( spriteSetData->visibilityGroup ) )
			{
				// create a spriteset for this ship
				EveSpriteSetPtr spriteSet;
				spriteSet.CreateInstance();
				// set skinned or unskinned shader
				spriteSet->SetEffect( m_spriteSetEffect );
				spriteSet->SetSkinned( spriteSetData->skinned && buildFlags != EveSOFDataHullBuildFilter::INSTANCED_PLACEMENT );
				uint32_t index = 0;

				for( auto& offset : offsets )
				{
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
						spriteSetItem->m_position = XMVector3TransformCoord( itemData->position + hullOffset, offset );

						if( dna->UsingSof6() )
						{
							spriteSetItem->m_color = Saturate( spriteSetItem->m_color, itemData->saturation );

							if( itemData->light )
							{
								auto saturatedColor = Saturate( spriteSetItem->m_color, itemData->light->saturation );
								auto lightData = itemData->light->AsLightData( saturatedColor, 1.0f);
								lightData.position += spriteSetItem->m_position;
								lightData.boneIndex = spriteSetItem->m_boneIndex;

								EveSpriteLight light( lightData, itemData->blinkPhase, itemData->blinkRate, itemData->minScale, itemData->maxScale, index, itemData->light->lightProfilePath );
								spriteSet->AddLight( light );
							}
							index++;
						}
						spriteSet->Add(spriteSetItem);
					}
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
void EveSOF::SetupSpotlightSets( IEveSpaceObjectAttachmentOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// cycle over all hulls in the multi-hull list
	Vector3 hullOffset( 0.f, 0.f, 0.f );
	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		// cycle over all spritesets of this hull
		const std::vector<EveSOFDataMgr::HullSpotlightSetData>& hullSpotlightSets = dna->GetHullSpotlightSets( hullIdx );
		for( auto& spotlightSetData : hullSpotlightSets )
		{
			if( dna->UsingSof6() && !dna->IsInVisibilityData( spotlightSetData.visibilityGroup ) )
			{
				continue;
			}

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
			glowEffect->AddResourceTexture2D( textureMap, spotlightSetData.glowTextureResPath.c_str() );
			coneEffect->AddResourceTexture2D( textureMap, spotlightSetData.coneTextureResPath.c_str() );

			// parameters
			coneEffect->AddParameterFloat( BlueSharedString( "zOffset" ), spotlightSetData.zOffset );

			// that's it for setting up these shaders, must rebuild cache on it!
			coneEffect->EndUpdate();
			glowEffect->EndUpdate();

			// set to set
			spotlightSet->SetConeEffect( coneEffect );
			spotlightSet->SetGlowEffect( glowEffect );
			spotlightSet->SetSkinned( spotlightSetData.skinned && buildFlags != EveSOFDataHullBuildFilter::INSTANCED_PLACEMENT );
			uint32_t index = 0;

			for( auto& offset: offsets)
			{
				// add all individual items
				for( auto& ssiit : spotlightSetData.items )
				{
					const EveSOFDataMgr::FactionSpotlightSetColorData* factionSpotlightData = nullptr;
					if( !dna->UsingSof6() )
					{
						// faction data?
						factionSpotlightData = dna->GetFactionSpotlightSetData( ssiit.groupIndex );
						if( !factionSpotlightData )
						{
							// This spotlight item is not used for this faction.
							continue;
						}
					}

					// create it
					EveSpotlightSetItemPtr spotlightSetItem;
					spotlightSetItem.CreateInstance();
					Color lightColor = Color( 0, 0, 0, 0 );

					if( dna->UsingSof6() )
					{
						auto colorSet = dna->GetColorSet();
						spotlightSetItem->m_coneColor = ssiit.coneIntensity * ModifyColor( colorSet[ssiit.colorType], ssiit.saturation * 0.75f, 0.5f );
						spotlightSetItem->m_flareColor = ssiit.flareIntensity * ModifyColor( colorSet[ssiit.colorType], ssiit.saturation, 1.0f );
						spotlightSetItem->m_spriteColor = ssiit.spriteIntensity * ModifyColor( colorSet[ssiit.colorType], ssiit.saturation * 0.9f, 0.75f );
						lightColor = colorSet[ssiit.colorType];
					}
					else
					{
						// set it up the per-faction data
						spotlightSetItem->m_coneColor = ssiit.coneIntensity * factionSpotlightData->coneColor;
						spotlightSetItem->m_flareColor = ssiit.flareIntensity * factionSpotlightData->flareColor;
						spotlightSetItem->m_spriteColor = ssiit.spriteIntensity * factionSpotlightData->spriteColor;
						lightColor = factionSpotlightData->coneColor;
					}

					// set it up the per-hull data
					spotlightSetItem->m_boneIndex = ssiit.boneIndex;
					spotlightSetItem->m_boosterGainInfluence = ssiit.boosterGainInfluence;
					spotlightSetItem->m_spriteScale = ssiit.spriteScale;

					Matrix transformed = XMMatrixMultiply( XMMatrixMultiply(ssiit.transform, TranslationMatrix(hullOffset)), offset );
					TriMatrixTranslate( &spotlightSetItem->m_transform, &transformed, &hullOffset );
					
					if( dna->UsingSof6() )
					{
						if( ssiit.light ) {
							Quaternion rotation;
							Vector3 pos;
							Vector3 scale;
							Decompose( scale, rotation, pos, spotlightSetItem->m_transform );

							scale.x = abs( scale.x );
							scale.y = abs( scale.y );
							scale.z = abs( scale.z );

							float innerAngle = 0.0f;
							float outerAngle = 0.0f;
							if( scale.z > 0.f ) {
								innerAngle = atanf( max( scale.x, scale.y ) / ( 2.0f * scale.z ) ) * 180.0f / TRI_PI;
								outerAngle = atanf( max( scale.x, scale.y ) / ( 2.0f * scale.z ) ) * 180.0f / TRI_PI;
							}

							auto saturatedColor = Saturate( lightColor, ssiit.saturation * ssiit.light->saturation );
							auto data = ssiit.light->AsLightData( saturatedColor, scale.z, innerAngle, outerAngle );

							data.boneIndex = ssiit.boneIndex;
							data.position = ( TranslationMatrix( data.position ) * RotationMatrix( rotation ) * TranslationMatrix( pos ) ).GetTranslation();
							data.rotation = rotation;
							data.brightness *= ssiit.coneIntensity;

							spotlightSet->AddLight( EveSpotlightLight( data, index, ssiit.light->lightProfilePath, ssiit.boosterGainInfluence ) );
						}
						index++;
					}					

					// add it
					spotlightSet->AddSpotlightItem( spotlightSetItem );
				}
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
void EveSOF::SetupPlaneSets( IEveSpaceObjectAttachmentOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// cycle over all hulls in the multi-hull list
	Vector3 hullOffset( 0.f, 0.f, 0.f );

	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		// cycle over all planesets of this hull
		for( auto& planeSetData : dna->GetHullPlaneSets( hullIdx ) )
		{
			if( dna->UsingSof6() && !dna->IsInVisibilityData( planeSetData.visibilityGroup ) )
			{
				continue;
			}

			// create a planeset for this ship
			EvePlaneSetPtr planeSet;
			planeSet.CreateInstance();

			// create shader
			Tr2EffectPtr planeEffect;
			planeEffect.CreateInstance();
			planeEffect->StartUpdate();

			auto isSkinned = planeSetData.skinned && buildFlags != EveSOFDataHullBuildFilter::INSTANCED_PLACEMENT;
			planeEffect->SetEffectPathName( GetPlaneSetEffectPath( planeSetData.usage, isSkinned ) );

			TriTextureParameterPtr imageMap, layer1, layer2, mask;

			auto CreateTextureParameter = []( const char* name, const char* path, TriTextureParameterPtr &param ) {
				param.CreateInstance();
				param->SetParameterName( BlueSharedString( name ) );
				param->SetResourcePath( path );
				return param;
			};

			// Select the planeset's data based on the usage
			switch( planeSetData.usage )
			{
			case EveSOFDataHullPlaneSet::USAGE_SPACE_VIDEO:
				planeEffect->AddResource( CreateTextureParameter( "ImageMap", "dynamic:/inspacevideos", imageMap ) );
				break;
			case EveSOFDataHullPlaneSet::USAGE_HANGAR_VIDEO:
				planeEffect->AddResource( CreateTextureParameter( "ImageMap", "dynamic:/hangarvideos", imageMap ) );
				// Set the pickbuffer so we can pick the videos in the client
				planeSet->SetPickBufferID( PICKABLE_HANGARVIEO_BUFFER_ID );
				break;
			default:
				break;
			}

			// textures 
			planeEffect->AddResource( CreateTextureParameter( "Layer1Map", planeSetData.layer1MapResPath.c_str(), layer1 ) );
			planeEffect->AddResource( CreateTextureParameter( "Layer2Map", planeSetData.layer2MapResPath.c_str(), layer2 ) );
			planeEffect->AddResource( CreateTextureParameter( "MaskMap", planeSetData.maskMapResPath.c_str(), mask ) );

			// parameters
			float angularFadeOut = planeSetData.usage == EveSOFDataHullPlaneSet::USAGE_HAZE ? 1.f : 0.f;
			Vector4 planeData( 
				angularFadeOut,
				(float)planeSetData.atlasSize,
				std::floor( planeSetData.atlasAspectRatio.x ),
				std::floor( planeSetData.atlasAspectRatio.y )
			);
			planeEffect->AddParameterVector4( BlueSharedString( "PlaneData" ), &planeData );

			// finish up shader and set it
			planeEffect->EndUpdate();
			planeSet->SetEffect( planeEffect );
			uint32_t index = 0;
			

			planeSet->SetIsSkinned( isSkinned );

			for( auto& offset : offsets )
			{
				// add all individual items
				for( auto& psiit : planeSetData.items )
				{
					// create it
					EvePlaneSetItemPtr planeSetItem;
					planeSetItem.CreateInstance();

					// set up the position data
					Vector3 tmp;
					Matrix m = TransformationMatrix( Vector3( 1.0f, 1.0f, 1.0f ), psiit.rotation, psiit.position + hullOffset ) * offset;
					Decompose( tmp, planeSetItem->m_rotation, planeSetItem->m_position, m );

					// fill it up
					planeSetItem->m_scaling = psiit.scaling;
					planeSetItem->m_layer1Transform = psiit.layer1Transform;
					planeSetItem->m_layer1Scroll = psiit.layer1Scroll;
					planeSetItem->m_layer2Transform = psiit.layer2Transform;
					planeSetItem->m_layer2Scroll = psiit.layer2Scroll;
					planeSetItem->m_boneIndex = psiit.boneIndex;
					planeSetItem->m_maskAtlasID = psiit.maskMapAtlasIndex;
					planeSetItem->m_blinkData = Vector4( psiit.rate, psiit.phase, psiit.dutyCycle, (float)psiit.blinkMode );

					if( dna->UsingSof6() )
					{
						auto colorSet = dna->GetColorSet();
						planeSetItem->m_color = Saturate( psiit.intensity * colorSet[psiit.colorType], psiit.saturation );
					}
					else
					{
						planeSetItem->m_color = psiit.color;
						// groupindex allows to overwrite color
						const EveSOFDataMgr::FactionPlaneSetColorData* factionalData = dna->GetFactionPlaneSetData( psiit.groupIndex );
						if( factionalData )
						{
							planeSetItem->m_color = factionalData->color;
						}
					}

					if( dna->UsingSof6() )
					{

						if( psiit.lights.size() > 0 )
						{
							planeSet->SetImageMapParameter( imageMap );
							planeSet->SetLayerMap1Parameter( layer1 );
							planeSet->SetLayerMap2Parameter( layer2 );
							planeSet->SetMaskMapParameter( mask );
						}

						for( auto& pslight : psiit.lights ) {
							float maxScale = max( psiit.scaling.x, max( psiit.scaling.y, psiit.scaling.z ) );
							auto saturatedColor = Saturate( planeSetItem->m_color, pslight.saturation );
							auto lightData = pslight.AsLightData( saturatedColor, maxScale );
							lightData.position = Transform( lightData.position, RotationMatrix( planeSetItem->m_rotation ) ) + planeSetItem->m_position;
							lightData.rotation = Normalize( lightData.rotation * planeSetItem->m_rotation );
							lightData.boneIndex = planeSetItem->m_boneIndex;

							planeSet->AddLight( EvePlaneLight( lightData, pslight.saturation, index, pslight.lightProfilePath, (EveSpaceObjectAttachmentUtils::FadeType) psiit.blinkMode, psiit.phase, psiit.rate ) );
						}

						index++;
					}
					// add it
					planeSet->AddPlaneItem( planeSetItem );
				}
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
void EveSOF::SetupSpriteLineSets( IEveSpaceObjectAttachmentOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const
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

			// visible?
			if( dna->IsInVisibilityData( spriteLineSetData->visibilityGroup ) )
			{
				// create a spriteset for this ship
				EveSpriteLineSetPtr spriteLineSet;
				spriteLineSet.CreateInstance();
				// set shader
				spriteLineSet->Setup( m_spriteSetEffect, spriteLineSetData->skinned && buildFlags != EveSOFDataHullBuildFilter::INSTANCED_PLACEMENT );
				uint32_t index = 0;

				for( auto& offset : offsets )
				{
					// add all the individual items
					for( auto slsiit = spriteLineSetData->items.begin(); slsiit != spriteLineSetData->items.end(); ++slsiit )
					{
						const EveSOFDataMgr::HullSpriteLineSetItemData* itemData = &( *slsiit );

						// color data?
						const Color* colorSet = dna->GetColorSet();

						// create spritelineset items
						EveSpriteLineSetItemPtr spriteLineSetItem;
						spriteLineSetItem.CreateInstance();

						// set it up the colorset data
						spriteLineSetItem->m_color = itemData->intensity * colorSet[itemData->colorType];
						
						// set up the position data
						Vector3 tmp;
						Matrix m = TransformationMatrix( Vector3(1.0f, 1.0f, 1.0f), itemData->rotation, itemData->position + hullOffset ) * offset;
						Decompose( tmp, spriteLineSetItem->m_rotation, spriteLineSetItem->m_position, m );

						// set it up the per-hull data
						spriteLineSetItem->m_blinkPhaseShift = itemData->blinkPhaseShift;
						spriteLineSetItem->m_blinkPhase = itemData->blinkPhase;
						spriteLineSetItem->m_blinkRate = itemData->blinkRate;
						spriteLineSetItem->m_boneIndex = itemData->boneIndex;
						spriteLineSetItem->m_falloff = itemData->falloff;
						spriteLineSetItem->m_maxScale = itemData->maxScale;
						spriteLineSetItem->m_minScale = itemData->minScale;
						spriteLineSetItem->m_scaling = itemData->scaling;
						spriteLineSetItem->m_spacing = itemData->spacing;
						spriteLineSetItem->m_isCircle = itemData->isCircle;

						if( dna->UsingSof6() )
						{
							spriteLineSetItem->m_color = Saturate( spriteLineSetItem->m_color, itemData->saturation );
							if( itemData->light )
							{
								auto saturatedColor = Saturate( spriteLineSetItem->m_color, itemData->light->saturation );
								auto lightData = itemData->light->AsLightData( saturatedColor, 1.0 );
								lightData.boneIndex = spriteLineSetItem->m_boneIndex;

								Tr2LightProfileResPtr profile = nullptr;
								if( !itemData->light->lightProfilePath.empty() )
								{
									BeResMan->GetResource( itemData->light->lightProfilePath, L"lp", profile );
								}

								unsigned spriteInSetIndex = 0;
								for( auto& pos : spriteLineSetItem->GetPositions() )
								{
									lightData.position = itemData->light->translation + pos + spriteLineSetItem->m_position;
									lightData.rotation = Normalize( lightData.rotation * spriteLineSetItem->m_rotation );

									EveSpriteLight light = EveSpriteLight();
									light.index = index;
									light.lightProfile = profile;
									light.lightData = lightData;
									light.blinkRate = itemData->blinkRate;
									light.blinkPhase = itemData->blinkPhase + itemData->blinkPhaseShift * spriteInSetIndex++;
									light.minScale = itemData->minScale;
									light.maxScale = itemData->maxScale;

									spriteLineSet->AddLight( light );
								}
							}
							index++;
						}

						// put it into spriteset
						spriteLineSet->Add( spriteLineSetItem );
					}
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
void EveSOF::SetupHazeSets( IEveSpaceObjectAttachmentOwnerPtr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const
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
				// set shader, depends on type and skinning
				// TODO: Make this better, this is pretty silly
				switch( hazeSetData->hazeType )
				{
				case EveSOFDataHullHazeSet::TYPE_SPHERICAL:
					if( hazeSetData->skinned && buildFlags != EveSOFDataHullBuildFilter::INSTANCED_PLACEMENT )
					{
						hazeSet->Setup( m_skinnedHazeSetEffectSpherical );					
					}
					else
					{
						hazeSet->Setup( m_hazeSetEffectSpherical );
					}
					break;
				case EveSOFDataHullHazeSet::TYPE_HALFSPHERICAL:
					hazeSet->Setup( m_hazeSetEffectHalfSpherical );
					break;
				}

				uint32_t index = 0;
				for( auto& offset: offsets )
				{
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

						// set up the position data
						Vector3 tmp;
						Matrix m = TransformationMatrix( Vector3( 1.0f, 1.0f, 1.0f ), itemData->rotation, itemData->position + hullOffset ) * offset;
						Decompose( tmp, hazeSetItem->m_rotation, hazeSetItem->m_position, m );

						// set it up the per-hull data
						hazeSetItem->m_scaling = itemData->scaling;
						hazeSetItem->m_boneIndex = itemData->boneIndex;
						hazeSetItem->m_hazeData = Vector4( itemData->hazeFalloff, itemData->sourceSize, itemData->sourceBrightness, itemData->boosterGainInfluence ? 1.f : 0.f );
						
						if( dna->UsingSof6() )
						{
							hazeSetItem->m_color = Saturate( hazeSetItem->m_color, itemData->saturation );
							for( auto& light : itemData->lights )
							{
								auto saturatedColor = Saturate( color, light.saturation );
								Vector3 scale = hazeSetItem->m_scaling;
								float maxScale = max( scale.x, max( scale.y, scale.z ) );

								auto lightData = light.AsLightData( saturatedColor, maxScale );
								lightData.position = Transform( lightData.position, RotationMatrix( hazeSetItem->m_rotation ) ) + hazeSetItem->m_position;
								lightData.rotation = Normalize( lightData.rotation * hazeSetItem->m_rotation );

								lightData.boneIndex = itemData->boneIndex;

								EveHazeSetLight hazeSetLight( lightData, index, light.lightProfilePath, itemData->boosterGainInfluence );

								hazeSet->AddLight( hazeSetLight );
							}
						}
						

						index++;
						// put it into hazeset
						hazeSet->AddHazeItem( hazeSetItem );
					}
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
void EveSOF::SetupBanners( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// cycle over all hulls in the multi-hull list
	Vector3 hullOffset( 0.f, 0.f, 0.f );
	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		auto& hullBanners = dna->GetHullBanners( hullIdx );
		for( auto hbit = begin( hullBanners ); hbit != end( hullBanners ); ++hbit )
		{
			auto& bannerData = *hbit;

			EveBannerSetPtr bannerSet;
			uint32_t index = 0;

			for( auto hbiit = begin( bannerData.items ); hbiit != end( bannerData.items ); ++hbiit )
			{
				auto& itemData = *hbiit;

				if( !dna->IsInVisibilityData( itemData.visibilityGroup ) )
				{
					continue;
				}

				if( !bannerSet )
				{
					bannerSet.CreateInstance();
				}
				
				for( auto& offset : offsets )
				{
					auto modifiedData = EveBannerItem( itemData.item );

					// set up the position data
					Vector3 tmp;
					Matrix m = TransformationMatrix( Vector3( 1.0f, 1.0f, 1.0f ), modifiedData.rotation, modifiedData.position + hullOffset ) * offset;
					Decompose( tmp, modifiedData.rotation, modifiedData.position, m );

					bannerSet->AddBanner( modifiedData );

					LightData data;
					data.position = modifiedData.position;

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
					
					EveBannerLight bannerLight;
					bannerLight.lightData = data;
					bannerLight.saturation = itemData.bannerLight.saturation;
					bannerLight.index = index;
					bannerSet->AddLight( bannerLight );
					index++;
				}
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

			TriTextureParameterPtr imageMap;
			imageMap.CreateInstance();
			imageMap->SetParameterName( BlueSharedString( "ImageMap" ) );
			effect->AddResource( imageMap );

			bannerSet->SetPrimaryTextureParameter( imageMap );

			effect->EndUpdate();
			bannerSet->SetEffect( effect );
			bannerSet->SetKey( int32_t( bannerData.usage ) );
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
			if( bannerData.usage >= 0 && bannerData.usage < EveSOFDataHullBanner::_USAGE_COUNT )
			{
				externalParamName = names[bannerData.usage];
			}

			Tr2ExternalParameterPtr externalParameter;
			externalParameter.CreateInstance();
			externalParameter->SetName( externalParamName );
			externalParameter->SetDestinationObject( imageMap->GetRawRoot() );
			externalParameter->SetDestinationAttribute( "resourcePath" );
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

//
// Description:
//	Set up the bannersets on the space object from the bannerset information from the hull
//
void EveSOF::SetupBannerSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets ) const
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

			// don't need bannersets if they are not in the visibility group or if they don't have any banners
			if( !dna->IsInVisibilityData( bannerSetData.visibilityGroup ) || bannerSetData.bannerTypes.empty() )
			{
				continue;
			}

			for( auto hbtit = begin( bannerSetData.bannerTypes ); hbtit != end( bannerSetData.bannerTypes ); ++hbtit )
			{
				auto usage = hbtit->first;
				auto banners = hbtit->second;
				EveBannerSetPtr bannerSet;
				uint32_t index = 0;

				for( auto b = begin( banners ); b != end( banners ); ++b )
				{
					auto& banner = *b;
					if( !bannerSet )
					{
						bannerSet.CreateInstance();
					}

					for( auto& offset : offsets )
					{
						auto modifiedItem = EveBannerItem( banner.item );

						// set up the position data
						Vector3 tmp;
						Matrix m = TransformationMatrix( Vector3( 1.0f, 1.0f, 1.0f ), modifiedItem.rotation, modifiedItem.position + hullOffset ) * offset;
						Decompose( tmp, modifiedItem.rotation, modifiedItem.position, m );

						bannerSet->AddBanner( modifiedItem );

						if( banner.light ) {

							Vector3 scale = banner.item.scaling;
							float maxScale = max( scale.x, max( scale.y, scale.z ) );
							auto black = Color( 0, 0, 0, 0 );
							auto lightData = banner.light->AsLightData( black, maxScale );
							lightData.position = Transform( lightData.position, RotationMatrix( modifiedItem.rotation ) ) + modifiedItem.position;
							lightData.rotation = Normalize( lightData.rotation * modifiedItem.rotation );
							
							EveBannerLight bannerLight( lightData, banner.light->saturation, index, banner.light->lightProfilePath );

							bannerSet->AddLight( bannerLight );
						}
					}
					index++;
				}

				// if no banners in set, then ignore it...
				if( !bannerSet )
				{
					continue;
				}

				// setup shader
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

				TriTextureParameterPtr imageMap;
				imageMap.CreateInstance();
				imageMap->SetParameterName( BlueSharedString( "ImageMap" ) );
				effect->AddResource( imageMap );

				bannerSet->SetPrimaryTextureParameter( imageMap );

				effect->EndUpdate();
				bannerSet->SetEffect( effect );
				bannerSet->SetKey( int32_t( usage ) );
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
				static_assert( sizeof( names ) / sizeof( names[0] ) == EveSOFDataHullBannerSetItem::_USAGE_COUNT, "Banner usage names mismatch" );

				const char* externalParamName = nullptr;
				if( usage >= 0 && usage < EveSOFDataHullBannerSetItem::_USAGE_COUNT )
				{
					externalParamName = names[usage];
				}


				Tr2ExternalParameterPtr externalParameter;
				externalParameter.CreateInstance();
				externalParameter->SetName( externalParamName );
				externalParameter->SetDestinationObject( imageMap->GetRawRoot() );
				externalParameter->SetDestinationAttribute( "resourcePath" );
				externalParameter->Initialize();
				obj->AddExternalParameter( externalParameter );
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
//   Load Model Curves for rotation and translation, if there are any
// --------------------------------------------------------------------------------
void EveSOF::SetupModelCurves( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Model rotation curve
	const char* rotationCurvePath = dna->GetModelRotationCurvePath();
	if( rotationCurvePath )
	{
		if( auto rotationCurve = BeResMan->LoadObject<ITriQuaternionFunction>( rotationCurvePath ) )
		{
			obj->SetModelRotationCurve( rotationCurve );
		}
	}

	// Model translation curve
	const char* translationCurvePath = dna->GetModelTranslationCurvePath();
	if( translationCurvePath )
	{
		if( auto translationCurve = BeResMan->LoadObject<ITriVectorFunction>( translationCurvePath ) )
		{
			obj->SetModelTranslationCurve( translationCurve );
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
void EveSOF::SetupChildrenAndAnimations( EveSpaceObject2Ptr obj, IEveEffectChildrenOwnerPtr childOwner, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	auto postCopy = m_editorMode ? &PostCopyMetadata : nullptr;

	std::map<int, std::vector<EveTransformPtr>> childrenToBindTo;
	std::map<int, std::vector<IEveSpaceObjectChildPtr>> soChildrenToBindTo;

	const std::vector<EveSOFDataMgr::HullChild>& hullChildren = dna->GetHullChildren();
	for( auto childIt = hullChildren.begin(); childIt != hullChildren.end(); ++childIt )
	{
		if( ( childIt->buildFilter & buildFlags ) == 0 )
		{
			continue;
		}

		const EveSOFDataMgr::FactionChildData* fcd = dna->GetFactionChildData( childIt->groupIndex );

		// child can be invisibe for this faction
		if( fcd && !fcd->isVisible )
		{
			continue;
		}

		IRootPtr p = BeResMan->LoadObject( childIt->redFilePath.c_str() );
		if( !p )
		{
			CCP_LOGERR( "resource file %s is invalid!", childIt->redFilePath.c_str() );
			continue;
		}

		// is it of right type?
		if( EveTransformPtr child = BlueCastPtr( p ) )
		{
			for(auto& offset: offsets)
			{
				EveTransformPtr transformedChild = BlueCastPtr( BlueCopy( child ) );

				// set up the position data
				Vector3 tmp, pos;
				Quaternion rot;
				Matrix m = TransformationMatrix( Vector3( 1.0f, 1.0f, 1.0f ), childIt->rotation, childIt->translation ) * offset;
				Decompose( tmp, rot, pos, m );
				transformedChild->SetRotation( rot );
				transformedChild->SetScaling( childIt->scaling );
				transformedChild->SetTranslation( pos );
				if( childIt->id != -1 )
				{
					childrenToBindTo[childIt->id].push_back( transformedChild );
				}
			
				obj->AddToChildrenList( transformedChild );
			}		
		}
		else if( IEveSpaceObjectChildPtr effectChild = BlueCastPtr( p ) )
		{
			size_t index = 0;
			for( auto& offset : offsets )
			{
				IEveSpaceObjectChildPtr transformedChild;
				if( ++index < offsets.size() )
				{
					CCP_STATS_ZONE( "Child Copy" );
					transformedChild = BlueCastPtr( BlueCopy( effectChild, nullptr, nullptr, postCopy ) );
				}
				else
				{
					transformedChild = effectChild;
				}

				// set up the position data
				Vector3 tmp, pos;
				Quaternion rot;
				Matrix m = TransformationMatrix( Vector3( 1.0f, 1.0f, 1.0f ), childIt->rotation, childIt->translation ) * offset;
				Decompose( tmp, rot, pos, m );

				transformedChild->Setup( &childIt->scaling, &rot, &pos, childIt->lowestLodVisible );

				transformedChild->SetOrigin( IEveSpaceObjectChild::SOF );
				childOwner->AddToEffectChildrenList( transformedChild );
				if( childIt->id != -1 )
				{
					soChildrenToBindTo[childIt->id].push_back( transformedChild );
				}
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
void EveSOF::SetupEffectChildren( EveSpaceObject2Ptr newObj, IEveEffectChildrenOwnerPtr childOwner, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t buildFlags ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );
	// Experimental features for PHASE-6
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
			if( ( childSetItem.buildFilter & buildFlags ) == 0 )
			{
				continue;
			}

			IRootPtr p = BeResMan->LoadObject( childSetItem.redFilePath.c_str() );
			if( !p )
			{
				CCP_LOGERR( "resource file %s is invalid!", childSetItem.redFilePath.c_str() );
				continue;
			}

			// is it of right type?
			if( EveTransformPtr child = BlueCastPtr( p ) )
			{
				for( auto& offset : offsets )
				{
					EveTransformPtr transformedChild;
					BeClasses->CopyTo( child->GetRawRoot(), (IRoot**)&transformedChild );

					// set up the position data
					Vector3 tmp, pos;
					Quaternion rot;
					Matrix m = TransformationMatrix( Vector3( 1.0f, 1.0f, 1.0f ), childSetItem.rotation, childSetItem.translation ) * offset;
					Decompose( tmp, rot, pos, m );
					transformedChild->SetRotation( rot );
					transformedChild->SetScaling( childSetItem.scaling );
					transformedChild->SetTranslation( pos );
					newObj->AddToChildrenList( transformedChild );
				}
			}
			else if( IEveSpaceObjectChildPtr effectChild = BlueCastPtr( p ) )
			{
				for( auto& offset : offsets )
				{
					IEveSpaceObjectChildPtr transformedChild;
					BeClasses->CopyTo( effectChild->GetRootObject(), (IRoot**)&transformedChild );

					// set up the position data
					Vector3 tmp, pos;
					Quaternion rot;
					Matrix m = TransformationMatrix( Vector3( 1.0f, 1.0f, 1.0f ), childSetItem.rotation, childSetItem.translation ) * offset;
					Decompose( tmp, rot, pos, m );

					transformedChild->Setup( &childSetItem.scaling, &rot, &pos, childSetItem.lowestLodVisible );

					transformedChild->SetOrigin( IEveSpaceObjectChild::SOF );
					childOwner->AddToEffectChildrenList( transformedChild );
				}
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
void EveSOF::SetupControllers( ITr2ControllerOwnerPtr owner, const EveSOFDNAPtr dna, uint32_t buildFlags ) const
{
	auto& hullControllers = dna->GetHullControllers();
	for( auto cit = begin( hullControllers ); cit != end( hullControllers ); ++cit )
	{
		if( ( buildFlags & cit->buildFilter ) == 0 )
		{
			continue;
		}

		if( auto controller = BeResMan->LoadObject<ITr2Controller>( cit->path.c_str() ) )
		{
			owner->AddController( controller );
		}
		else
		{
			CCP_LOGERR( "controller resource file %s is invalid!", cit->path.c_str() );
		}
	}
}


// --------------------------------------------------------------------------------
// Description:
//   add audio children to the space object
// --------------------------------------------------------------------------------
void EveSOF::SetupAudio( ITr2SoundEmitterOwnerPtr newObj, const EveSOFDNAPtr dna, const Matrix& parentOffset ) const
{
	auto& hullEmitters = dna->GetHullSoundEmitters();

	Quaternion parentRotation;
	Vector3 tmp, tmp2;
	Decompose( tmp, parentRotation, tmp2, parentOffset );

	for( auto cit = begin( hullEmitters ); cit != end( hullEmitters ); ++cit )
	{
		TriObserverLocalPtr observer;
		observer.CreateInstance();
		observer->m_name = cit->name;
		observer->SetPosition( XMVector3TransformCoord(cit->position, parentOffset) );

		// Convert rotation to a front value
		Vector3 front;
		static const Vector3 zAxis( 0.f, 0.f, 1.f );
		Quaternion rot = cit->rotation * parentRotation;
		TriVectorRotateQuaternion( &front, &zAxis, &rot);

		observer->SetFront( front );

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
			theRealEmitter->Initialize( cit->name.c_str(), cit->prefix.c_str(), XMVector3TransformCoord( cit->position, parentOffset ) );
			theRealEmitter->SetAttenuationScalingFactor( cit->attenuationScalingFactor );
		}
	}
}


// --------------------------------------------------------------------------------
// Description:
//   helper method to create instanced child meshes
// --------------------------------------------------------------------------------
Tr2InstancedMeshPtr EveSOF::CreateInstancedMesh( std::vector<EveSOFDataMgr::HullMeshInstance> instances, std::string resPath ) const
{
	Tr2InstancedMeshPtr mesh;
	mesh.CreateInstance();

	Tr2VertexDefinition instanceDef;
	instanceDef.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 0 ); // transform0
	instanceDef.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 1 ); // transform1
	instanceDef.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 2 ); // transform2
	instanceDef.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 3 ); // lastTransform0
	instanceDef.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 4 ); // lastTransform1
	instanceDef.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 5 ); // lastTransform2
	instanceDef.Add( Tr2VertexDefinition::BYTE_4, Tr2VertexDefinition::TEXCOORD, 6 ); // boneindex

	Tr2RuntimeInstanceDataPtr instanceData;
	instanceData.CreateInstance();
	instanceData->SetLayout( instanceDef );
	auto dest = instanceData->GetData( unsigned( instances.size() ) );
	memcpy( dest, &instances[0], sizeof( instances[0] ) * instances.size() );
	instanceData->UpdateData();

	float maxScale = 0;
	CcpMath::AxisAlignedBox aabb;
	for( auto& instance : instances )
	{
		aabb.Include( Vector3( instance.transform0.w, instance.transform1.w, instance.transform2.w ) );
		maxScale = std::max( maxScale, LengthSq( instance.transform0.GetXYZ() ) );
		maxScale = std::max( maxScale, LengthSq( instance.transform1.GetXYZ() ) );
		maxScale = std::max( maxScale, LengthSq( instance.transform2.GetXYZ() ) );
	}
	instanceData->SetBoundingBox( aabb );
	mesh->SetDynamicScaledBounds( sqrt( maxScale ) );
	
	mesh->SetInstanceGeometryRes( instanceData );
	mesh->SetMeshResPath( resPath.c_str() );

	return mesh;
}


// --------------------------------------------------------------------------------
// Description:
//   helper method to fill shaders of a mesh
// --------------------------------------------------------------------------------
void EveSOF::SetupShaders( const EveSOFDNAPtr dna, Tr2MeshBase* mesh ) const
{
	if( mesh == nullptr )
	{
		return;
	}
	// multi-hull! so mesh index must be tracked
	size_t meshIndexOffset = 0;
	// cycle over all hulls in the multi-hull list
	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		size_t cntr = 0;
		cntr += FillMeshAreaVector( mesh->GetAreas( TRIBATCHTYPE_OPAQUE ), TRIBATCHTYPE_OPAQUE, dna, hullIdx, meshIndexOffset );
		cntr += FillMeshAreaVector( mesh->GetAreas( TRIBATCHTYPE_OPAQUE ), TRIBATCHTYPE_DECAL, dna, hullIdx, meshIndexOffset );
		cntr += FillMeshAreaVector( mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT ), TRIBATCHTYPE_TRANSPARENT, dna, hullIdx, meshIndexOffset );
		cntr += FillMeshAreaVector( mesh->GetAreas( TRIBATCHTYPE_ADDITIVE ), TRIBATCHTYPE_ADDITIVE, dna, hullIdx, meshIndexOffset );
		cntr += FillMeshAreaVector( mesh->GetAreas( TRIBATCHTYPE_DISTORTION ), TRIBATCHTYPE_DISTORTION, dna, hullIdx, meshIndexOffset );

		auto distortionAreas = mesh->GetAreas( TRIBATCHTYPE_DISTORTION );
		for( auto ait = begin( *distortionAreas ); ait != end( *distortionAreas ); ++ait )
		{
			( *ait )->SetMinLod( TR2_LOD_HIGH );
		}
		meshIndexOffset += cntr;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   add instanced meshes to the ship
// --------------------------------------------------------------------------------
void EveSOF::SetupInstancedMeshes( EveSpaceObject2Ptr newObj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets ) const
{
	const std::vector<EveSOFDataMgr::HullInstancedMesh>& hullInstanced = dna->GetHullInstancedMeshes();
	
	if( hullInstanced.empty() )
	{
		// No need to do anything!
		return;
	}

	// Create a child container named "Instanced Meshes" and place the instances there
	EveChildContainerPtr meshContainer;
	meshContainer.CreateInstance();
	meshContainer->SetName( "Instanced Meshes" );

	for( auto instIt = hullInstanced.begin(); instIt != hullInstanced.end(); ++instIt )
	{
		const EveSOFDataMgr::HullInstancedMesh* him = &(*instIt);
		if( him->instances.empty() )
		{
			continue;
		}
		
		EveChildMeshPtr childMesh;
		childMesh.CreateInstance();

		std::vector<EveSOFDataMgr::HullMeshInstance> instances;
		// propogate the instances to the offsets and resize the bounds
		if( offsets.size() > 1 || offsets[0] != IdentityMatrix() )
		{
			instances.reserve( offsets.size() * him->instances.size() );
			for( auto &offset : offsets )
			{
				std::transform( ( him->instances ).begin(), ( him->instances ).end(), std::back_inserter( instances ), [offset]( EveSOFDataMgr::HullMeshInstance instance ) -> EveSOFDataMgr::HullMeshInstance {
					
					EveSOFDataMgr::HullMeshInstance i( instance );
					
					Matrix m = Matrix( i.transform0.x, i.transform1.x, i.transform2.x, 0, 
									   i.transform0.y, i.transform1.y, i.transform2.y, 0, 
									   i.transform0.z, i.transform1.z, i.transform2.z, 0, 
									   i.transform0.w, i.transform1.w, i.transform2.w, 1);
					
					m = Transpose( m * offset );
					i.transform0 = *reinterpret_cast<Vector4*>( &m.GetX() );
					i.transform1 = *reinterpret_cast<Vector4*>( &m.GetY() );
					i.transform2 = *reinterpret_cast<Vector4*>( &m.GetZ() );

					i.lastTransform0 = *reinterpret_cast<Vector4*>( &m.GetX() );
					i.lastTransform1 = *reinterpret_cast<Vector4*>( &m.GetY() );
					i.lastTransform2 = *reinterpret_cast<Vector4*>( &m.GetZ() );

					return i;
				} );
			}
		}
		else
		{
			instances = him->instances;
		}

		auto instancedMesh = CreateInstancedMesh( instances, him->geometryResPath );

		Tr2MeshAreaVector* areas = instancedMesh->GetAreas( TRIBATCHTYPE_OPAQUE );

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
				dna->ModifyTextureResPath( resPath, m_allowFileCaching ? &m_existingFilesCache : nullptr );
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

		childMesh->SetMesh( instancedMesh );
		childMesh->SetMinScreenSize( MIN_INSTANCED_MESH_SCREEN_SIZE );
		childMesh->SetCastShadow( dna->CastShadow() );
		childMesh->Setup( nullptr, nullptr, nullptr, him->lowestLodVisible );

		if( him->displayModifier != EveChildContainer::DisplayQualityModifier::SHADER_ALL )
		{
			// need to create an eveChildContainer with the display quality modifier
			EveChildContainerPtr qualityControl;
			qualityControl.CreateInstance();
			qualityControl->SetName( "Shader Quality Controlled Instanced Mesh" );
			qualityControl->Setup( nullptr, nullptr, nullptr, him->lowestLodVisible );
			qualityControl->AddToEffectChildrenList( childMesh );
			qualityControl->SetDisplayQualityModifier( (EveChildContainer::DisplayQualityModifier) him->displayModifier );

			// and add it to the main mesh container
			meshContainer->AddToEffectChildrenList( qualityControl );
		}
		else
		{
			meshContainer->AddToEffectChildrenList( childMesh );
		}
	}
	newObj->AddToEffectChildrenList( static_cast<IEveSpaceObjectChild*>( meshContainer) );
}

// --------------------------------------------------------------------------------
// Description:
//   Add the ppt to the ship
// --------------------------------------------------------------------------------
void EveSOF::SetupCustomMask( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
    bool deletePatterData = false;
	const EveSOFDataMgr::PatternApplicationData* data = dna->GetPatternApplicationData( deletePatterData );

	if( data )
	{
		size_t layerCount = dna->GetPatternLayerCount();
		for( size_t i = 0; i < layerCount; ++i )
		{
			auto patternProjectionData = dna->GetPatternProjectionData( data, i );

			if( patternProjectionData && patternProjectionData->enabled )
			{
				const EveSOFDataMgr::PatternLayerData* patternLayerData = dna->GetPatternLayerData( data, i );

				if( patternLayerData )
				{
					Vector4 materialTargets = dna->GetMaterialTargets( patternLayerData );

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
						materialTargets );
					obj->AddCustomMask( customMask );
				}
			}
		}
        if( deletePatterData )
        {
            delete data;
        }
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Add all kinds of effects to the ship
// --------------------------------------------------------------------------------
void EveSOF::SetupImpactEffects( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna ) const
{
	EveSOFDataHull::ImpactEffectType impactType = dna->GetImpactEffectType();
	// todo - how in the hell do we create impact effects for instanced shit?

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

			impactOverlay->SetDamageLocatorCount( dna->GetLocatorCount( "damage" ) );

			// shield impact effect via Tr2Mesh with LOD
			Tr2MeshPtr shieldMesh;

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

				shieldMesh.CreateInstance();
				std::string geometryPath;
				// what type of shield effect determines the geometry resource
				if( impactType == EveSOFDataHull::IMPACTEFFECT_ELLIPSOID )
				{
					// only the ellpisoid geometry
					geometryPath = genericDamageData->shieldGeometryResFilePath;
				}
				else if( impactType == EveSOFDataHull::IMPACTEFFECT_HULL )
				{
					// use the ships main geometry incl LODs
					geometryPath = dna->GetHullGeometryResPath();
					// adjust mesharea count to that from the mesh
					meshArea->SetCount( 1 + dna->GetHighestMeshAreaIndex( TRIBATCHTYPE_OPAQUE ) );
				}

				shieldMesh->SetMeshResPath( geometryPath.c_str() );

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
void EveSOF::SetupLights( ITr2LightOwnerPtr spaceObject, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets ) const
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
				for( auto& offset : offsets )
				{
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

					// set up the position data
					Vector3 tmp;
					Matrix m = TransformationMatrix( Vector3( 1.0f, 1.0f, 1.0f ), lightSetItem.rotation, lightSetItem.position + hullOffset) * offset;
					Decompose( tmp, data.rotation, data.position, m );

					data.radius = lightSetItem.radius;
					data.innerRadius = lightSetItem.innerRadius;
					data.flags = lightSetItem.flags;
					data.color = dna->GetColorSet()[lightSetItem.lightColor];
					data.brightness = lightSetItem.brightness;
					data.noiseAmplitude = lightSetItem.noiseAmplitude;
					data.noiseFrequency = lightSetItem.noiseFrequency;
					data.noiseOctaves = lightSetItem.noiseOctaves;
					data.innerAngle = lightSetItem.innerAngle;
					data.outerAngle = lightSetItem.outerAngle;
					data.texturePath = lightSetItem.texturePath;
					data.boneIndex = lightSetItem.boneIndex;
					light->SetLightData( data );

					spaceObject->AddLight( light );
				}
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

		// add all the indiviual items and make a locatorSet marking booster locations
		std::vector<EveSOFDataMgr::LocatorDirectionData> locators;
		locators.reserve(hdata->items.size());
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

			EveSOFDataMgr::LocatorDirectionData data;
			Vector3 scaling;
			Decompose( scaling, data.rotation, data.position, biit->transform );
			data.boneIndex = -1;
			locators.push_back( data );
		}
		glow->Rebuild();

		EveLocatorSetsPtr locSet;
		std::vector<Locator> locatorList;
		locSet.CreateInstance();
		locatorList.reserve( locators.size() );
		
		for( auto locator : locators )
		{
			Locator loc = locator;  
			locatorList.push_back( loc );
		}

		locSet->Set( "boosters", locatorList.data(), locators.size() );
		locSet->Translate( hullOffset );
		ship->MergeToLocatorSet( *locSet );

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
void EveSOF::SetupDecalSets( IEveSpaceObjectDecalOwnerPtr obj, const EveSOFDNAPtr dna ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// cycle over all hulls in the multi-hull list
	Vector3 hullOffset( 0.f, 0.f, 0.f );

	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		const std::vector<EveSOFDataMgr::HullDecalSetData>& hullDecalSets = dna->GetHullDecalSets( hullIdx );

		for( const auto &hullDecalSet : hullDecalSets )
		{
			if( !dna->IsInVisibilityData( hullDecalSet.visibilityGroup ) )
			{
				continue;
			}

			for( const auto& itemData : hullDecalSet.items )
			{
				// if we have a logo decal but the faction doesn't have a that logo in the logo set then we can skip this decal
				if( itemData.usage == EveSOFDataHullDecalSetItem::USAGE_LOGO && !dna->HasLogoSet( itemData.logoType ) )
				{
					continue;
				}

				// create
				EveSpaceObjectDecalPtr decal;
				decal.CreateInstance();
				
				// set general datas
				decal->SetPosition( itemData.position + hullOffset );
				decal->SetRotation( itemData.rotation );
				decal->SetScaling( itemData.scaling );
				decal->SetBoneIndex( itemData.boneIndex );
				decal->SetMinScreenSize( MIN_DECAL_SCREEN_SIZE );

				// pre-calculated index buffer is only valid for first multi-hull
				if( itemData.indexBuffers.empty() || hullIdx != 0 )
				{
					decal->SetIndices( {} );
				}
				else
				{
					decal->SetIndices( itemData.indexBuffers );
				}

				// the decal effect
				Tr2EffectPtr shader;
				shader.CreateInstance();
				shader->StartUpdate();


				// construct shader path and set it on the Tr2Effect
				std::string shaderPath = dna->GetDecalShaderLocationResPath() + std::string( "/" ) + dna->GetShaderPrefix( false ) + m_decalsEffectName[itemData.usage].c_str();
				shader->SetEffectPathName( shaderPath.c_str() );
				
				// Set the glow color based on the colors in the colorsetuS
				if( itemData.usage != EveSOFDataHullDecalSetItem::USAGE_LOGO && itemData.usage != EveSOFDataHullDecalSetItem::USAGE_STANDARD )
				{
					Color decalGlowColor = dna->GetColorSet()[itemData.glowColorType];
					shader->AddParameterColor( BlueSharedString( "DecalGlowColor" ), &decalGlowColor );
				}

				// always set hull parameters & textures for this decal
				for( auto hdpit = itemData.parameters.begin(); hdpit != itemData.parameters.end(); ++hdpit )
				{
					shader->AddParameterVector4( hdpit->first, &hdpit->second );
				}
				for( auto hdtit = itemData.textures.begin(); hdtit != itemData.textures.end(); ++hdtit )
				{
					shader->AddResourceTexture2D( hdtit->first, hdtit->second.resFilePath.c_str() );
				}

				// find data on this shader from generics, we need it!
				const EveSOFDataMgr::GenericDecalShaderData* shaderData = dna->GetGenericDecalShaderData( m_decalsEffectName[itemData.usage] );
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
						if( itemData.meshIndex != -1 )
						{
							// get the filepath from the hull
							std::string resFilePath;
							if( dna->GetHullTextureWithMeshIndex( resFilePath, *ptit, itemData.meshIndex, hullIdx, m_allowFileCaching ? &m_existingFilesCache : nullptr ) )
							{
								shader->AddResourceTexture2D( *ptit, resFilePath.c_str() );
							}
						}
					}

					if( shaderData->additive )
					{
						decal->SetBatchType( TRIBATCHTYPE_DECAL_ADDITIVE );
					}
				}
				
				// Set the logo from the logoset
				if( itemData.usage == EveSOFDataHullDecalSetItem::USAGE_LOGO )
				{
					const EveSOFDataMgr::LogoData* logo = dna->GetLogo( itemData.logoType );
					if( logo )
					{
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
//   add the hull locator sets to the new ship
// --------------------------------------------------------------------------------
void EveSOF::SetupLocatorSets( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// cycle over all hulls in the multi-hull list
	Vector3 hullOffset( 0.f, 0.f, 0.f );
	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		// locator sets (like damage, explosions, etc.)
		const std::vector<BlueSharedString> locatorSetsNames = dna->GetHullLocatorSetNames( hullIdx );
		for( auto locatorSetName = locatorSetsNames.begin(); locatorSetName != locatorSetsNames.end(); ++locatorSetName )
		{
			auto locators = dna->GetHullLocators( locatorSetName->c_str(), hullIdx );
			if( locators )
			{
				EveLocatorSetsPtr locSets;
				locSets.CreateInstance();

				if( !offsets.empty() )
				{
					// add the distributed locators
					// need to move the locators by the hullOffset
					std::vector<EveSOFDataMgr::LocatorDirectionData> distributedLocators;
					distributedLocators.reserve( offsets.size() * locators->size() );
					for( auto& offset : offsets )
					{
						std::transform( ( *locators ).begin(), ( *locators ).end(), std::back_inserter( distributedLocators ), [offset, hullOffset]( EveSOFDataMgr::LocatorDirectionData d ) -> EveSOFDataMgr::LocatorDirectionData {
							Matrix m = TransformationMatrix( Vector3( 1.0, 1.0, 1.0 ), d.rotation, d.position + hullOffset ) * offset;
							Vector3 tmp;
							Decompose( tmp, d.rotation, d.position, m );
							return d;
						} );
					}
					locSets->Set( locatorSetName->c_str(), distributedLocators.data(), distributedLocators.size() );
				}
				obj->MergeToLocatorSet( *locSets );
			}
		}
	}
}

void EveSOF::SetupLayout( EveSpaceObject2Ptr obj, const EveSOFDNAPtr dna, const std::vector<Matrix>& offsets, uint32_t seedOverwrite )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	std::map<BlueSharedString, std::vector<EveSOFDataMgr::LocatorDirectionData>> locatorSets;
	// map out all possible locators that the layouts can utilize
	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		const std::vector<BlueSharedString> locatorSetNames = dna->GetHullLocatorSetNames( hullIdx );

		for(const auto & locatorSetName : locatorSetNames)
		{
			const std::vector<EveSOFDataMgr::LocatorDirectionData>* locators = dna->GetHullLocators( locatorSetName.c_str(), hullIdx );
			std::vector<EveSOFDataMgr::LocatorDirectionData> locatorsCopy( *locators );

			auto existingSet = locatorSets.find( locatorSetName );
			if( existingSet == locatorSets.end())
			{
				locatorSets.insert({locatorSetName, locatorsCopy} );
			}
			else
			{
				locatorSets[locatorSetName].reserve( locatorSets[locatorSetName].size() + locatorsCopy.size() );
				locatorSets[locatorSetName].insert( locatorSets[locatorSetName].end(), locatorsCopy.begin(), locatorsCopy.end() );
			}
		}
	}

	// dna can have multiple layouts
	for( size_t layoutIdx = 0; layoutIdx < dna->GetLayoutCount(); ++layoutIdx )
	{
		auto layout = dna->GetLayoutData( layoutIdx );
		size_t placementIdx = 0;

		uint32_t oldSeed = TriRandGetSeed();
		uint32_t trinitySeed = layout->seed;

		if( layout->scrambleSeed )
		{
			trinitySeed += uint32_t( BeOS->GetActualTime() );
		}

		trinitySeed += seedOverwrite;

		TriSrand( trinitySeed );

		// Go over all the placements (each layout can have multiple mesh attachments)
		for( auto placement : layout->placements )
		{
			ProcessPlacementDistributionOrGroup( placement, obj, dna, locatorSets, layoutIdx, placementIdx, offsets );
		}
		
		if( layout->scrambleSeed )
		{
			TriSrand( oldSeed + seedOverwrite ); // restore seed if scrambled for parent proceduralness
		}
	}
}

void EveSOF::ProcessPlacementDistributionOrGroup( EveSOFDataMgr::ExtensionPlacementData& placement,
												  EveSpaceObject2Ptr obj,
												  const EveSOFDNAPtr dna,
												  std::map<BlueSharedString, std::vector<EveSOFDataMgr::LocatorDirectionData>>& managedLocatorSets,
												  size_t& layoutIdx,
												  size_t& placementIdx,
												  const std::vector<Matrix>& offsets )
{
	if( placement.isAGroup )
	{
		if( !placement.enabled )
		{
			placementIdx += placement.placements.size();
			return;
		}

		if( !ProcessLayoutDistributionConditions( placement, dna ) )
		{
			placementIdx += placement.placements.size();
			return;
		}

		// Go over all the placements (each layout can have multiple mesh attachments)
		for( auto& placement : placement.placements )
		{
			ProcessPlacementDistributionOrGroup( placement, obj, dna, managedLocatorSets, layoutIdx, placementIdx, offsets );
		}
		return;
	}

	if( !placement.enabled )
	{
		placementIdx++;
		return;
	}

	auto locators = std::vector<EveSOFDataMgr::LocatorDirectionData>();

	Vector3 hullOffset( 0, 0, 0 );

	// Need to go over all the hulls
	for( size_t hullIdx = 0; hullIdx < dna->GetMultiHullCount(); ++hullIdx )
	{
		auto hullLocators = dna->GetPlacementLocators( hullIdx, placement.locatorSetName );
		if( hullLocators && !hullLocators->empty() )
		{
			std::vector<EveSOFDataMgr::LocatorDirectionData> placementLocators( *hullLocators );

			for( int32_t index = 0; index < placementLocators.size(); index++ )
			{
				bool inBoth = false;
				int32_t pID = placementLocators.at( index ).uniqueID;
				for( auto mLoc : managedLocatorSets[placement.locatorSetName] )
				{
					if( mLoc.uniqueID == pID )
					{
						inBoth = true;
						break;
					}
				}

				if( !inBoth )
				{
					std::swap( placementLocators.at( index ), placementLocators.back() );
					placementLocators.pop_back();
					index--;
				}
			}

			if( !placementLocators.empty() )
			{
				if( hullIdx == 0 )
				{
					locators.insert( locators.end(), placementLocators.begin(), placementLocators.end() );
				}
				else
				{
					// need to move the locators by the hullOffset
					std::transform( placementLocators.begin(), placementLocators.end(), std::back_inserter( locators ), [hullOffset]( EveSOFDataMgr::LocatorDirectionData d ) -> EveSOFDataMgr::LocatorDirectionData {
						d.position += hullOffset;
						return d;
					} );
				}
			}
		}

		const Vector3* nextSubsystemOffset = dna->GetHullNextSubsystemOffset( hullIdx );
		if( nextSubsystemOffset )
		{
			hullOffset += *nextSubsystemOffset;
		}
	}

	if( locators.empty() )
	{
		placementIdx++;
		return;
	}


	if( !ProcessLayoutDistributionConditions( placement, dna ) )
	{
		placementIdx++;
		return;
	}

	if( placement.hasDistribution )
	{
		ProcessLayoutDistributionDistribute( placement.distribution, dna, locators, managedLocatorSets[placement.locatorSetName] );
	}

	EveSOFDNAPtr placementDna;
	placementDna.CreateInstance();
	auto layout = dna->GetLayoutData(layoutIdx);
	placementDna->Setup( layout->name, placement.descriptor, dna, &m_dataMgr );
	if( placement.isInstanced )
	{
		placementDna->DisableAnimation();
	}

	if( !locators.empty() )
	{
		if( !placement.descriptor.layout.empty() )
		{
			// for sub-layouts to have their own random generation instead of sharing the same
			std::vector<EveSOFDataMgr::LocatorDirectionData> singleLocator(1);
			for( auto &locator : locators )
			{
				singleLocator[0] = locator;
				CreatePlacement( obj, placementDna, placement, singleLocator, offsets );
			}
		}
		else
		{
			CreatePlacement( obj, placementDna, placement, locators, offsets );
		}
	}

	placementIdx++;
}
 

bool EveSOF::ProcessLayoutDistributionConditions( EveSOFDataMgr::ExtensionPlacementData& placement,
												  const EveSOFDNAPtr dna )
{
	bool distributionSuccessful = true;
	for( auto condition : placement.placementConditions )
	{
		switch( condition.distributionType )
		{
		case EveSOFDataMgr::RANDOM_INCLUCION: {
			distributionSuccessful = distributionSuccessful && condition.triggerChance > TriRand();
			break;
		}
		case EveSOFDataMgr::PARENT_MATCH: {
			
			bool hull = condition.spaceObjectParentDescriptor.hull.empty() ? true : true; // TODO condition.spaceObjectParentDescriptor.hull == dna->;
			bool faction = condition.spaceObjectParentDescriptor.faction.empty() ? true : condition.spaceObjectParentDescriptor.faction == dna->GetFactionName();
			bool race = condition.spaceObjectParentDescriptor.race.empty() ? true : condition.spaceObjectParentDescriptor.race == dna->GetRaceName();
			bool pattern = condition.spaceObjectParentDescriptor.pattern.empty() ? true : true; // TODO condition.spaceObjectParentDescriptor.pattern == dna->patter;
			bool material1 = condition.spaceObjectParentDescriptor.material1.empty() ? true : true; // TODO condition.spaceObjectParentDescriptor.material1 == placement.descriptor.material1;
			bool material2 = condition.spaceObjectParentDescriptor.material2.empty() ? true : true; // TODO  condition.spaceObjectParentDescriptor.material2 == placement.descriptor.material2;
			bool material3 = condition.spaceObjectParentDescriptor.material3.empty() ? true : true; // TODO condition.spaceObjectParentDescriptor.material3 == placement.descriptor.material3;
			bool material4 = condition.spaceObjectParentDescriptor.material4.empty() ? true : true; // TODO condition.spaceObjectParentDescriptor.material4 == placement.descriptor.material4;
			bool layout = condition.spaceObjectParentDescriptor.layout.empty() ? true : true; // TODO condition.spaceObjectParentDescriptor.layout == placement.descriptor.layout;
			distributionSuccessful = distributionSuccessful && hull && faction && race && pattern && material1 && material2 && material3 && material4 && layout;
			break;
		}
		case EveSOFDataMgr::DEPLETION_COUNTER: {
			// TODO
			break;
		}
		case EveSOFDataMgr::GRAPHIC_SETTING_MAP: {
			TR2SHADERMODEL settings = Tr2Renderer::GetShaderModel();

			switch( condition.displayModifier )
			{
			case EveSOFDataHullExtensionPlacementDistributionMapGraphicSettings::SHADER_LOW:
				distributionSuccessful = distributionSuccessful && settings == TR2SM_3_0_LO;
				break;
			case EveSOFDataHullExtensionPlacementDistributionMapGraphicSettings::SHADER_LOWMID:
				distributionSuccessful = distributionSuccessful && settings <= TR2SM_3_0_HI;
				break;
			case EveSOFDataHullExtensionPlacementDistributionMapGraphicSettings::SHADER_MED:
				distributionSuccessful = distributionSuccessful && settings == TR2SM_3_0_HI;
				break;
			case EveSOFDataHullExtensionPlacementDistributionMapGraphicSettings::SHADER_HIGHMID:
				distributionSuccessful = distributionSuccessful && settings >= TR2SM_3_0_HI;
				break;
			case EveSOFDataHullExtensionPlacementDistributionMapGraphicSettings::SHADER_HIGH:
				distributionSuccessful = distributionSuccessful && settings == TR2SM_3_0_DEPTH;
				break;
			case EveSOFDataHullExtensionPlacementDistributionMapGraphicSettings::SHADER_ALL:
				break;
			case EveSOFDataHullExtensionPlacementDistributionMapGraphicSettings::ONLY_REFLECTIONS:
				distributionSuccessful = false;
				break;
			default:
				distributionSuccessful = false;
				break;
			}
			break;
		}
		default:
			break;
		}

		if (!distributionSuccessful)
		{
			return false;
		}
	}

	for( auto condition : placement.placementConditions )
	{
	    if( condition.distributionType == EveSOFDataMgr::DEPLETION_COUNTER)
		{
			// consume counters if enabled
			// TODO
		}
	}

	return true;
}




void EveSOF::ProcessLayoutDistributionDistribute( EveSOFDataMgr::ExtensionPlacementDistribution& distributionData,
										const EveSOFDNAPtr dna,
										std::vector<EveSOFDataMgr::LocatorDirectionData>& placementSet,
										std::vector<EveSOFDataMgr::LocatorDirectionData>& managedLocatorSet )
{
	float preCount = float( placementSet.size() ) * ( 1.f - distributionData.completeness );
	float remainder = fmod( preCount, 1.f );

	int32_t count = int( preCount ) + int( remainder + TriRand() );

	if( distributionData.cap > 0 )
	{
		count = max( count, int( placementSet.size() ) - distributionData.cap );
	}

	if( count > 0 )
	{
		std::map<int, Vector4> rankedLocators; // uniqueID, sorted rankings where =Vector4(Xaxis, Yaxis, Zaxis, distFromCenter)
		for( auto locator : placementSet )
		{
			rankedLocators.insert({locator.uniqueID, Vector4(0,0,0,0)});
		}

		if( distributionData.placementBias.x != 0.f )
		{
			std::sort(placementSet.begin(), placementSet.end(),[] (auto &a,auto &b) { return a.position.x < b.position.x; });
			for( size_t i = 0; i < placementSet.size(); i++) rankedLocators[placementSet[i].uniqueID].x = float(i);
		}
		if( distributionData.placementBias.y != 0.f )
		{
			std::sort(placementSet.begin(), placementSet.end(),[] (auto &a,auto &b) { return a.position.y < b.position.y; });
			for( size_t i = 0; i < placementSet.size(); i++) rankedLocators[placementSet[i].uniqueID].y = float(i);
		}
		if( distributionData.placementBias.z != 0.f )
		{
			std::sort(placementSet.begin(), placementSet.end(),[] (auto &a,auto &b) { return a.position.z < b.position.z; });
			for( size_t i = 0; i < placementSet.size(); i++) rankedLocators[placementSet[i].uniqueID].z = float(i);
		}
		if( distributionData.centerBias != 0.f )
		{
			std::sort(placementSet.begin(), placementSet.end(),[] (auto &a,auto &b) { return LengthSq(a.position) < LengthSq(b.position); });
			for( size_t i = 0; i < placementSet.size(); i++) rankedLocators[placementSet[i].uniqueID].w = float(i);
		}

		float biasAmmount = Length(distributionData.placementBias) + abs(distributionData.centerBias);
		float randomFactor = max(1.f - biasAmmount, 0.f); // BiasAmmount -> [0-2]

		for( auto& locator : placementSet )
		{
			uint32_t uID = locator.uniqueID;
			float powerRank = rankedLocators[uID].x * distributionData.placementBias.x;
			powerRank += rankedLocators[uID].y * distributionData.placementBias.y + rankedLocators[uID].z * distributionData.placementBias.z;
			powerRank *= -1;
			powerRank += float( placementSet.size() ) * TriRand() * randomFactor * 4.f;
			float cb = distributionData.centerBias;
			cb = cb < 0 ?  ( float(placementSet.size()) - rankedLocators[uID].w ) * abs(cb) : rankedLocators[uID].w * cb;
			powerRank += cb;

			// reuse the map vector for sorting as we no longer need it
			rankedLocators[uID].x = powerRank;
		}

		struct {
			bool operator()(EveSOFDataMgr::LocatorDirectionData &a,EveSOFDataMgr::LocatorDirectionData &b, std::map<int, Vector4> &priorityMap) const
			{
				return priorityMap[a.uniqueID].x < priorityMap[b.uniqueID].x;
			}
		} prioritySort;

		using namespace std::placeholders;
		std::sort(placementSet.begin(), placementSet.end(), std::bind(prioritySort, _1, _2, rankedLocators ));
	}

	while( count > 0 )
	{
		placementSet.pop_back();
		count--;
	}


	if( LengthSq(distributionData.randomRotationMaxSteps) > 0.f )
	{
		// handle rotation randomness
		float yaw, pitch, roll, stepYaw, stepPitch, stepRoll;
		TriQuaternionToYawPitchRoll( &stepYaw, &stepPitch, &stepRoll, &distributionData.randomRotationStepSizeYPR );

		for( auto& locator : placementSet )
		{
			yaw = stepYaw * floor( distributionData.randomRotationMaxSteps[0] * TriRand() + 0.5f );
			pitch = stepPitch * floor( distributionData.randomRotationMaxSteps[1] * TriRand() + 0.5f );
			roll = stepRoll * floor( distributionData.randomRotationMaxSteps[2] * TriRand() + 0.5f );
			
			Quaternion randomRotation = RotationQuaternion( yaw, pitch, roll );

			locator.rotation *= randomRotation;
		}
	}

	if( !distributionData.occupyLocators )
	{
		return;
	}

	// remove remaining locators that are about to be utilized from the spaceObj locator map
	for( auto locator : placementSet )
	{
		for( int32_t index = 0; index < managedLocatorSet.size(); index++ )
		{
			if( managedLocatorSet[index].uniqueID == locator.uniqueID )
			{
				std::swap(managedLocatorSet[index], managedLocatorSet.back());
				managedLocatorSet.pop_back();
				break;
			}
		}
	}
}

EveChildContainerPtr EveSOF::CreatePlacement( EveSpaceObject2Ptr parent, EveSOFDNAPtr extensionDna, EveSOFDataMgr::ExtensionPlacementData& placement, const std::vector<EveSOFDataMgr::LocatorDirectionData>& locators, const std::vector<Matrix>& nestedOffsets )
{
	Matrix placementOffset = TranslationMatrix( placement.offset );
	// This is the container for all placements
	EveChildContainerPtr container;
	container.CreateInstance();
	container->SetName( placement.name.c_str() );
	container->SetOrigin( IEveSpaceObjectChild::SOF );
	container->SetIsPlacementRoot( true );
	// The top container needs to be always on so clipping/activation strength will work properly
	// This will however not work properly for effects... sigh...
	container->SetAlwaysOn( true ); // #vomit

	if( !extensionDna->IsValid() )
	{
		CCP_LOGERR( "Creating layout placement failed because extentionDNA is invalid (probably missing assets): %s", extensionDna->GetDnaString() );
		return container;
	}

	std::vector<Matrix> placementOffsets;
	placementOffsets.reserve( nestedOffsets.size() * locators.size() );
	
	CcpMath::Sphere updatedBoundingSphere(extensionDna->GetParentBoundingSphere());
	CcpMath::AxisAlignedEllipsoid updatedEllipsoid(extensionDna->GetParentHullShapeEllipsoid());

	// Stuff for instanced placements
	std::vector<EveSOFDataMgr::HullMeshInstance> instances;
	instances.reserve( nestedOffsets.size() * locators.size() );

	uint32_t buildFlags = placement.isInstanced ? EveSOFDataHullBuildFilter::INSTANCED_PLACEMENT : EveSOFDataHullBuildFilter::NON_INSTANCED_PLACEMENT;

	// Placement Containers - used for non instanced meshes, controllers, effects and audio
	std::vector<EveChildContainerPtr> placementContainers;
	
	BlueSharedString extensionName = BlueSharedString(extensionDna->GetDnaString());

	for( auto& offset : nestedOffsets )
	{
		for( auto loc = locators.begin(); loc != locators.end(); ++loc )
		{
			Vector3 randomScale = loc->scaling;
			if( placement.hasDistribution )
			{
				if (placement.distribution.uniformScaling)
				{
					randomScale = randomScale * Lerp( placement.distribution.randomScaleMin, placement.distribution.randomScaleMax, TriRand() );
				}
				else
				{
					randomScale[0] = randomScale[0] * Lerp( placement.distribution.randomScaleMin[0], placement.distribution.randomScaleMax[0], TriRand() );
					randomScale[1] = randomScale[1] * Lerp( placement.distribution.randomScaleMin[1], placement.distribution.randomScaleMax[1], TriRand() );
					randomScale[2] = randomScale[2] * Lerp( placement.distribution.randomScaleMin[2], placement.distribution.randomScaleMax[2], TriRand() );
				}
			}
			Matrix transform = placementOffset * TransformationMatrix( randomScale, Normalize( loc->rotation ), loc->position ) * offset;
			placementOffsets.push_back( transform );

			Matrix transposed( XMMatrixTranspose( transform ) );

			EveChildContainerPtr placementContainer;
			placementContainer.CreateInstance();
			placementContainer->SetName( extensionName.c_str() );
			placementContainers.push_back( placementContainer );

			if( placement.isInstanced )
			{
				// add to the instance "buffer"
				EveSOFDataMgr::HullMeshInstance i;
				i.transform0 = *reinterpret_cast<Vector4*>( &transposed.GetX() );
				i.transform1 = *reinterpret_cast<Vector4*>( &transposed.GetY() );
				i.transform2 = *reinterpret_cast<Vector4*>( &transposed.GetZ() );
				i.lastTransform0 = *reinterpret_cast<Vector4*>( &transposed.GetX() );
				i.lastTransform1 = *reinterpret_cast<Vector4*>( &transposed.GetY() );
				i.lastTransform2 = *reinterpret_cast<Vector4*>( &transposed.GetZ() );
				i.boneIndex = loc->boneIndex;
				instances.push_back( i );
			}
			else
			{
				// create the child normally
				Quaternion rotation;
				Vector3 translation, scale;
				Decompose( scale, rotation, translation, transform );

				// create the non instanced extension mesh
				EveChildMeshPtr child;
				child.CreateInstance();
				auto mesh = CreateMesh( extensionDna );
				child->SetMesh( mesh );
				child->SetReflectionMode( extensionDna->GetReflectionMode() );
				child->SetCastShadow( extensionDna->CastShadow() );

				if( extensionDna->IsHullAnimated() )
				{ 
					Tr2GrannyAnimationPtr animationPtr;
					animationPtr.CreateInstance();
					child->SetAnimationController( animationPtr );

					// This will set the child as the animation owner of the parent, don't think this will be a problem...
					placementContainer->SetAnimationOwner( child );
				}
				child->SetName( "Hull" );
				child->Setup( &randomScale, &rotation, &translation, Tr2Lod::TR2_LOD_LOW );
				SetupDecalSets( BlueCastPtr( child->GetRawRoot() ), extensionDna );
				auto centerOffset = std::vector<Matrix>( 1, IdentityMatrix() );

				SetupAttachments( BlueCastPtr( child->GetRawRoot() ), extensionDna, centerOffset, buildFlags );
				
				placementContainer->AddToEffectChildrenList( child );
			}


			CcpMath::Sphere instanceSphere( extensionDna->GetHullBoundingSphere() );
			instanceSphere.Transform( transform );
			
			if( placement.extendsBoundingSphere )
			{
				// update the bounding sphere of the parent
				updatedBoundingSphere.IncludeSphere( instanceSphere );
			}

			// update the shield ellipsoid of the parent
			if( placement.extendsShieldEllipsoid )
			{
				CcpMath::AxisAlignedBox instanceBox( instanceSphere );
				if( extensionDna->GetHullShapeEllipsoid() )
				{
					instanceBox = CcpMath::AxisAlignedBox( extensionDna->GetHullShapeEllipsoid() );
					instanceBox.Transform( transform );
				}

				// include the instance box in the ellipsoid
				updatedEllipsoid.IncludeBox( instanceBox );
			}
			// Controllers!
			SetupControllers( BlueCastPtr( placementContainer->GetRawRoot() ), extensionDna, buildFlags );

			// And last but not least! AUDIO!
			SetupAudio( BlueCastPtr( placementContainer->GetRawRoot() ), extensionDna, transform );
		}
	}

	if( placement.isInstanced)
	{
		auto instancedMesh = CreateInstancedMesh( instances, extensionDna->GetHullGeometryResPath() );
		SetupShaders( extensionDna, instancedMesh );

		EveChildMeshPtr child;
		child.CreateInstance();
		child->SetMesh( instancedMesh );
		child->SetName( "Instanced Hull" );
		child->SetCastShadow( extensionDna->CastShadow() );

		// Set the reflectionMode based on the category
		child->SetReflectionMode( extensionDna->GetReflectionMode() );
		child->SetCastShadow( extensionDna->CastShadow() );
		child->SetMinScreenSize( MIN_INSTANCED_MESH_SCREEN_SIZE );

		SetupDecalSets( BlueCastPtr( child->GetRawRoot() ), extensionDna );
		// do this last so it sets all the needed shaders as instanced
		child->SetShaderOption( BlueSharedString( "SPACE_OBJECT_INSTANCED_ATTACHMENT" ), BlueSharedString( "SOIA_ENABLED" ) );

		child->SetInstanceTransforms( placementOffsets );
		
		SetupAttachments( BlueCastPtr( container->GetRawRoot() ), extensionDna, placementOffsets, buildFlags );

		container->AddToEffectChildrenList( child );
	}

	if( placement.extendsBoundingSphere )
	{
		// change the bounding sphere of the parent, so clipping spheres etc will still be correct
		parent->SetBoundingSphereInformation( updatedBoundingSphere );
		// also store the parentBounds in the dna if we have nested layouts
		extensionDna->SetParentBoundingSphere( updatedBoundingSphere );
	}
	if( placement.extendsShieldEllipsoid )
	{
		// update the space object and the dna
		parent->SetShapeEllipsoid( updatedEllipsoid );
		extensionDna->SetParentShapeEllipsoidInfo( updatedEllipsoid );
	}
	SetupInstancedMeshes( parent, extensionDna, placementOffsets );

	// Finish up setting up the common data
	// Need to move the effects into the correct placement containers, this is a bit ugly, but kinda works :D
	EveChildContainerPtr fakeContainer;
	fakeContainer.CreateInstance();
	SetupEffects( parent, (IEveEffectChildrenOwnerPtr)fakeContainer, extensionDna, placementOffsets, buildFlags );

	uint32_t index = 0;
	// We need to place the effects under the correct containers
	for( auto& effect : fakeContainer->m_objects )
	{
		placementContainers[index++ % placementContainers.size()]->AddToEffectChildrenList( effect );
	}

	for( auto& placementContainer : placementContainers )
	{
		// we don't really know until now if we have an empty placement container
		if( !placementContainer->Empty() )
		{
			container->AddToEffectChildrenList( placementContainer );
		}		
	}

	parent->AddToEffectChildrenList( container );

	//SetupCustomMask( newObj, dna );
	SetupLocatorSets( parent, extensionDna, placementOffsets );
	// setup nested layout
	SetupLayout( parent, extensionDna, placementOffsets );

	CCP_LOGNOTICE( "Creating %d extensions on %d places. Total: %d", placement.isInstanced ? " instanced" : "", locators.size(), nestedOffsets.size(), placementOffsets.size() );

	return container;
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


void EveSOF::RegenerateLayout( EveSpaceObject2* owner, const char* dnaString )
{
	// get parent ship's DNA
	EveSOFDNAPtr parentDna;
	parentDna.CreateInstance();
	parentDna->Setup( dnaString, &m_dataMgr );
	if( owner == nullptr)
	{
		CCP_LOGERR( "EveSOF: RegenerateLayout failed, owner is nullptr!" );
		return;
	
	}
	if( !parentDna->IsValid() )
	{
		CCP_LOGERR( "EveSOF: RegenerateLayout failed, wrong dna string %s!", dnaString );
		return;
	}

	// gather all the layout placement names
	std::vector<std::string> placmentNames;
	std::stack<EveSOFDNAPtr> dnaStrings;	
	dnaStrings.push( parentDna );

	while( !dnaStrings.empty() )
	{
		auto dna = dnaStrings.top();
		dnaStrings.pop();

		for( uint32_t layoutIndex = 0; layoutIndex < dna->GetLayoutCount(); ++layoutIndex )
		{
			auto layout = dna->GetLayoutData( layoutIndex );

			for( auto placement : layout->placements )
			{
				auto existingChild = owner->GetEffectChildByName( placement.name.c_str() );
				if( existingChild )
				{
					owner->RemoveFromEffectChildrenList( existingChild );
				}
				EveSOFDNAPtr placementDna;
				placementDna.CreateInstance();
				placementDna->Setup( layout->name, placement.descriptor, dna, &m_dataMgr );
				if( placementDna->IsValid() && placementDna->GetLayoutCount() > 0 )
				{
					dnaStrings.push( placementDna );	
				}
			}
		}
	}
	
	auto offsets = std::vector<Matrix>( 1, IdentityMatrix() );
	
	// because the layout adds locatorsets we need to recreate them
	owner->ClearLocatorSets();
	SetupLocatorSets( owner, parentDna, offsets );
	SetupLayout( owner, parentDna, offsets );
}
