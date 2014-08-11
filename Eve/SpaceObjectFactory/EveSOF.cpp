////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "Utilities/StringUtils.h"
#include "EveSOF.h"
#include "EveSOFDNA.h"
#include "Eve/EveTransform.h"
#include "Eve/SpaceObject/EveShip2.h"
#include "Eve/SpaceObject/Attachments/EveSpriteSet.h"
#include "Eve/SpaceObject/Attachments/EveTrailsSet.h"
#include "Eve/SpaceObject/Attachments/EveSpotlightSet.h"
#include "Eve/SpaceObject/Attachments/EvePlaneSet.h"
#include "Eve/SpaceObject/Attachments/EveBoosterSet2.h"
#include "Eve/SpaceObject/Attachments/EveSpaceObjectDecal.h"
#include "Eve/SpaceObject/Utils/EveLocator2.h"
#include "Tr2Mesh.h"
#include "Tr2MeshArea.h"
#include "Tr2Effect.h"
#include "EffectParameter/TriTexture2DParameter.h"
#include "EffectParameter/Tr2Vector4Parameter.h"
#include "EffectParameter/Tr2FloatParameter.h"
#include "include/ITriFunction.h"
#include "Curves/Tr2QuaternionCurve.h"
#include "Curves/Tr2ScalarCurve.h"
#include "Curves/TriCurveSet.h"
#include "Curves/TriRotationCurve.h"
#include "TriValueBinding.h"
#include "Particle/Tr2DynamicEmitter.h"


// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveSOF::EveSOF( IRoot* lockobj ) :
	PARENTLOCK( m_dataMgr )
{
	// some shared shaders here
	m_spriteSetEffect.CreateInstance();
	m_spriteSetEffect->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/fx/blinkinglights.fx" );
	m_spriteSetEffect->AddResourceTexture2D( "GradientMap", "res:/texture/particle/whitesharp_gradient.dds" );
	m_spriteSetEffectSkinned.CreateInstance();
	m_spriteSetEffectSkinned->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/fx/skinned_blinkinglights.fx" );
	m_spriteSetEffectSkinned->AddResourceTexture2D( "GradientMap", "res:/texture/particle/whitesharp_gradient.dds" );

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
//   This is the old way of loading a ship via redfile
// --------------------------------------------------------------------------------
IRootPtr EveSOF::Load( const char* resFile, const char* hullName, const char* factionName, const char* raceName )
{
	// load it like we used to: blue res manager
	IRootPtr p;
	p.Attach( BeResMan->LoadObject( resFile ) );
	if( p == nullptr )
	{
		return p;
	}

	// stop it here if no SOF info
	if( ( hullName[0] == 0 ) || ( factionName[0] == 0 ) || ( raceName[0] == 0 ) )
	{
		return p;
	}

	// create a temporary(!) DNA object
	EveSOFDNAPtr dna;
	dna.CreateInstance();

	// init it with given dna string
	std::string dnaString = std::string( hullName ) + "." + std::string( factionName ) + "." + std::string( raceName );
	dna->Setup( dnaString.c_str(), &m_dataMgr );

	// if this dna is not correct, we are done here
	if( !dna->isValid() )
	{
		return p;
	}

	// see if we can attach boosters, but only to ships
	EveShip2Ptr newShip;
	if( p->QueryInterface( BlueInterfaceIID<EveShip2>(), (void**)&newShip ) )
	{
		SetupBoosters( newShip, dna );
	}

	return p;
}

// --------------------------------------------------------------------------------
// Description:
//   Build a ship from a dna string
// --------------------------------------------------------------------------------
IRootPtr EveSOF::BuildFromDNA( const char* dnaString )
{
	// create a temporary(!) DNA object
	EveSOFDNAPtr dna;
	dna.CreateInstance();

	// init it with given dna string
	dna->Setup( dnaString, &m_dataMgr );
	if( !dna->isValid() )
	{
		return nullptr;
	}

	// make an EveShip2 for now...
	EveShip2Ptr newShip;
	newShip.CreateInstance();

	// get us the base geometry
	SetupMesh( newShip, dna );

	// decals
	SetupDecals( newShip, dna );

	// effects on ships
	SetupSpriteSets( newShip, dna );
	SetupSpotlightSets( newShip, dna );
	SetupPlaneSets( newShip, dna );

	// attachments to ship
	SetupBoosters( newShip, dna );
	SetupLocators( newShip, dna );

	// children, animations and particles
	SetupChildrenAndAnimations( newShip, dna );

	// model curves
	SetupModelCurves( newShip, dna );

	// ships needs a final ::Initialize call
	newShip->Initialize();

	return newShip->GetRawRoot();
}

// --------------------------------------------------------------------------------
// Description:
//   This is where it is all going to happen: building a ship with the three
//   main infos
// --------------------------------------------------------------------------------
IRootPtr EveSOF::Build( const char* hullName, const char* factionName, const char* raceName )
{
	// make a SOF ship dna string
	std::string dnaString = std::string( hullName ) + "." + std::string( factionName ) + "." + std::string( raceName );

	// pass on to real build function
	return BuildFromDNA( dnaString.c_str() );
}

// --------------------------------------------------------------------------------
// Description:
//   This is where it is all going to happen
// --------------------------------------------------------------------------------
void EveSOF::SetupMesh( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const
{
	// need a mesh
	Tr2MeshPtr mesh;
	mesh.CreateInstance();

	// gr2 res path
	mesh->SetMeshResPath( dna->GetHullGeometryResPath() );

	// beoundingsphere comes from data, is faster
	ship->SetBoundingSphereInformation( dna->GetHullBoundingSphere() );

	// shadow
	if( dna->IsHullAnimated() )
	{
		ship->SetShadowEffect( m_shadowEffectSkinned );
	}
	else
	{
		ship->SetShadowEffect( m_shadowEffect );
	}

	// setup mesh areas
	FillMeshAreaVector( mesh->GetAreas( TRIBATCHTYPE_OPAQUE ), TRIBATCHTYPE_OPAQUE, dna );
	FillMeshAreaVector( mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT ), TRIBATCHTYPE_TRANSPARENT, dna );
	FillMeshAreaVector( mesh->GetAreas( TRIBATCHTYPE_ADDITIVE ), TRIBATCHTYPE_ADDITIVE, dna );
	FillMeshAreaVector( mesh->GetAreas( TRIBATCHTYPE_DEPTH ), TRIBATCHTYPE_DEPTH, dna );
	FillMeshAreaVector( mesh->GetAreas( TRIBATCHTYPE_DISTORTION ), TRIBATCHTYPE_DISTORTION, dna );

	// create lod levels
	ship->SetHighDetailMesh( mesh );
	ship->SetMediumDetailMesh( CreateMeshLOD( mesh, "_mediumDetail" ) );
	ship->SetLowDetailMesh( CreateMeshLOD( mesh, "_lowDetail" ) );
}

// --------------------------------------------------------------------------------
// Description:
//   Fill up mesh area vector given the hull and faction area data provided.
// --------------------------------------------------------------------------------
void EveSOF::FillMeshAreaVector( Tr2MeshAreaVector* meshAreaVector, TriBatchType areaType, const EveSOFDNAPtr dna ) const
{
	const std::vector<EveSOFDataMgr::HullAreas>* hullAreas = dna->GetHullMeshAreas( areaType );
	for( auto area = hullAreas->begin(); area != hullAreas->end(); ++area )
	{
		// every area has it's own shader, nothing we can share here
		Tr2EffectPtr newShader;
		newShader.CreateInstance();
		newShader->StartUpdate();
		if( dna->IsHullAnimated() )
		{
			std::string skinnedName = area->shaderPath;
			std::string insertString = "skinned_";
			size_t pos = skinnedName.find_last_of( '/' );
			if ( pos != std::string::npos )
			{
				skinnedName.insert( pos + 1, insertString );
			}
			newShader->SetEffectPathName( skinnedName.c_str() );
		}
		else
		{
			newShader->SetEffectPathName( area->shaderPath.c_str() );
		}

		// parameters
		for( auto hullAreaParamsIt = area->parameters.begin(); hullAreaParamsIt != area->parameters.end(); ++hullAreaParamsIt )
		{
			const Vector4* factionParam = dna->GetFactionMeshAreaParameters( areaType, area->designation.c_str(), hullAreaParamsIt->first.c_str() );
			if( factionParam )
			{
				newShader->AddParameterVector4( hullAreaParamsIt->first.c_str(), factionParam );
			}
			else
			{
				newShader->AddParameterVector4( hullAreaParamsIt->first.c_str(), &hullAreaParamsIt->second );
			}
		}

		// shader textures
		for( auto it = area->textures.begin(); it != area->textures.end(); ++it )
		{
			// res path might be factional!!
			std::string resPath = it->second.resFilePath.c_str();
			dna->ModifyTextureResPath( resPath, it->first.c_str() );
			newShader->AddResourceTexture2D( it->first.c_str(), resPath.c_str() );
		}

		// that's it for setting up this shader, must rebuild cache on it!
		newShader->EndUpdate();

		// new mesharea
		Tr2MeshAreaPtr newMeshArea;
		newMeshArea.CreateInstance();
		newMeshArea->SetName( area->designation );
		newMeshArea->SetMaterial( newShader );
		newMeshArea->SetIndex( area->index );
		newMeshArea->SetCount( area->count );
		meshAreaVector->Append( newMeshArea );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Helper function for replacting resource path strings
// --------------------------------------------------------------------------------
void EveSOF::ModifyResourcePathsForLOD( const Tr2MeshAreaVector* areas, const char* lodInsert ) const
{
	const char* name = nullptr;
	for( auto it = areas->begin(); it != areas->end(); ++it )
	{
		Tr2EffectPtr effect; 
		if( !(*it)->GetMaterialInterface()->QueryInterface( BlueInterfaceIID<Tr2Effect>(), (void**)&effect ) )
		{
			continue;
		}
		for( auto resIt = effect->m_resources.begin(); resIt != effect->m_resources.end(); ++resIt )
		{
			TriTexture2DParameterPtr textureRes;
			name = (*resIt)->GetParameterName();
			if( !strcmp( name, "PgsMap" ) || !strcmp( name, "NormalMap" ) || !strcmp( name, "DiffuseMap" ) || !strcmp( name, "AoMap" ) )
			{
				if( (*resIt)->QueryInterface( BlueInterfaceIID<TriTexture2DParameter>(), (void**)&textureRes ) )
				{
					std::string resPathCopy = textureRes->GetResourcePath();
					if( StringInsertStub( resPathCopy, ".dds", lodInsert ) )
					{
						textureRes->SetResourcePath( resPathCopy.c_str() );
					}
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Create an actual copy of the base mesh to use as LOD.
// --------------------------------------------------------------------------------
Tr2MeshPtr EveSOF::CreateMeshLOD( const Tr2Mesh* base, const char* lodInsert ) const
{
	Tr2MeshPtr mesh;
	BeClasses->CopyTo( base->GetRawRoot(), (IRoot**)&mesh );

	ModifyResourcePathsForLOD( mesh->GetAreas( TRIBATCHTYPE_OPAQUE ), lodInsert );
	ModifyResourcePathsForLOD( mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT ), lodInsert );
	
	std::string path = mesh->GetMeshResPath();
	if( StringInsertStub( path, ".gr2", lodInsert ) )
	{
		mesh->SetMeshResPath( path.c_str() );
	}
	
	return mesh;
}

// --------------------------------------------------------------------------------
// Description:
//   This is where it is all going to happen
// --------------------------------------------------------------------------------
void EveSOF::SetupSpriteSets( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const
{
	// cycle over all spritesets of this hull
	const std::vector<EveSOFDataMgr::HullSpriteSetData>& hullSpriteSets = dna->GetHullSpriteSets();
	for( auto ssit = hullSpriteSets.begin(); ssit != hullSpriteSets.end(); ++ssit )
	{
		const EveSOFDataMgr::HullSpriteSetData* spriteSetData = &(*ssit);

		// create a spriteset for this ship
		EveSpriteSetPtr spriteSet;
		spriteSet.CreateInstance();
		// set skinned or unskinned shader
		spriteSet->SetEffect( spriteSetData->skinned ? m_spriteSetEffectSkinned : m_spriteSetEffect );
		// add all the individual items
		for( auto ssiit = spriteSetData->m_items.begin(); ssiit != spriteSetData->m_items.end(); ++ssiit )
		{
			const EveSOFDataMgr::HullSpriteSetItemData* itemData = &(*ssiit);

			// faction data?
			const EveSOFDataMgr::FactionSpriteSetColorData* factionSpriteData = dna->GetFactionSpriteSetData( ssiit->groupIndex );
			if( !factionSpriteData )
			{
				// This spriteset item is not used for this faction.
				continue;
			}

			// create spriteset items
			EveSpriteSetItemPtr spriteSetItem;
			spriteSetItem.CreateInstance();

			if( factionSpriteData )
			{
				spriteSetItem->m_color = factionSpriteData->color;
			}
			else
			{
				// sprite set doesn't exist for faction
				spriteSetItem->m_color = Color( 1.f, 0.f, 0.f, 1.f );
			}

			// set it up, first with the per-hull data
			spriteSetItem->m_blinkPhase = ssiit->blinkPhase;
			spriteSetItem->m_blinkRate = ssiit->blinkRate;
			spriteSetItem->m_boneIndex = ssiit->boneIndex;
			spriteSetItem->m_falloff = ssiit->falloff;
			spriteSetItem->m_maxScale = ssiit->maxScale;
			spriteSetItem->m_minScale = ssiit->minScale;
			spriteSetItem->m_position = ssiit->position;

			// put it into spriteset
			spriteSet->Add( spriteSetItem );
		}
		// spriteset needs internal rebuild
		spriteSet->Rebuild();
		// put set onto ship
		ship->AddSpriteSet( spriteSet );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   This is where it is all going to happen
// --------------------------------------------------------------------------------
void EveSOF::SetupSpotlightSets( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const
{
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

		// set skinned or unskinned shader
		if( spotlightSetData->skinned )
		{
			coneEffect->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/fx/skinned_spotlightcone.fx" );
			glowEffect->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/fx/skinned_spotlightglow.fx" );
		}
		else
		{
			coneEffect->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/fx/spotlightcone.fx" );
			glowEffect->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/fx/spotlightglow.fx" );
		}

		// textures
		glowEffect->AddResourceTexture2D( "TextureMap", spotlightSetData->glowTextureResPath.c_str() );
		coneEffect->AddResourceTexture2D( "TextureMap", spotlightSetData->coneTextureResPath.c_str() );

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

		// add all individual items
		for( auto ssiit = spotlightSetData->items.begin(); ssiit != spotlightSetData->items.end(); ++ssiit )
		{
			// crete it
			EveSpotlightSetItemPtr spotlightSetItem;
			spotlightSetItem.CreateInstance();
			// fill it up
			spotlightSetItem->m_boneIndex = ssiit->boneIndex;
			spotlightSetItem->m_boosterGainInfluence = ssiit->boosterGainInfluence;
			
			// faction data?
			const EveSOFDataMgr::FactionSpotlightSetColorData* factionSpotlightData = dna->GetFactionSpotlightSetData( ssiit->groupIndex );
			if( factionSpotlightData )
			{
				spotlightSetItem->m_coneColor = ssiit->coneIntensity * factionSpotlightData->coneColor;
				spotlightSetItem->m_flareColor = ssiit->flareIntensity * factionSpotlightData->flareColor;
				spotlightSetItem->m_spriteColor = ssiit->spriteIntensity * factionSpotlightData->spriteColor;
			}
			else
			{
				spotlightSetItem->m_coneColor = Color( 1.f, 1.f, 1.f, 1.f );
				spotlightSetItem->m_flareColor = Color( 1.f, 1.f, 1.f, 1.f );
				spotlightSetItem->m_spriteColor = Color( 1.f, 1.f, 1.f, 1.f );
			}

			spotlightSetItem->m_spriteScale = ssiit->spriteScale;
			spotlightSetItem->m_transform = ssiit->transform;
			// add it
			spotlightSet->AddSpotlightItem( spotlightSetItem );
		}
		// spotlightset needs internal rebuild
		spotlightSet->Rebuild();
		// add to ship
		ship->AddSpotlightSet( spotlightSet );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   This is where it is all going to happen
// --------------------------------------------------------------------------------
void EveSOF::SetupPlaneSets( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const
{
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

		// set skinned or unskinned shader
		if( planeSetData->skinned )
		{
			planeEffect->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/fx/skinned_planeglow.fx" );
		}
		else
		{
			planeEffect->SetEffectPathName( "res:/graphics/effect/managed/space/spaceobject/fx/planeglow.fx" );
		}

		// textures
		planeEffect->AddResourceTexture2D( "Layer1Map", planeSetData->layer1MapResPath.c_str() );
		planeEffect->AddResourceTexture2D( "Layer2Map", planeSetData->layer2MapResPath.c_str() );
		planeEffect->AddResourceTexture2D( "MaskMap", planeSetData->maskMapResPath.c_str() );

		// parameters
		planeEffect->AddParameterVector4( "PlaneData", &planeSetData->planeData );

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
		ship->AddPlaneSet( planeSet );
	}

}

// --------------------------------------------------------------------------------
// Description:
//   Load Model Curves for rotation and translation, if there are any
// --------------------------------------------------------------------------------
void EveSOF::SetupModelCurves( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const
{
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
				ship->SetModelRotationCurve( rotationCurve );
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
				ship->SetModelTranslationCurve( translationCurve );
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

// --------------------------------------------------------------------------------
// Description:
//   Add Children and Animations to the ship
//   
// --------------------------------------------------------------------------------
void EveSOF::SetupChildrenAndAnimations( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const
{
	std::map<int, std::vector<EveTransformPtr>> childrenToBindTo;

	const std::vector<EveSOFDataMgr::HullChild>& hullChildren = dna->GetHullChildren();
	for( auto childIt = hullChildren.begin(); childIt != hullChildren.end(); ++childIt )
	{
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
		if( !p->QueryInterface( BlueInterfaceIID<EveTransform>(), (void**)&child ) )
		{
			CCP_LOGERR( "resource file %s is not of correct type!", childIt->redFilePath.c_str() );
			return;
		}
		child->SetRotation( childIt->rotation );
		child->SetScaling( childIt->scaling );
		child->SetTranslation( childIt->translation );
		if( childIt->id != -1 )
		{
			childrenToBindTo[childIt->id].push_back( child );
		}

		ship->AddToChildrenList( child );
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
			if( !ship->GetModelRotationCurve() )
			{
				TriRotationCurvePtr modelRotationcurve;
				modelRotationcurve.CreateInstance();
				ship->SetModelRotationCurve( (ITriQuaternionFunctionPtr)modelRotationcurve );
			}
			binding->SetDestination( "value", ship->GetModelRotationCurve()->GetRootObject() );
			binding->Initialize();

			curveSet->AddBinding( (ITr2ValueBindingPtr)binding );

			// Append the curveSet
			ship->AddCurveSet( curveSet );

		}
		// Do we have valid translation info?
		if( animIt->startTranslationTime != -1.0 )
		{
			// TODO
		}
	}
}


// --------------------------------------------------------------------------------
// Description:
//   add the booster to the new ship
// --------------------------------------------------------------------------------
void EveSOF::SetupBoosters( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const
{
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
	set->SetData( rdata->glowScale, &rdata->glowColor, rdata->symHaloScale, rdata->haloScaleX, rdata->haloScaleY, &rdata->haloColor, hdata->alwaysOn );

	// create and setup booster effect
	Tr2EffectPtr effect;
	effect.CreateInstance();
	effect->StartUpdate();
	effect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/Booster/Booster.fx" );
	effect->AddParameterColor( "Color", &rdata->color );
	effect->AddParameterVector4( "BoosterScale", &rdata->scale );
	effect->AddResourceTexture2D( "WaveMap", "res:/Texture/Sprite/waveHiFi.dds" );
	effect->AddResourceTexture2D( "DiffuseMap", rdata->textureResPath.c_str() );
	// finish effect and set it
	effect->EndUpdate();
	set->SetEffect( effect );

	// create and setup glows
	EveSpriteSetPtr glow;
	glow.CreateInstance();
	Tr2EffectPtr glowEffect;
	glowEffect.CreateInstance();
	glowEffect->StartUpdate();
	glowEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/Booster/BoosterGlow.fx" );
	glowEffect->AddResourceTexture2D( "DiffuseMap", "res:/Texture/Particle/whitesharp.dds" );
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
		trailEffect->AddParameterVector4( "TrailSize", &rdata->trailSize );
		trailEffect->AddParameterColor( "TrailColor", &rdata->trailColor );
		trailEffect->EndUpdate();
		trail->SetEffect( trailEffect );
		trail->SetMeshResPath( "res:/dx9/model/ship/booster/volumetricTrail.gr2" );
		set->SetTrail( trail );
	}

	// add all the indiviual items
	for( auto biit = hdata->items.begin(); biit != hdata->items.end(); ++biit )
	{
		set->Add( &biit->transform, &biit->functionality, biit->hasTrail );
	}

	// add it to ship
	set->PrepareResources();
	ship->SetBoosterSet( set );
}

// --------------------------------------------------------------------------------
// Description:
//   add the hull decals to the new ship
// --------------------------------------------------------------------------------
void EveSOF::SetupDecals( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const
{
	// create and setup all hull decals
	const std::vector<EveSOFDataMgr::HullDecalData>& hullDecals = dna->GetHullDecals();
	for( auto hdit = hullDecals.begin(); hdit != hullDecals.end(); ++hdit )
	{
		// do we have faction data for this decal?
		const EveSOFDataMgr::FactionDecalData* fdd = dna->GetFactionDecalData( hdit->groupIndex );

		// decal can be invisibe for this faction
		if( fdd && !fdd->isVisible )
		{
			continue;
		}

		// create
		EveSpaceObjectDecalPtr decal;
		decal.CreateInstance();
		// set general datas
		decal->SetPosition( hdit->position );
		decal->SetRotation( hdit->rotation );
		decal->SetScaling( hdit->scaling );
		decal->SetBoneIndex( hdit->boneIndex );
		// the decal effect
		Tr2EffectPtr shader;
		shader.CreateInstance();
		shader->StartUpdate();

		if( fdd && !fdd->shaderPath.empty() )
		{
			shader->SetEffectPathName( fdd->shaderPath.c_str() );
		}
		else
		{
			shader->SetEffectPathName( hdit->shaderPath.c_str() );
		}

		// always set hull parameters & textures for this decal
		for( auto hdpit = hdit->parameters.begin(); hdpit != hdit->parameters.end(); ++hdpit )
		{
			shader->AddParameterVector4( hdpit->first.c_str(), &hdpit->second );
		}
		for( auto hdtit = hdit->textures.begin(); hdtit != hdit->textures.end(); ++hdtit )
		{
			shader->AddResourceTexture2D( hdtit->first.c_str(), hdtit->second.resFilePath.c_str() );
		}

		// then set the factional
		if( fdd )
		{
			for( auto fdpit = fdd->parameters.begin(); fdpit != fdd->parameters.end(); ++fdpit )
			{
				shader->AddParameterVector4( fdpit->first.c_str(), &fdpit->second );
			}
			for( auto fdtit = fdd->textures.begin(); fdtit != fdd->textures.end(); ++fdtit )
			{
				shader->AddResourceTexture2D( fdtit->first.c_str(), fdtit->second.resFilePath.c_str() );
			}
		}

		// init and add
		shader->EndUpdate();
		decal->SetEffect( shader );
		decal->Initialize();
		ship->AddDecal( decal );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   add the hull locators to the new ship
// --------------------------------------------------------------------------------
void EveSOF::SetupLocators( EveShip2Ptr ship, const EveSOFDNAPtr dna ) const
{
	// create and setup all turret locators
	const std::vector<EveSOFDataMgr::LocatorData>& turretLocators = dna->GetHullTurretLocators();
	for( auto tlit = turretLocators.begin(); tlit != turretLocators.end(); ++tlit )
	{
		// create a new locator
		EveLocator2Ptr loc;
		loc.CreateInstance();

		// set it up
		loc->SetName( tlit->name.c_str() );
		loc->SetTransform( tlit->transform );

		// add it to the new ship
		ship->AddLocator( loc );
	}
}








