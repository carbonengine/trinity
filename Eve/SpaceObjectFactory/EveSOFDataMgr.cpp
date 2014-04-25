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
		return NULL;
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
		return NULL;
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
		return NULL;
	}
	return &finder->second;
}

// --------------------------------------------------------------------------------
// Description:
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::SetData( IRootPtr dbData )
{
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
		CCP_LOGERR( "Couldn't find hull data resource file: %s", filePath );
		return false;
	}
	// is it of right type?
	EveSOFDataPtr srcData;
	if( !p->QueryInterface( BlueInterfaceIID<EveSOFData>(), (void**)&srcData ) )
	{
		CCP_LOGERR( "resource file %s is not of correct type!", filePath );
		return false;
	}

	// load hull data
	if(!LoadHullData( srcData ) )
	{
		CCP_LOGERR( "Error loading hull data from %s!", filePath );
		return false; 
	}
	CCP_LOGNOTICE( "SOF: loaded %d hulls", m_hullData.size() );

	// load faction data
	if(!LoadFactionData( srcData ) )
	{
		CCP_LOGERR( "Error loading faction data from %s!", filePath );
		return false;
	}
	CCP_LOGNOTICE( "SOF: loaded %d factions", m_factionData.size() );

	// load race data
	if(!LoadRaceData( srcData ) )
	{
		CCP_LOGERR( "Error loading race data from %s!", filePath );
		return false;
	}
	CCP_LOGNOTICE( "SOF: loaded %d races", m_raceData.size() );

	return true;
}


