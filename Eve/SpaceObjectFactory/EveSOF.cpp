////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "EveSOF.h"
#include "Eve/EveTransform.h"
#include "Eve/SpaceObject/EveShip2.h"
#include "Eve/SpaceObject/Attachments/EveSpriteSet.h"
#include "Eve/SpaceObject/Attachments/EveTrailsSet.h"
#include "Eve/SpaceObject/Attachments/EveSpotlightSet.h"
#include "Eve/SpaceObject/Attachments/EvePlaneSet.h"
#include "Eve/SpaceObject/Attachments/EveBoosterSet2.h"
#include "Eve/SpaceObject/Attachments/EveSpaceObjectDecal.h"
#include "Tr2Mesh.h"
#include "Tr2Effect.h"
#include "EffectParameter/TriTexture2DParameter.h"
#include "EffectParameter/Tr2Vector4Parameter.h"
#include "EffectParameter/Tr2FloatParameter.h"

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
IRootPtr EveSOF::Load( const char* resFile, const char* hullName, const char* raceName )
{
	// load it like we used to: blue res manager
	IRootPtr p;
	p.Attach( BeResMan->LoadObject( resFile ) );
	if( p == nullptr )
	{
		return p;
	}

	// try to get hullinfo
	const EveSOFDataMgr::HullData* hullData = m_dataMgr.GetHullData( hullName );
	if( hullData == NULL )
	{
		return p;
	}

	// try to get raceinfo
	const EveSOFDataMgr::RaceData* raceData = m_dataMgr.GetRaceData( raceName );
	if( raceData == NULL )
	{
		return p;
	}

	// see if we can attach boosters, but only to ships
	EveShip2Ptr newShip;
	if( p->QueryInterface( BlueInterfaceIID<EveShip2>(), (void**)&newShip ) )
	{
		SetupBoosters( newShip, hullData, raceData );
	}

	return p;
}

// --------------------------------------------------------------------------------
// Description:
//   This is where it is all going to happen
// --------------------------------------------------------------------------------
IRootPtr EveSOF::Build( const char* hullName, const char* factionName, const char* raceName )
{
	// make sure we find this hull
	const EveSOFDataMgr::HullData* hullData = m_dataMgr.GetHullData( hullName );
	if( hullData == NULL )
	{
		CCP_LOGERR( "Couldn't find the requested hull %s in the database", hullName );
		return NULL;
	}
	// make sure we find this faction
	const EveSOFDataMgr::FactionData* factionData = m_dataMgr.GetFactionData( factionName );
	if( factionData == NULL )
	{
		CCP_LOGERR( "Couldn't find the requested faction %s in the database", factionName );
		return NULL;
	}
	// make sure we find this race
	const EveSOFDataMgr::RaceData* raceData = m_dataMgr.GetRaceData( raceName );
	if( raceData == NULL )
	{
		CCP_LOGERR( "Couldn't find the requested race %s in the database", raceName );
		return NULL;
	}

	// make an EveShip2 for now...
	EveShip2Ptr newShip;
	newShip.CreateInstance();

	// get us the base geometry
	SetupGeometry( newShip, hullData, factionData );

	// materials (aka skins)
	SetupMeshArea( newShip, hullData, factionData );

	// decals
	SetupHullDecals( newShip, hullData );

	// effects on ships
	SetupSpriteSets( newShip, hullData, factionData );
	SetupSpotlightSets( newShip, hullData, factionData );
	SetupPlaneSets( newShip, hullData, factionData );

	// attachments to ship
	SetupBoosters( newShip, hullData, raceData );

	// children
	SetupChildren( newShip, hullData, raceData );

	// ships needs a final ::Initialize call
	newShip->Initialize();

	return newShip->GetRawRoot();
}

