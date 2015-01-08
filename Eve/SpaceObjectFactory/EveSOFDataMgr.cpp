////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "EveSOFDataMgr.h"
#include "EveSOFData.h"

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveSOFDataMgr::EveSOFDataMgr( IRoot* lockobj )
{
}

// --------------------------------------------------------------------------------
// Description:
//   tear down
// --------------------------------------------------------------------------------
EveSOFDataMgr::~EveSOFDataMgr()
{

}

// --------------------------------------------------------------------------------
// Description:
//   check if hull data is there. Mainly for debug reason!
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::HasHullData( const char* hullName ) const
{
	std::map<std::string, HullData>::const_iterator finder = m_hullData.find( hullName );
	return finder != m_hullData.end();
}

// --------------------------------------------------------------------------------
// Description:
//   Access to hulldata, only const pointer!!
// --------------------------------------------------------------------------------
const EveSOFDataMgr::HullData* EveSOFDataMgr::GetHullData( const char* hullName ) const
{
	std::map<std::string, HullData>::const_iterator finder = m_hullData.find( hullName );
	if( finder == m_hullData.end() )
	{
		return nullptr;
	}
	return &finder->second;
}

// --------------------------------------------------------------------------------
// Description:
//   check if faction data is there. Mainly for debug reason!
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::HasFactionData( const char* factionName ) const
{
	std::map<std::string, FactionData>::const_iterator finder = m_factionData.find( factionName );
	return finder != m_factionData.end();
}

// --------------------------------------------------------------------------------
// Description:
//   Access to factiondata, only const pointer!!
// --------------------------------------------------------------------------------
const EveSOFDataMgr::FactionData* EveSOFDataMgr::GetFactionData( const char* factionName ) const
{
	std::map<std::string, FactionData>::const_iterator finder = m_factionData.find( factionName );
	if( finder == m_factionData.end() )
	{
		return nullptr;
	}
	return &finder->second;
}

// --------------------------------------------------------------------------------
// Description:
//   check if race data is there. Mainly for debug reason!
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::HasRaceData( const char* raceName ) const
{
	std::map<std::string, RaceData>::const_iterator finder = m_raceData.find( raceName );
	return finder != m_raceData.end();
}

// --------------------------------------------------------------------------------
// Description:
//   Access to racedata, only const pointer!!
// --------------------------------------------------------------------------------
const EveSOFDataMgr::RaceData* EveSOFDataMgr::GetRaceData( const char* raceName ) const
{
	std::map<std::string, RaceData>::const_iterator finder = m_raceData.find( raceName );
	if( finder == m_raceData.end() )
	{
		return nullptr;
	}
	return &finder->second;
}

// --------------------------------------------------------------------------------
// Description:
//   check if material data is there. Mainly for debug reason!
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::HasMaterialData( const char* materialName ) const
{
	std::map<std::string, MaterialData>::const_iterator finder = m_materialData.find( materialName );
	return finder != m_materialData.end();
}

// --------------------------------------------------------------------------------
// Description:
//   Access to materialdata, only const pointer!!
// --------------------------------------------------------------------------------
const EveSOFDataMgr::MaterialData* EveSOFDataMgr::GetMaterialData( const char* materialName ) const
{
	std::map<std::string, MaterialData>::const_iterator finder = m_materialData.find( materialName );
	if( finder == m_materialData.end() )
	{
		return nullptr;
	}
	return &finder->second;
}

// --------------------------------------------------------------------------------
// Description:
//   Access to genericdata, only const pointer!!
// --------------------------------------------------------------------------------
const EveSOFDataMgr::GenericData* EveSOFDataMgr::GetGenericData() const
{
	return &m_genericData;
}