// --------------------------------------------------------------------------------
// Description:
//   Create hull area from sof db data
// --------------------------------------------------------------------------------
EveSOFDataMgr::HullAreas EveSOFDataMgr::LoadHullAreaData( const EveSOFDataHullAreaPtr areaData )
{
	HullAreas ha;
	ha.index = areaData->m_index;
	ha.count = areaData->m_count;
	ha.designation = areaData->m_name;
	ha.shaderPath = areaData->m_shaderPath;
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

		// insert data
		HullData hd;
		hd.geometryResFilePath = hullData->m_geometryResFilePath;
		hd.boundingSphere = hullData->m_boundingSphere;
		hd.isSkinned = hullData->m_isSkinned;

		// boosters
		if( hullData->m_booster )
		{
			EveSOFDataHullBoosterPtr boosterData = hullData->m_booster;

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

				hbd.items.push_back( hbid );
			}

			hd.boosters = hbd;
		}

		// spritesets
		for( auto ssit = hullData->m_spriteSets.begin(); ssit != hullData->m_spriteSets.end(); ++ssit )
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
		for( auto ssit = hullData->m_spotlightSets.begin(); ssit != hullData->m_spotlightSets.end(); ++ssit )
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
				hssd.items.push_back( hssid );
			}
			hd.spotlightSets.push_back( hssd );
		}

		// planesets
		for( auto psit = hullData->m_planeSets.begin(); psit != hullData->m_planeSets.end(); ++psit )
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
				hpsd.items.push_back( pssid );
			}
			hd.planeSets.push_back( hpsd );
		}

		// hulldecals
		for( auto hdit = hullData->m_hullDecals.begin(); hdit != hullData->m_hullDecals.end(); ++hdit )
		{
			EveSOFDataHullDecalPtr hullDecal = (*hdit);

			HullDecalData hdd;
			hdd.position = hullDecal->m_position;
			hdd.rotation = hullDecal->m_rotation;
			hdd.scaling = hullDecal->m_scaling;
			hdd.shaderPath = hullDecal->m_shaderPath;
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
		for( auto mait = hullData->m_opaqueAreas.begin(); mait != hullData->m_opaqueAreas.end(); ++mait )
		{
			EveSOFDataHullAreaPtr areaData = (*mait);
			hd.opaqueAreas.push_back( LoadHullAreaData( areaData ) );
		}
		for( auto mait = hullData->m_transparentAreas.begin(); mait != hullData->m_transparentAreas.end(); ++mait )
		{
			EveSOFDataHullAreaPtr areaData = (*mait);
			hd.transparentAreas.push_back( LoadHullAreaData( areaData ) );
		}
		for( auto mait = hullData->m_additiveAreas.begin(); mait != hullData->m_additiveAreas.end(); ++mait )
		{
			EveSOFDataHullAreaPtr areaData = (*mait);
			hd.additiveAreas.push_back( LoadHullAreaData( areaData ) );
		}
		for( auto mait = hullData->m_distortionAreas.begin(); mait != hullData->m_distortionAreas.end(); ++mait )
		{
			EveSOFDataHullAreaPtr areaData = (*mait);
			hd.distortionAreas.push_back( LoadHullAreaData( areaData ) );
		}
		for( auto mait = hullData->m_depthAreas.begin(); mait != hullData->m_depthAreas.end(); ++mait )
		{
			EveSOFDataHullAreaPtr areaData = (*mait);
			hd.depthAreas.push_back( LoadHullAreaData( areaData ) );
		}

		// turret locators
		for( auto tlit = hullData->m_locatorTurrets.begin(); tlit != hullData->m_locatorTurrets.end(); ++it )
		{
			EveSOFDataHullLocatorPtr locatorData = (*tlit);

			LocatorData ld;
			ld.name = locatorData->m_name;
			ld.transform = locatorData->m_transform;
			hd.locatorTurrets.push_back( ld );
		}

		// audio locators
		for( auto alit = hullData->m_locatorAudio.begin(); alit != hullData->m_locatorAudio.end(); ++it )
		{
			EveSOFDataHullLocatorPtr locatorData = (*alit);

			LocatorData ld;
			ld.name = locatorData->m_name;
			ld.transform = locatorData->m_transform;
			hd.locatorAudio.push_back( ld );
		}

		// children
		for( auto chit = hullData->m_children.begin(); chit != hullData->m_children.end(); ++chit )
		{
			EveSOFDataHullChildPtr child = (*chit);
			HullChild hc;
			hc.redFilePath = child->m_redFilePath;
			hc.translation = child->m_translation;
			hc.rotation = child->m_rotation;
			hc.scaling = child->m_scaling;
			hd.children.push_back( hc );
		}

		m_hullData[(*it)->m_name] = hd;
	}


	return true;
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

		// insert data
		FactionData fd;
		fd.resPathInsert = factionData->m_resPathInsert;

		// sprite set colors
		for( auto sscit = factionData->m_spriteSets.begin(); sscit != factionData->m_spriteSets.end(); ++sscit )
		{
			EveSOFDataFactionSpriteSetPtr spriteSetData = (*sscit);

			FactionSpriteSetColorData sscd;
			sscd.color = spriteSetData->m_color;

			fd.spriteSetsColor[spriteSetData->m_groupIndex] = sscd;
		}

		// area parameters
		for( auto hait = factionData->m_opaqueAreas.begin(); hait != factionData->m_opaqueAreas.end(); ++hait )
		{
			EveSOFDataFactionHullAreaPtr hullAreaData = (*hait);

			FactionAreaData ad;
			for( auto hapit = hullAreaData->m_parameters.begin(); hapit != hullAreaData->m_parameters.end(); ++hapit )
			{
				EveSOFDataParameterPtr parameterData = (*hapit);
				ad.parameters[parameterData->m_name] = parameterData->m_value;
			}
			fd.opaqueAreaParameters[hullAreaData->m_name] = ad;
		}
		for( auto hait = factionData->m_transparentAreas.begin(); hait != factionData->m_transparentAreas.end(); ++hait )
		{
			EveSOFDataFactionHullAreaPtr hullAreaData = (*hait);

			FactionAreaData ad;
			for( auto hapit = hullAreaData->m_parameters.begin(); hapit != hullAreaData->m_parameters.end(); ++hapit )
			{
				EveSOFDataParameterPtr parameterData = (*hapit);
				ad.parameters[parameterData->m_name] = parameterData->m_value;
			}
			fd.transparentAreaParameters[hullAreaData->m_name] = ad;
		}


		m_factionData[(*it)->m_name] = fd;
	}


	return true;
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

		// insert data
		RaceData rd;

		// booster data
		rd.boosterData.color = raceData->m_booster->m_color;
		rd.boosterData.scale = raceData->m_booster->m_scale;
		rd.boosterData.textureResPath = raceData->m_booster->m_textureResPath;
		rd.boosterData.glowScale = raceData->m_booster->m_glowScale;
		rd.boosterData.glowColor = raceData->m_booster->m_glowColor;
		rd.boosterData.haloColor = raceData->m_booster->m_haloColor;
		rd.boosterData.haloScaleX = raceData->m_booster->m_haloScaleX;
		rd.boosterData.haloScaleY = raceData->m_booster->m_haloScaleY;
		rd.boosterData.symHaloScale = raceData->m_booster->m_symHaloScale;
		rd.boosterData.trailColor = raceData->m_booster->m_trailColor;
		rd.boosterData.trailSize = raceData->m_booster->m_trailSize;

		m_raceData[(*it)->m_name] = rd;
	}

	return true;
}