// --------------------------------------------------------------------------------
// Description:
//   This is where it is all going to happen
// --------------------------------------------------------------------------------
void EveSOF::SetupGeometry( EveShip2Ptr ship, const EveSOFDataMgr::HullData* hullData, const EveSOFDataMgr::FactionData* factionData ) const
{
	// need a mesh
	Tr2MeshPtr newMesh;
	newMesh.CreateInstance();

	// gr2 res path
	newMesh->SetMeshResPath( hullData->geometryResFilePath.c_str() );

	// plug it into ship
	ship->SetMesh( newMesh );

	// beoundingsphere comes from data, is faster
	ship->SetBoundingSphereInformation( &hullData->boundingSphere );

	// shadow
	if( hullData->isSkinned )
	{
		ship->SetShadowEffect( m_shadowEffectSkinned );
	}
	else
	{
		ship->SetShadowEffect( m_shadowEffect );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Fill up mesh area vector given the hull and faction area data provided.
// --------------------------------------------------------------------------------
void EveSOF::FillMeshAreaVector( const std::vector<EveSOFDataMgr::HullAreas>* hullAreas, const FactionAreaMap* factionAreas, const EveSOFDataMgr::FactionData* factionData, Tr2MeshAreaVector* meshAreaVector ) const
{
	for( auto area = hullAreas->begin(); area != hullAreas->end(); ++area )
	{
		// every area has it's own shader, nothing we can share here
		Tr2EffectPtr newShader;
		newShader.CreateInstance();
		newShader->SetEffectPathName( area->shaderPath.c_str() );

		const std::map<std::string, Vector4>* factionShaderParams = nullptr;
		if( factionAreas )
		{
			// shader parameters all come from faction
			auto shaderParameters = factionAreas->find( area->designation );
			if( shaderParameters != factionAreas->end() )
			{
				factionShaderParams = &(shaderParameters->second.parameters);
			}
		}
		// parameters
		for( auto hullAreaParamsIt = area->parameters.begin(); hullAreaParamsIt != area->parameters.end(); ++hullAreaParamsIt )
		{
			if( !factionShaderParams )
			{
				newShader->AddParameterVector4( hullAreaParamsIt->first.c_str(), &hullAreaParamsIt->second );
			}
			else
			{
				auto factionParam = factionShaderParams->find( hullAreaParamsIt->first );
				if( factionParam != factionShaderParams->end() )
				{
					newShader->AddParameterVector4( hullAreaParamsIt->first.c_str(), &factionParam->second );
				}
				else
				{
					newShader->AddParameterVector4( hullAreaParamsIt->first.c_str(), &hullAreaParamsIt->second );
				}
			}
		}

		// shader textures
		for( auto it = area->textures.begin(); it != area->textures.end(); ++it )
		{
			// res path might be factional!!
			std::string resPath = it->second.resFilePath.c_str();
			ModifyTextureResPath( resPath, it->first.c_str(), factionData );
			newShader->AddResourceTexture2D( it->first.c_str(), resPath.c_str() );
		}

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
//   This is where it is all going to happen
// --------------------------------------------------------------------------------
void EveSOF::SetupMeshArea( EveShip2Ptr ship, const EveSOFDataMgr::HullData* hullData, const EveSOFDataMgr::FactionData* factionData ) const
{
	// start populating all areas with mesharea objects
	Tr2MeshPtr mesh = ship->GetMesh();

	Tr2MeshAreaVector* opaqueMeshAreaVector = mesh->GetAreas( TRIBATCHTYPE_OPAQUE );
	FillMeshAreaVector( &hullData->opaqueAreas, &factionData->opaqueAreaParameters, factionData, opaqueMeshAreaVector );
	
	Tr2MeshAreaVector* transparentMeshAreaVector = mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT );
	FillMeshAreaVector( &hullData->transparentAreas, &factionData->transparentAreaParameters, factionData, transparentMeshAreaVector );
	
	Tr2MeshAreaVector* additiveMeshAreaVector = mesh->GetAreas( TRIBATCHTYPE_ADDITIVE );
	FillMeshAreaVector( &hullData->additiveAreas, nullptr, factionData, additiveMeshAreaVector );
	
	Tr2MeshAreaVector* depthMeshAreaVector = mesh->GetAreas( TRIBATCHTYPE_DEPTH );
	FillMeshAreaVector( &hullData->depthAreas, nullptr, factionData, depthMeshAreaVector );
	
	Tr2MeshAreaVector* distortionMeshAreaVector = mesh->GetAreas( TRIBATCHTYPE_DISTORTION );
	FillMeshAreaVector( &hullData->distortionAreas, nullptr, factionData, distortionMeshAreaVector );
}

// --------------------------------------------------------------------------------
// Description:
//   This is where it is all going to happen
// --------------------------------------------------------------------------------
void EveSOF::SetupSpriteSets( EveShip2Ptr ship, const EveSOFDataMgr::HullData* hullData, const EveSOFDataMgr::FactionData* factionData ) const
{
	// cycle over all spritesets of this hull
	for( auto ssit = hullData->spriteSets.begin(); ssit != hullData->spriteSets.end(); ++ssit )
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

			// if we find no faction data for this sprite, it means it is not to be there for this ship/faction
			auto finder = factionData->spriteSetsColor.find( ssiit->groupIndex );
			if( finder == factionData->spriteSetsColor.end() )
			{
				continue;
			}

			// create spriteset items
			EveSpriteSetItemPtr spriteSetItem;
			spriteSetItem.CreateInstance();
			// set it up, first with the per-hull data
			spriteSetItem->m_blinkPhase = ssiit->blinkPhase;
			spriteSetItem->m_blinkRate = ssiit->blinkRate;
			spriteSetItem->m_boneIndex = ssiit->boneIndex;
			spriteSetItem->m_falloff = ssiit->falloff;
			spriteSetItem->m_maxScale = ssiit->maxScale;
			spriteSetItem->m_minScale = ssiit->minScale;
			spriteSetItem->m_position = ssiit->position;
			// now with the per-faction data
			spriteSetItem->m_color = finder->second.color;

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
void EveSOF::SetupSpotlightSets( EveShip2Ptr ship, const EveSOFDataMgr::HullData* hullData, const EveSOFDataMgr::FactionData* factionData ) const
{
	// cycle over all spritesets of this hull
	for( auto ssit = hullData->spotlightSets.begin(); ssit != hullData->spotlightSets.end(); ++ssit )
	{
		const EveSOFDataMgr::HullSpotlightSetData* spotlightSetData = &(*ssit);

		// create a spriteset for this ship
		EveSpotlightSetPtr spotlightSet;
		spotlightSet.CreateInstance();

		// create shaders
		Tr2EffectPtr coneEffect;
		coneEffect.CreateInstance();
		Tr2EffectPtr glowEffect;
		glowEffect.CreateInstance();

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
			
			auto finder = factionData->spotlightSetColors.find( ssiit->groupIndex );
			if( finder == factionData->spotlightSetColors.end() )
			{
				spotlightSetItem->m_coneColor = Color( 1.f, 1.f, 1.f, 1.f );
				spotlightSetItem->m_flareColor = Color( 1.f, 1.f, 1.f, 1.f );
				spotlightSetItem->m_spriteColor = Color( 1.f, 1.f, 1.f, 1.f );
			}
			else
			{
				const EveSOFDataMgr::FactionSpotlightSetColorData& colors = finder->second;
				spotlightSetItem->m_coneColor = colors.coneColor;
				spotlightSetItem->m_coneColor *= ssiit->coneIntensity;
				
				spotlightSetItem->m_flareColor = colors.flareColor;
				spotlightSetItem->m_flareColor *= ssiit->flareIntensity;
				
				spotlightSetItem->m_spriteColor = colors.spriteColor;
				spotlightSetItem->m_spriteColor *= ssiit->spriteIntensity;
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
void EveSOF::SetupPlaneSets( EveShip2Ptr ship, const EveSOFDataMgr::HullData* hullData, const EveSOFDataMgr::FactionData* factionData ) const
{
	// cycle over all spritesets of this hull
	for( auto psit = hullData->planeSets.begin(); psit != hullData->planeSets.end(); ++psit )
	{
		const EveSOFDataMgr::HullPlaneSetData* planeSetData = &(*psit);

		// create a planeset for this ship
		EvePlaneSetPtr planeSet;
		planeSet.CreateInstance();

		// create shader
		Tr2EffectPtr planeEffect;
		planeEffect.CreateInstance();

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

		// set to set
		planeSet->SetEffect( planeEffect );

		// add all individual items
		for( auto psiit = planeSetData->items.begin(); psiit != planeSetData->items.end(); ++psiit )
		{
			// crete it
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
//   Add Children to the ship
// --------------------------------------------------------------------------------
void EveSOF::SetupChildren( EveShip2Ptr ship, const EveSOFDataMgr::HullData* hullData, const EveSOFDataMgr::RaceData* raceData ) const
{
	for( auto chit = hullData->children.begin(); chit != hullData->children.end(); ++chit )
	{
		IRootPtr p;
		IRoot* tmp = BeResMan->LoadObject( chit->redFilePath.c_str() );
		if( !tmp )
		{
			CCP_LOGERR( "resource file %s is invalid!", chit->redFilePath.c_str() );
			continue;
		}
		p.Attach( tmp );

		// is it of right type?
		EveTransformPtr child;
		if( !p->QueryInterface( BlueInterfaceIID<EveTransform>(), (void**)&child ) )
		{
			CCP_LOGERR( "resource file %s is not of correct type!", chit->redFilePath.c_str() );
			return;
		}
		child->SetRotation( chit->rotation );
		child->SetScaling( chit->scaling );
		child->SetTranslation( chit->translation );

		ship->AddToChildrenList( child );

	}
	
}

// --------------------------------------------------------------------------------
// Description:
//   add the booster to the new ship
// --------------------------------------------------------------------------------
void EveSOF::SetupBoosters( EveShip2Ptr ship, const EveSOFDataMgr::HullData* hullData, const EveSOFDataMgr::RaceData* raceData ) const
{
	// does this hull have boosters at all?
	if( hullData->boosters.items.empty() )
	{
		return;
	}

	// create
	EveBoosterSet2Ptr set;
	set.CreateInstance();

	// per-race data
	const EveSOFDataMgr::BoosterData* rdata = (const EveSOFDataMgr::BoosterData*)&raceData->boosterData;
	// pre-hull data
	const EveSOFDataMgr::HullBoosterData* hdata = (const EveSOFDataMgr::HullBoosterData*)&hullData->boosters;

	// set the booster set's internal data
	set->SetData( rdata->glowScale, &rdata->glowColor, rdata->symHaloScale, rdata->haloScaleX, rdata->haloScaleY, &rdata->haloColor, hdata->alwaysOn );

	Tr2EffectPtr effect;
	effect.CreateInstance();
	effect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/Booster/Booster.fx" );
	effect->AddParameterColor( "Color", &rdata->color );
	effect->AddParameterVector4( "BoosterScale", &rdata->scale );
	effect->AddResourceTexture2D( "WaveMap", "res:/Texture/Sprite/waveHiFi.dds" );
	effect->AddResourceTexture2D( "DiffuseMap", rdata->textureResPath.c_str() );
	set->SetEffect( effect );

	EveSpriteSetPtr glow;
	glow.CreateInstance();
	Tr2EffectPtr glowEffect;
	glowEffect.CreateInstance();
	glowEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/Booster/BoosterGlow.fx" );
	glowEffect->AddResourceTexture2D( "DiffuseMap", "res:/Texture/Particle/whitesharp.dds" );
	glow->SetEffect( glowEffect );
	set->SetGlow( glow );

	if( hdata->hasTrails )
	{
		Tr2EffectPtr trailEffect;
		trailEffect.CreateInstance();
		EveTrailsSetPtr trail;
		trail.CreateInstance();
		trailEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/Booster/VolumetricTrails.fx" );
		trailEffect->AddParameterVector4( "TrailSize", &rdata->trailSize );
		trailEffect->AddParameterColor( "TrailColor", &rdata->trailColor );
		trail->SetEffect( trailEffect );
		trail->SetMeshResPath( "res:/dx9/model/ship/booster/volumetricTrail.gr2" );
		set->SetTrail( trail );
	}

	// add all the indiviual items
	for( auto biit = hdata->items.begin(); biit != hdata->items.end(); ++biit )
	{
		set->Add( &biit->transform, &biit->functionality );
	}

	// add it to ship
	set->PrepareResources();
	ship->SetBoosterSet( set );
}
// --------------------------------------------------------------------------------
// Description:
//   add the hull decals to the new ship
// --------------------------------------------------------------------------------
void EveSOF::SetupHullDecals( EveShip2Ptr ship, const EveSOFDataMgr::HullData* hullData ) const
{
	// create and setup all hull decals
	for( auto hdit = hullData->hullDecals.begin(); hdit != hullData->hullDecals.end(); ++hdit )
	{
		// create
		EveSpaceObjectDecalPtr decal;
		decal.CreateInstance();
		// set general datas
		decal->SetPosition( hdit->position );
		decal->SetRotation( hdit->rotation );
		decal->SetScaling( hdit->scaling );
		// the decal effect
		Tr2EffectPtr shader;
		shader.CreateInstance();
		shader->SetEffectPathName( hdit->shaderPath.c_str() );
		// set parameters
		for( auto hdpit = hdit->parameters.begin(); hdpit != hdit->parameters.end(); ++hdpit )
		{
			shader->AddParameterVector4( hdpit->first.c_str(), &hdpit->second );
		}
		// set textures
		for( auto hdtit = hdit->textures.begin(); hdtit != hdit->textures.end(); ++hdtit )
		{
			shader->AddResourceTexture2D( hdtit->first.c_str(), hdtit->second.resFilePath.c_str() );
		}
		// init and add
		decal->SetEffect( shader );
		decal->Initialize();
		ship->AddDecal( decal );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Takes a texture resPath and checks if there is a factional version of this.
// --------------------------------------------------------------------------------
void EveSOF::ModifyTextureResPath( std::string& resPath, const char* name, const EveSOFDataMgr::FactionData* factionData ) const
{
	if( !strcmp( name, "PgsMap" ) && factionData->resPathInsert.size() )
	{
		std::string resPathCopy = resPath;
		size_t index = resPath.rfind("/");
		if( index != std::string::npos )
		{
			resPathCopy.insert( index + 1, factionData->resPathInsert + std::string("/") );
		}

		index = resPathCopy.rfind("_pgs");
		if( index  != std::string::npos )
		{
			resPathCopy.insert( index, "_" + factionData->resPathInsert );

			std::wstring wstrCopy( resPathCopy.begin(), resPathCopy.end() );
			if( BePaths->FileExists( wstrCopy ) )
			{
				resPath = resPathCopy;
			}
		}
	}
}