// --------------------------------------------------------------------------------
// Description:
//   Update an individual hull, identified by it's name
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::UpdateHull( const char* hullName, EveSOFDataHull* hullData )
{
	// must exist
	if( !HasHullData( hullName ) )
	{
		CCP_LOGWARN( "Trying to update a hull which does not exist: %s", hullName );
		return false;
	}

	// fill the non-trinity struct with the provided data
	HullData hd;
	GenerateHullData( hd, hullData );

	// set it to the main map
	m_hullData[ hullName ] = hd;

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Update an individual faction, identified by it's name
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::UpdateFaction( const char* factionName, EveSOFDataFaction* factionData )
{
	// must exist
	if( !HasFactionData( factionName ) )
	{
		CCP_LOGWARN( "Trying to update a faction which does not exist: %s", factionName );
		return false;
	}

	// fill the non-trinity struct with the provided data
	FactionData fd;
	GenerateFactionData( fd, factionData );

	// set it to the main map
	m_factionData[ factionName ] = fd;

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Update an individual race, identified by it's name
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::UpdateRace( const char* raceName, EveSOFDataRace* raceData )
{
	// must exist
	if( !HasRaceData( raceName ) )
	{
		CCP_LOGWARN( "Trying to update a faction which does not exist: %s", raceName );
		return false;
	}

	// fill the non-trinity struct with the provided data
	RaceData rd;
	GenerateRaceData( rd, raceData );

	// set it to the main map
	m_raceData[ raceName ] = rd;

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Update an individual material, identified by it's name
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::UpdateMaterial( const char* materialName, EveSOFDataMaterial* materialData )
{
	// must exist
	if( !HasMaterialData( materialName ) )
	{
		CCP_LOGWARN( "Trying to update a material which does not exist: %s", materialName );
		return false;
	}

	// fill the non-trinity struct with the provided data
	MaterialData md;
	GenerateMaterialData( md, materialData );

	// set it to the main map
	m_materialData[ materialName ] = md;

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Here we get a blue object which is the whole database and use it to
//   set internal containers with all SOF db data
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::SetData( EveSOFData* dbData )
{
	// clear existing data
	m_hullData.clear();
	m_factionData.clear();
	m_raceData.clear();
	m_materialData.clear();

	// load hull data
	if(!LoadHullData( dbData ) )
	{
		CCP_LOGERR( "Error loading hull data!" );
		return false; 
	}
	CCP_LOGNOTICE( "SOF: loaded %d hulls", m_hullData.size() );

	// load faction data
	if(!LoadFactionData( dbData ) )
	{
		CCP_LOGERR( "Error loading faction data!" );
		return false;
	}
	CCP_LOGNOTICE( "SOF: loaded %d factions", m_factionData.size() );

	// load race data
	if(!LoadRaceData( dbData ) )
	{
		CCP_LOGERR( "Error loading race data!" );
		return false;
	}
	CCP_LOGNOTICE( "SOF: loaded %d races", m_raceData.size() );

	// load material data
	if(!LoadMaterialData( dbData ) )
	{
		CCP_LOGERR( "Error loading material data!" );
		return false;
	}
	CCP_LOGNOTICE( "SOF: loaded %d materials", m_materialData.size() );

	// load generic data
	if(!LoadGenericData( dbData ) )
	{
		CCP_LOGERR( "Error loading generic data!" );
		return false;
	}
	CCP_LOGNOTICE( "SOF: loaded generics" );

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Here we load this blue object, read data from it to store it in this class
//   internally and then release the blue object
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::LoadData( const char* filePath )
{
	CCP_LOGNOTICE( "SOF: start loading data from: %s", filePath );

	// load via resman
	IRootPtr p;
	p.Attach( BeResMan->LoadObject( filePath ) );
	if( p == NULL )
	{
		CCP_LOGERR( "Couldn't find SOF db data resource file: %s", filePath );
		return false;
	}

	// is it of right type?
	EveSOFDataPtr dbData;
	if( !p->QueryInterface( BlueInterfaceIID<EveSOFData>(), (void**)&dbData ) )
	{
		CCP_LOGERR( "SOF resource file %s is not of correct type!", filePath );
		return false;
	}

	// set it, so we create the stl containers in here
	if( !SetData( dbData ) )
	{
		CCP_LOGERR( "resource set SOF db data resource file: %s", filePath );
		return false;
	}
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Create hull area from sof db data
// --------------------------------------------------------------------------------
EveSOFDataMgr::HullAreas EveSOFDataMgr::LoadHullAreaData( const EveSOFDataHullAreaPtr areaData ) const
{
	HullAreas ha;
	ha.index = areaData->m_index;
	ha.count = areaData->m_count;
	ha.designation = areaData->m_name;
	ha.shader = areaData->m_shader;
	for( auto matit = areaData->m_textures.begin(); matit != areaData->m_textures.end(); ++matit )
	{
		EveSOFDataTexturePtr textureData = (*matit);

		TextureData td;
		td.resFilePath = textureData->m_resFilePath;
		ha.textures[textureData->m_name] = td;
	}
	for( auto paramIt = areaData->m_parameters.begin(); paramIt != areaData->m_parameters.end(); paramIt++ )
	{
		EveSOFDataParameter* param = *paramIt;
		ha.parameters[param->m_name] = param->m_value;
	}
	return ha;
}

// --------------------------------------------------------------------------------
// Description:
//   Init hull-specific data
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::LoadHullData( EveSOFDataPtr srcData )
{
	// store that data from that object internally
	for( EveSOFDataHullVector::const_iterator it = srcData->m_hull.begin(); it != srcData->m_hull.end(); ++it )
	{
		EveSOFDataHullPtr hullData = (*it);

		// if this hull is already there, we have a problem!
		if( m_hullData.find( hullData->m_name ) != m_hullData.end() )
		{
			CCP_LOGERR( "Found a duplicate hull name: %s", hullData->m_name.c_str() );
			return false;
		}

		// fill the non-trinity struct with the provided data
		HullData hd;
		GenerateHullData( hd, hullData );

		// put it into the main map
		m_hullData[(*it)->m_name] = hd;
	}
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Fill a non-trinity hull data struct with all the data from the trinity
//   data struct
// --------------------------------------------------------------------------------
void EveSOFDataMgr::GenerateHullData( HullData& hd, EveSOFDataHullPtr srcData ) const
{
	// insert data
	hd.geometryResFilePath = srcData->m_geometryResFilePath;
	hd.boundingSphere = srcData->m_boundingSphere;
	hd.isSkinned = srcData->m_isSkinned;
	hd.audioPosition = srcData->m_audioPosition;

	// boosters
	if( srcData->m_booster )
	{
		EveSOFDataHullBoosterPtr boosterData = srcData->m_booster;

		HullBoosterData hbd;
		hbd.alwaysOn = boosterData->m_alwaysOn;
		hbd.hasTrails = boosterData->m_hasTrails;

		// booster items
		for( auto biit = boosterData->m_items.begin(); biit != boosterData->m_items.end(); ++biit )
		{
			EveSOFDataHullBoosterItemPtr boosterItemData = (*biit);

			HullBoosterItemData hbid;
			hbid.transform = boosterItemData->m_transform;
			hbid.functionality = boosterItemData->m_functionality;
			hbid.hasTrail = boosterItemData->m_hasTrail;

			hbd.items.push_back( hbid );
		}

		hd.boosters = hbd;
	}

	// spritesets
	for( auto ssit = srcData->m_spriteSets.begin(); ssit != srcData->m_spriteSets.end(); ++ssit )
	{
		EveSOFDataHullSpriteSetPtr spriteSetData = (*ssit);

		HullSpriteSetData hssd;
		hssd.skinned = spriteSetData->m_skinned;
		for( auto ssiit = spriteSetData->m_items.begin(); ssiit != spriteSetData->m_items.end(); ++ssiit )
		{
			EveSOFDataHullSpriteSetItemPtr spriteSetItemData = (*ssiit);

			HullSpriteSetItemData hssid;
			hssid.blinkPhase = spriteSetItemData->m_blinkPhase;
			hssid.blinkRate = spriteSetItemData->m_blinkRate;
			hssid.boneIndex = spriteSetItemData->m_boneIndex;
			hssid.falloff = spriteSetItemData->m_falloff;
			hssid.maxScale = spriteSetItemData->m_maxScale;
			hssid.minScale = spriteSetItemData->m_minScale;
			hssid.position = spriteSetItemData->m_position;
			hssid.groupIndex = spriteSetItemData->m_groupIndex;
			hssd.m_items.push_back( hssid );
		}
		hd.spriteSets.push_back( hssd );
	}

	// spotlightsets
	for( auto ssit = srcData->m_spotlightSets.begin(); ssit != srcData->m_spotlightSets.end(); ++ssit )
	{
		EveSOFDataHullSpotlightSetPtr spotlightSetData = (*ssit);

		HullSpotlightSetData hssd;
		hssd.skinned = spotlightSetData->m_skinned;
		hssd.zOffset = spotlightSetData->m_zOffset;
		hssd.coneTextureResPath = spotlightSetData->m_coneTextureResPath;
		hssd.glowTextureResPath = spotlightSetData->m_glowTextureResPath;
		for( auto ssiit = spotlightSetData->m_items.begin(); ssiit != spotlightSetData->m_items.end(); ++ssiit )
		{
			EveSOFDataHullSpotlightSetItemPtr spotlightSetItemData = (*ssiit);

			HullSpotlightSetItemData hssid;
			hssid.boneIndex = spotlightSetItemData->m_boneIndex;
			hssid.boosterGainInfluence = spotlightSetItemData->m_boosterGainInfluence;
			hssid.spriteScale = spotlightSetItemData->m_spriteScale;
			hssid.transform = spotlightSetItemData->m_transform;
			hssid.groupIndex = spotlightSetItemData->m_groupIndex;
			hssid.coneIntensity = spotlightSetItemData->m_coneIntensity;
			hssid.flareIntensity = spotlightSetItemData->m_flareIntensity;
			hssid.spriteIntensity = spotlightSetItemData->m_spriteIntensity;
			hssd.items.push_back( hssid );
		}
		hd.spotlightSets.push_back( hssd );
	}

	// planesets
	for( auto psit = srcData->m_planeSets.begin(); psit != srcData->m_planeSets.end(); ++psit )
	{
		EveSOFDataHullPlaneSetPtr planeSetData = (*psit);

		HullPlaneSetData hpsd;
		hpsd.layer1MapResPath = planeSetData->m_layer1MapResPath;
		hpsd.layer2MapResPath = planeSetData->m_layer2MapResPath;
		hpsd.maskMapResPath = planeSetData->m_maskMapResPath;
		hpsd.planeData = planeSetData->m_planeData;
		hpsd.skinned = planeSetData->m_skinned;
		for( auto psiit = planeSetData->m_items.begin(); psiit != planeSetData->m_items.end(); ++psiit )
		{
			EveSOFDataHullPlaneSetItemPtr planeSetItemData = (*psiit);

			HullPlaneSetItemData pssid;
			pssid.boneIndex = planeSetItemData->m_boneIndex;
			pssid.layer1Scroll = planeSetItemData->m_layer1Scroll;
			pssid.layer1Transform = planeSetItemData->m_layer1Transform;
			pssid.layer2Scroll = planeSetItemData->m_layer2Scroll;
			pssid.layer2Transform = planeSetItemData->m_layer2Transform;
			pssid.position = planeSetItemData->m_position;
			pssid.rotation = planeSetItemData->m_rotation;
			pssid.scaling = planeSetItemData->m_scaling;
			pssid.color = planeSetItemData->m_color;
			pssid.groupIndex = planeSetItemData->m_groupIndex;
			hpsd.items.push_back( pssid );
		}
		hd.planeSets.push_back( hpsd );
	}

	// hulldecals
	for( auto hdit = srcData->m_hullDecals.begin(); hdit != srcData->m_hullDecals.end(); ++hdit )
	{
		EveSOFDataHullDecalPtr hullDecal = (*hdit);

		HullDecalData hdd;
		hdd.position = hullDecal->m_position;
		hdd.rotation = hullDecal->m_rotation;
		hdd.scaling = hullDecal->m_scaling;
		hdd.groupIndex = hullDecal->m_groupIndex;
		hdd.boneIndex = hullDecal->m_boneIndex;
		hdd.indexBuffer.insert( hdd.indexBuffer.begin(), hullDecal->m_indexBuffer.begin(), hullDecal->m_indexBuffer.end() );
		hdd.shader = hullDecal->m_shader;
		for( auto hdtit = hullDecal->m_textures.begin(); hdtit != hullDecal->m_textures.end(); ++hdtit )
		{
			EveSOFDataTexturePtr textureData = (*hdtit);

			TextureData td;
			td.resFilePath = textureData->m_resFilePath;
			hdd.textures[textureData->m_name] = td;
		}
		for( auto hdpit = hullDecal->m_parameters.begin(); hdpit != hullDecal->m_parameters.end(); ++hdpit )
		{
			EveSOFDataParameterPtr parameterData = (*hdpit);

			hdd.parameters[parameterData->m_name] = parameterData->m_value;
		}
		hd.hullDecals.push_back( hdd );
	}

	// meshareas
	for( auto mait = srcData->m_opaqueAreas.begin(); mait != srcData->m_opaqueAreas.end(); ++mait )
	{
		EveSOFDataHullAreaPtr areaData = (*mait);
		hd.opaqueAreas.push_back( LoadHullAreaData( areaData ) );
	}
	for( auto mait = srcData->m_decalAreas.begin(); mait != srcData->m_decalAreas.end(); ++mait )
	{
		EveSOFDataHullAreaPtr areaData = (*mait);
		hd.decalAreas.push_back( LoadHullAreaData( areaData ) );
	}
	for( auto mait = srcData->m_transparentAreas.begin(); mait != srcData->m_transparentAreas.end(); ++mait )
	{
		EveSOFDataHullAreaPtr areaData = (*mait);
		hd.transparentAreas.push_back( LoadHullAreaData( areaData ) );
	}
	for( auto mait = srcData->m_additiveAreas.begin(); mait != srcData->m_additiveAreas.end(); ++mait )
	{
		EveSOFDataHullAreaPtr areaData = (*mait);
		hd.additiveAreas.push_back( LoadHullAreaData( areaData ) );
	}
	for( auto mait = srcData->m_distortionAreas.begin(); mait != srcData->m_distortionAreas.end(); ++mait )
	{
		EveSOFDataHullAreaPtr areaData = (*mait);
		hd.distortionAreas.push_back( LoadHullAreaData( areaData ) );
	}
	for( auto mait = srcData->m_depthAreas.begin(); mait != srcData->m_depthAreas.end(); ++mait )
	{
		EveSOFDataHullAreaPtr areaData = (*mait);
		hd.depthAreas.push_back( LoadHullAreaData( areaData ) );
	}

	// turret locators
	for( auto tlit = srcData->m_locatorTurrets.begin(); tlit != srcData->m_locatorTurrets.end(); ++tlit )
	{
		EveSOFDataHullLocatorPtr locatorData = (*tlit);

		LocatorData ld;
		ld.name = locatorData->m_name;
		ld.transform = locatorData->m_transform;
		hd.locatorTurrets.push_back( ld );
	}

	// damage locators
	for( auto dlit = srcData->m_damageLocators.begin(); dlit != srcData->m_damageLocators.end(); ++dlit )
	{
		EveSOFDataTransformPtr locatorData = (*dlit);

		LocatorDirectionData ldd;
		ldd.position = locatorData->m_position;
		ldd.rotation = locatorData->m_rotation;
		hd.locatorDamage.push_back( ldd );
	}

	// children
	for( auto chit = srcData->m_children.begin(); chit != srcData->m_children.end(); ++chit )
	{
		EveSOFDataHullChildPtr child = (*chit);
		HullChild hc;
		hc.redFilePath = child->m_redFilePath;
		hc.translation = child->m_translation;
		hc.rotation = child->m_rotation;
		hc.scaling = child->m_scaling;
		hc.id = child->m_id;
		hd.children.push_back( hc );
	}

	// animations
	for( auto chit = srcData->m_animations.begin(); chit != srcData->m_animations.end(); ++chit )
	{
		EveSOFDataHullAnimationPtr anim = (*chit);
		HullAnimation ha;

		ha.name = anim->m_name;

		ha.startRotationTime = anim->m_startRotationTime;
		ha.endRotationTime = anim->m_endRotationTime;
		ha.startRotationValue = anim->m_startRotationValue;
		ha.endRotationValue = anim->m_endRotationValue;
		
		ha.startTranslationTime = anim->m_startTranslationTime;
		ha.endTranslationTime = anim->m_endTranslationTime;
		ha.startTranslationValue = anim->m_startTranslationValue;
		ha.endTranslationValue = anim->m_endTranslationValue;
		
		ha.startRate = anim->m_startRate;
		ha.endRate = anim->m_endRate;
		ha.id = anim->m_id;

		hd.animations.push_back( ha );
	}

	// model curves
	hd.modelRotationCurvePath = srcData->m_modelRotationCurvePath;
	hd.modelTranslationCurvePath = srcData->m_modelTranslationCurvePath;
}

// --------------------------------------------------------------------------------
// Description:
//   Init faction-specific data
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::LoadFactionData( EveSOFDataPtr srcData )
{
	// store that data from that object internally
	for( EveSOFDataFactionVector::const_iterator it = srcData->m_faction.begin(); it != srcData->m_faction.end(); ++it )
	{
		EveSOFDataFactionPtr factionData = (*it);

		// if this hull is already there, we have a problem!
		if( m_factionData.find( factionData->m_name ) != m_factionData.end() )
		{
			CCP_LOGERR( "Found a duplicate faction name: %s", factionData->m_name.c_str() );
			return false;
		}

		// fill the non-trinity struct with the provided data
		FactionData fd;
		GenerateFactionData( fd, factionData );

		// put it into the main map
		m_factionData[(*it)->m_name] = fd;
	}

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Fill a non-trinity faction data struct with all the data from the trinity
//   data struct
// --------------------------------------------------------------------------------
void EveSOFDataMgr::GenerateFactionData( FactionData& fd, EveSOFDataFactionPtr srcData ) const
{
	// insert data
	fd.resPathInsert = srcData->m_resPathInsert;

	// material usage ids
	fd.materialUsageMain = srcData->m_materialUsageMain;
	fd.materialUsageMask = srcData->m_materialUsageMask;

	// sprite set colors
	for( auto sscit = srcData->m_spriteSets.begin(); sscit != srcData->m_spriteSets.end(); ++sscit )
	{
		EveSOFDataFactionSpriteSetPtr spriteSetData = (*sscit);

		FactionSpriteSetColorData sscd;
		sscd.isVisible = spriteSetData->m_isVisible;
		sscd.color = spriteSetData->m_color;

		fd.spriteSetsColor[spriteSetData->m_groupIndex] = sscd;
	}

	// spotlight set colors
	for( auto spotcit = srcData->m_spotlightSets.begin(); spotcit != srcData->m_spotlightSets.end(); ++spotcit )
	{
		EveSOFDataFactionSpotlightSetPtr spotlightSetData = (*spotcit);

		FactionSpotlightSetColorData spotcd;
		spotcd.coneColor = spotlightSetData->m_coneColor;
		spotcd.flareColor = spotlightSetData->m_flareColor;
		spotcd.spriteColor = spotlightSetData->m_spriteColor;

		fd.spotlightSetsColors[spotlightSetData->m_groupIndex] = spotcd;
	}

	// planeset set colors
	for( auto plcit = srcData->m_planeSets.begin(); plcit != srcData->m_planeSets.end(); ++plcit )
	{
		EveSOFDataFactionPlaneSetPtr planeSetData = (*plcit);

		FactionPlaneSetColorData plscd;
		plscd.color = planeSetData->m_color;

		fd.planeSetsColors[planeSetData->m_groupIndex] = plscd;
	}

	// decal faction data
	for( auto ddit = srcData->m_decals.begin(); ddit != srcData->m_decals.end(); ++ddit )
	{
		EveSOFDataFactionDecalPtr decalData = (*ddit);

		FactionDecalData fdd;
		fdd.isVisible = decalData->m_isVisible;
		fdd.shader = decalData->m_shader;
		for( auto ddpit = decalData->m_parameters.begin(); ddpit != decalData->m_parameters.end(); ++ddpit )
		{
			EveSOFDataParameterPtr parameterData = (*ddpit);
			fdd.parameters[parameterData->m_name] = parameterData->m_value;
		}
		for( auto ddtit = decalData->m_textures.begin(); ddtit != decalData->m_textures.end(); ++ddtit )
		{
			EveSOFDataTexturePtr textureData = (*ddtit);

			TextureData td;
			td.resFilePath = textureData->m_resFilePath;
			fdd.textures[textureData->m_name] = td;
		}

		fd.decalData[decalData->m_groupIndex] = fdd;
	}

	// area parameters
	for( auto hait = srcData->m_areas.begin(); hait != srcData->m_areas.end(); ++hait )
	{
		EveSOFDataFactionHullAreaPtr hullAreaData = (*hait);

		FactionAreaData ad;
		for( auto hapit = hullAreaData->m_parameters.begin(); hapit != hullAreaData->m_parameters.end(); ++hapit )
		{
			EveSOFDataParameterPtr parameterData = (*hapit);
			ad.parameters[parameterData->m_name] = parameterData->m_value;
		}
		fd.areaParameters[hullAreaData->m_name] = ad;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Init race-specific data
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::LoadRaceData( EveSOFDataPtr srcData )
{
	// store that data from that object internally
	for( EveSOFDataRaceVector::const_iterator it = srcData->m_race.begin(); it != srcData->m_race.end(); ++it )
	{
		EveSOFDataRacePtr raceData = (*it);

		// if this hull is already there, we have a problem!
		if( m_raceData.find( raceData->m_name ) != m_raceData.end() )
		{
			CCP_LOGERR( "Found a duplicate race name: %s", raceData->m_name.c_str() );
			return false;
		}

		// fill the non-trinity struct with the provided data
		RaceData rd;
		GenerateRaceData( rd, raceData );

		// put it into the main map
		m_raceData[(*it)->m_name] = rd;
	}

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Fill a non-trinity race data struct with all the data from the trinity
//   data struct
// --------------------------------------------------------------------------------
void EveSOFDataMgr::GenerateRaceData( RaceData& rd, EveSOFDataRacePtr srcData ) const
{
	// booster data
	rd.boosters.color = srcData->m_booster->m_color;
	rd.boosters.scale = srcData->m_booster->m_scale;
	rd.boosters.textureResPath = srcData->m_booster->m_textureResPath;
	rd.boosters.glowScale = srcData->m_booster->m_glowScale;
	rd.boosters.glowColor = srcData->m_booster->m_glowColor;
	rd.boosters.haloColor = srcData->m_booster->m_haloColor;
	rd.boosters.haloScaleX = srcData->m_booster->m_haloScaleX;
	rd.boosters.haloScaleY = srcData->m_booster->m_haloScaleY;
	rd.boosters.symHaloScale = srcData->m_booster->m_symHaloScale;
	rd.boosters.trailColor = srcData->m_booster->m_trailColor;
	rd.boosters.trailSize = srcData->m_booster->m_trailSize;
}

// --------------------------------------------------------------------------------
// Description:
//   Init material-specific data
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::LoadMaterialData( EveSOFDataPtr srcData )
{
	// store that data from that object internally
	for( EveSOFDataMaterialVector::const_iterator it = srcData->m_material.begin(); it != srcData->m_material.end(); ++it )
	{
		EveSOFDataMaterialPtr materialData = (*it);

		// if this material is already there, we have a problem!
		if( m_materialData.find( materialData->m_name ) != m_materialData.end() )
		{
			CCP_LOGERR( "Found a duplicate material name: %s", materialData->m_name.c_str() );
			return false;
		}

		// fill the non-trinity struct with the provided data
		MaterialData md;
		GenerateMaterialData( md, materialData );

		// put it into the main map
		m_materialData[(*it)->m_name] = md;
	}

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Fill a non-trinity material data struct with all the data from the trinity
//   data struct
// --------------------------------------------------------------------------------
void EveSOFDataMgr::GenerateMaterialData( MaterialData& rd, EveSOFDataMaterialPtr srcData ) const
{
	// parameter data
	for( auto mpit = srcData->m_parameters.begin(); mpit != srcData->m_parameters.end(); ++mpit )
	{
		EveSOFDataParameterPtr parameterData = (*mpit);

		rd.parameters[parameterData->m_name] = parameterData->m_value;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Init generic data
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::LoadGenericData( EveSOFDataPtr srcData )
{
	// shader locations
	m_genericData.shaderPrefix = srcData->m_generic->m_shaderPrefix;
	m_genericData.shaderPrefixAnimated = srcData->m_generic->m_shaderPrefixAnimated;
	m_genericData.areaShaderLocation = srcData->m_generic->m_areaShaderLocation;
	m_genericData.decalShaderLocation = srcData->m_generic->m_decalShaderLocation;

	return true;
}



