////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "EveSOFDataMgr.h"
#include "EveSOFUtils.h"
#include "Utilities/StringUtils.h"

namespace
{
uint32_t GetVisibilityGroupHash( const BlueSharedString visibilityGroup )
{
	return CcpHashFNV1( visibilityGroup.c_str(), std::string( visibilityGroup.c_str() ).length() );
}

}

EveSOFDataMgr::HullSpotlightSetItemData::HullSpotlightSetItemData( const EveSOFDataHullSpotlightSetItem& item ):
	boneIndex( item.m_boneIndex ),
	boosterGainInfluence( item.m_boosterGainInfluence ),
	colorType( item.m_colorType ),
	spriteScale( item.m_spriteScale ),
	transform( item.m_transform ),
	groupIndex( item.m_groupIndex ),
	coneIntensity( item.m_coneIntensity ),
	flareIntensity( item.m_flareIntensity ),
	spriteIntensity( item.m_spriteIntensity ),
	saturation( item.m_saturation ),
	light( nullptr )
{
	if (item.m_light) 
	{	
		light = std::make_unique<SpotLightAttachment>( *item.m_light );
	}
}

EveSOFDataMgr::HullSpotlightSetData::HullSpotlightSetData( const EveSOFDataHullSpotlightSet& spotLightSet ):
	skinned( spotLightSet.m_skinned ),
	zOffset( spotLightSet.m_zOffset ),
	visibilityGroup( GetVisibilityGroupHash( spotLightSet.m_visibilityGroup ) ),
	coneTextureResPath( spotLightSet.m_coneTextureResPath ),
	glowTextureResPath( spotLightSet.m_glowTextureResPath )
{
	items.reserve( spotLightSet.m_items.size() );
	for( auto& item : spotLightSet.m_items )
	{
		items.push_back( HullSpotlightSetItemData( *item ) );
	}
}

EveSOFDataMgr::HullPlaneSetItemData::HullPlaneSetItemData( const EveSOFDataHullPlaneSetItem& item ):
	boneIndex( item.m_boneIndex ),
	layer1Scroll( item.m_layer1Scroll ),
	layer1Transform( item.m_layer1Transform ),
	layer2Scroll( item.m_layer2Scroll ),
	layer2Transform( item.m_layer2Transform ),
	position( item.m_position ),
	rotation( item.m_rotation ),
	scaling( item.m_scaling ),
	color( item.m_color ),
	colorType( item.m_colorType ),
	intensity( item.m_intensity ),
	groupIndex( item.m_groupIndex ),
	maskMapAtlasIndex( item.m_maskMapAtlasIndex ),
	rate( item.m_rate ),
	phase( item.m_phase ),
	dutyCycle( item.m_dutyCycle ),
	blinkMode( item.m_blinkMode ),
	saturation( item.m_saturation )
{
	lights.reserve( item.m_lights.size() );
	for( auto& l : item.m_lights ) 
	{
		lights.push_back( PointLightAttachment( *l ) );
	}
}

EveSOFDataMgr::HullPlaneSetData::HullPlaneSetData( const EveSOFDataHullPlaneSet& planeSet ) :
	layer1MapResPath( planeSet.m_layer1MapResPath ),
	layer2MapResPath( planeSet.m_layer2MapResPath ),
	maskMapResPath( planeSet.m_maskMapResPath ),
	visibilityGroup( GetVisibilityGroupHash( planeSet.m_visibilityGroup ) ),
	atlasSize( planeSet.m_atlasSize ),
	atlasAspectRatio( planeSet.m_atlasAspectRatio ),
	skinned( planeSet.m_skinned ),
	usage( planeSet.m_usage )
{
	items.reserve( planeSet.m_items.size() );
	for( auto& planeSetItemData : planeSet.m_items )
	{
		items.push_back( HullPlaneSetItemData( *planeSetItemData ) );
	}
}


EveSOFDataMgr::PointLightAttachment::PointLightAttachment( const EveSOFDataPointLightAttachment& light ) {
	this->innerScaleMultiplier = light.m_innerScaleMultiplier;
	this->intensity = light.m_intensity;
	this->noiseAmplitude = light.m_noiseAmplitude;
	this->noiseFrequency = light.m_noiseFrequency;
	this->noiseOctaves = light.m_noiseOctaves;
	this->translation = light.m_translation;
	this->outerScaleMultiplier = light.m_outerScaleMultiplier;
	this->saturation = light.m_saturation;
	this->lightProfilePath = light.m_lightProfilePath;
	this->rotation = light.m_rotation;
}

LightData EveSOFDataMgr::PointLightAttachment::AsLightData( Color& color, float scale ) const {
	LightData data;
	data.color = color;
	data.position = this->translation;
	data.rotation = this->rotation;
	data.radius = this->outerScaleMultiplier * scale;
	data.innerRadius = this->innerScaleMultiplier * scale;

	data.brightness = this->intensity;
	data.noiseAmplitude = this->noiseAmplitude;
	data.noiseFrequency = this->noiseFrequency;
	data.noiseOctaves = this->noiseOctaves;
	return data;
}

EveSOFDataMgr::SpotLightAttachment::SpotLightAttachment( const EveSOFDataSpotLightAttachment& light ) {
	this->innerAngleMultiplier = light.m_innerAngleMultiplier;
	this->translation = light.m_translation;
	this->intensity = light.m_intensity;
	this->noiseAmplitude = light.m_noiseAmplitude;
	this->noiseFrequency = light.m_noiseFrequency;
	this->noiseOctaves = light.m_noiseOctaves;
	this->outerAngleMultiplier = light.m_outerAngleMultiplier;
	this->saturation = light.m_saturation;
	this->innerScaleMultiplier = light.m_innerScaleMultiplier;
	this->outerScaleMultiplier = light.m_outerScaleMultiplier;
	this->lightProfilePath = light.m_lightProfilePath;
}

LightData EveSOFDataMgr::SpotLightAttachment::AsLightData( Color& color, float scale, float innerAngle, float outerAngle ) const {
	LightData data;
	data.color = color;
	data.position = this->translation;
	data.innerAngle = this->innerAngleMultiplier * innerAngle;
	data.outerAngle = this->outerAngleMultiplier * outerAngle;
	data.innerRadius = this->innerScaleMultiplier * scale;
	data.radius = this->outerScaleMultiplier * scale;

	data.brightness = this->intensity;
	data.noiseAmplitude = this->noiseAmplitude;
	data.noiseFrequency = this->noiseFrequency;
	data.noiseOctaves = this->noiseOctaves;
	data.texturePath = this->lightProfilePath;

	return data;
}


// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveSOFDataMgr::EveSOFDataMgr( IRoot* lockobj ) :
	m_genericData()
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
//   check if pattern data is there. Mainly for debug reason!
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::HasPatternData( const char* patternName ) const
{
	std::map<std::string, PatternData>::const_iterator finder = m_patternData.find( patternName );
	return finder != m_patternData.end();
}

// --------------------------------------------------------------------------------
// Description:
//   Access to patterndata, only const pointer!!
// --------------------------------------------------------------------------------
const EveSOFDataMgr::PatternData* EveSOFDataMgr::GetPatternData( const char* patternName ) const
{
	std::map<std::string, PatternData>::const_iterator finder = m_patternData.find( patternName );
	if( finder == m_patternData.end() )
	{
		return nullptr;
	}
	return &finder->second;
}


// --------------------------------------------------------------------------------
// Description:
//   check if layoutdata is there.
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::HasLayoutData( const char* layoutName ) const
{
	return GetLayoutData( layoutName ) != nullptr;
}

// --------------------------------------------------------------------------------
// Description:
//   Access to layoutdata, only const pointer!!
// --------------------------------------------------------------------------------
const EveSOFDataMgr::LayoutData* EveSOFDataMgr::GetLayoutData( const char* layoutName ) const
{
	std::map<std::string, LayoutData>::const_iterator finder = m_layoutData.find( layoutName );
	if( finder == m_layoutData.end() )
	{
		return nullptr;
	}
	return &finder->second;
}

// --------------------------------------------------------------------------------
// Description:
//   Returns all the layout data that is specified in the vector
// --------------------------------------------------------------------------------
const std::vector<const EveSOFDataMgr::LayoutData*> EveSOFDataMgr::GetLayoutData( std::vector<std::string>& names ) const
{
	std::vector<const EveSOFDataMgr::LayoutData*> layouts = std::vector<const EveSOFDataMgr::LayoutData*>();

	for( size_t i = 0; i < names.size(); i++ )
	{
		auto data = GetLayoutData( names[i].c_str() );
		if( data != nullptr )
		{
			layouts.push_back( data );
		}
	}

	return layouts;
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
	if( !HasHullData( hullName ) )
	{
		CCP_LOGWARN( "Hull '%s' does not exist, by doing this you will not add it to the sof data just to this instance of EveSOFDataMgr", hullName );
	}

	// fill the non-trinity struct with the provided data
	HullData hd;
	GenerateHullData( hd, hullData );

	// set it to the main map
	m_hullData[hullName] = std::move( hd );

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Update an individual faction, identified by it's name
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::UpdateFaction( const char* factionName, EveSOFDataFaction* factionData )
{
	if( !HasFactionData( factionName ) )
	{
		CCP_LOGWARN( "Faction '%s' does not exist, by doing this you will not add it to the sof data just to this instance of EveSOFDataMgr", factionName );
	}

	// fill the non-trinity struct with the provided data
	FactionData fd;
	GenerateFactionData( fd, factionData );

	// set it to the main map
	m_factionData[factionName] = fd;

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Update an individual race, identified by it's name
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::UpdateRace( const char* raceName, EveSOFDataRace* raceData )
{
	if( !HasRaceData( raceName ) )
	{
		CCP_LOGWARN( "Race '%s' does not exist, by doing this you will not add it to the sof data just to this instance of EveSOFDataMgr", raceName );
	}

	// fill the non-trinity struct with the provided data
	RaceData rd;
	GenerateRaceData( rd, raceData );

	// set it to the main map
	m_raceData[raceName] = rd;

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Update an individual material, identified by it's name
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::UpdateMaterial( const char* materialName, EveSOFDataMaterial* materialData )
{
	if( !HasMaterialData( materialName ) )
	{
		CCP_LOGWARN( "Material '%s' does not exist, by doing this you will not add it to the sof data just to this instance of EveSOFDataMgr", materialName );
	}

	// fill the non-trinity struct with the provided data
	MaterialData md;
	GenerateMaterialData( md, materialData );

	// set it to the main map
	m_materialData[materialName] = md;

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Update an individual pattern, identified by it's name
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::UpdatePattern( const char* patternName, EveSOFDataPattern* patternData )
{
	if( !patternData )
	{
		return false;
	}

	if( !HasPatternData( patternName ) )
	{
		CCP_LOGWARN( "Pattern '%s' does not exist, by doing this you will not add it to the sof data just to this instance of EveSOFDataMgr", patternName );
	}

	// fill the non-trinity struct with the provided data
	PatternData pd;
	GeneratePatternData( pd, patternData );

	// set it to the main map
	m_patternData[patternName] = pd;

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Update an individual layout, identified by it's name
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::UpdateLayout( const char* layoutName, EveSOFDataLayout* layoutData )
{
	if( !layoutData )
	{
		return false;
	}

	if( !HasLayoutData( layoutName ) )
	{
		CCP_LOGWARN( "Layout '%s' does not exist, by doing this you will not add it to the sof data just to this instance of EveSOFDataMgr", layoutName );
	}

	// fill the non-trinity struct with the provided data
	LayoutData ld;
	GenerateLayoutData( ld, layoutData );

	// set it to the main map
	m_layoutData[layoutName] = ld;

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Update the generic data
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::UpdateGeneric( EveSOFDataGeneric* genericData )
{
	// fill the non-trinity struct with the provided data
	GenerateGenericData( m_genericData, genericData );

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Here we get a blue object which is the whole database and use it to
//   set internal containers with all SOF db data
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::SetData( EveSOFData* dbData )
{
	if( !dbData )
	{
		CCP_LOGERR( "No data passed to EveSOFDataMgr::SetData" );
		return false;
	}

	// clear existing data
	m_hullData.clear();
	m_factionData.clear();
	m_raceData.clear();
	m_materialData.clear();

	// load hull data
	if( !LoadHullData( dbData ) )
	{
		CCP_LOGERR( "Error loading hull data!" );
		return false;
	}
	CCP_LOGNOTICE( "SOF: loaded %d hulls", m_hullData.size() );

	// load faction data
	if( !LoadFactionData( dbData ) )
	{
		CCP_LOGERR( "Error loading faction data!" );
		return false;
	}
	CCP_LOGNOTICE( "SOF: loaded %d factions", m_factionData.size() );

	// load race data
	if( !LoadRaceData( dbData ) )
	{
		CCP_LOGERR( "Error loading race data!" );
		return false;
	}
	CCP_LOGNOTICE( "SOF: loaded %d races", m_raceData.size() );

	// load material data
	if( !LoadMaterialData( dbData ) )
	{
		CCP_LOGERR( "Error loading material data!" );
		return false;
	}
	CCP_LOGNOTICE( "SOF: loaded %d materials", m_materialData.size() );

	// load pattern data
	if( !LoadPatternData( dbData ) )
	{
		CCP_LOGERR( "Error loading pattern data!" );
		return false;
	}
	CCP_LOGNOTICE( "SOF: loaded %d patterns", m_patternData.size() );

	// load layoutdata
	if( !LoadLayoutData( dbData ) )
	{
		CCP_LOGERR( "Error loading layout data!" );
		return false;
	}
	CCP_LOGNOTICE( "SOF: loaded %d layouts", m_layoutData.size() );

	// load generic data
	if( !LoadGenericData( dbData ) )
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
	auto dbData = BeResMan->LoadObject<EveSOFData>( filePath );
	if( !dbData )
	{
		CCP_LOGERR( "Couldn't find SOF db data resource file %s or it is not of correct type!", filePath );
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
	ha.blockedMaterials = areaData->m_blockedMaterials;
	ha.shader = areaData->m_shader;
	ha.areaType = areaData->m_areaType;
	for( auto matit = areaData->m_textures.begin(); matit != areaData->m_textures.end(); ++matit )
	{
		EveSOFDataTexturePtr textureData = ( *matit );

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
		EveSOFDataHullPtr hullData = ( *it );

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
		m_hullData[( *it )->m_name] = std::move( hd );
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
	hd.buildClass = srcData->m_buildClass;
	hd.geometryResFilePath = srcData->m_geometryResFilePath;
	hd.boundingSphere = CcpMath::Sphere( srcData->m_boundingSphere );
	hd.shapeEllipsoid = CcpMath::AxisAlignedEllipsoid( srcData->m_shapeEllipsoidCenter, srcData->m_shapeEllipsoidRadius );
	hd.isSkinned = srcData->m_isSkinned;
	hd.enableDynamicBoundingSphere = srcData->m_enableDynamicBoundingSphere;
	hd.castShadow = srcData->m_castShadow;
	hd.audioPosition = srcData->m_audioPosition;
	hd.impactEffectType = srcData->m_impactEffectType;
	hd.sof6 = srcData->m_sof6;

	hd.category = std::string( srcData->m_category.c_str() );

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
			EveSOFDataHullBoosterItemPtr boosterItemData = ( *biit );

			HullBoosterItemData hbid;
			hbid.transform = boosterItemData->m_transform;
			hbid.functionality = boosterItemData->m_functionality;
			hbid.hasTrail = boosterItemData->m_hasTrail;
			hbid.atlasIndex0 = boosterItemData->m_atlasIndex0;
			hbid.atlasIndex1 = boosterItemData->m_atlasIndex1;
			hbid.lightScale = boosterItemData->m_lightScale;

			hbd.items.push_back( hbid );
		}

		hd.boosters = hbd;
	}

	// spritesets
	hd.spriteSets.clear();
	for( auto ssit = srcData->m_spriteSets.begin(); ssit != srcData->m_spriteSets.end(); ++ssit )
	{
		EveSOFDataHullSpriteSetPtr spriteSetData = ( *ssit );

		HullSpriteSetData hssd;
		hssd.skinned = spriteSetData->m_skinned;
		hssd.visibilityGroup = GetVisibilityGroupHash( spriteSetData->m_visibilityGroup );
		for( auto ssiit = spriteSetData->m_items.begin(); ssiit != spriteSetData->m_items.end(); ++ssiit )
		{
			EveSOFDataHullSpriteSetItemPtr spriteSetItemData = ( *ssiit );

			HullSpriteSetItemData hssid;
			hssid.blinkPhase = spriteSetItemData->m_blinkPhase;
			hssid.blinkRate = spriteSetItemData->m_blinkRate;
			hssid.boneIndex = spriteSetItemData->m_boneIndex;
			hssid.falloff = spriteSetItemData->m_falloff;
			hssid.maxScale = spriteSetItemData->m_maxScale;
			hssid.minScale = spriteSetItemData->m_minScale;
			hssid.position = spriteSetItemData->m_position;
			hssid.intensity = spriteSetItemData->m_intensity;
			hssid.saturation = spriteSetItemData->m_saturation;
			hssid.colorType = spriteSetItemData->m_colorType;
			hssid.light = nullptr;
			if( spriteSetItemData->m_light ) 
			{
				hssid.light = std::make_unique<PointLightAttachment>( PointLightAttachment( *spriteSetItemData->m_light ));
			}
			hssd.items.push_back( std::move( hssid ) );
		}
		hd.spriteSets.push_back( std::move( hssd ) );
	}

	// spotlightsets
	hd.spotlightSets.clear();
	hd.spotlightSets.reserve( srcData->m_spotlightSets.size() );
	for( auto& ssit : srcData->m_spotlightSets )
	{
		hd.spotlightSets.push_back( HullSpotlightSetData( *ssit ) );
	}

	// planesets
	hd.planeSets.clear();
	hd.planeSets.reserve( srcData->m_planeSets.size() );
	for( auto& psit : srcData->m_planeSets )
	{
		hd.planeSets.push_back( HullPlaneSetData( *psit ) );
	}

	// spritelinesets
	hd.spriteLineSets.clear();
	for( auto slsit = srcData->m_spriteLineSets.begin(); slsit != srcData->m_spriteLineSets.end(); ++slsit )
	{
		EveSOFDataHullSpriteLineSetPtr spriteLineSetData = ( *slsit );

		HullSpriteLineSetData hslsd;
		hslsd.skinned = spriteLineSetData->m_skinned;
		hslsd.visibilityGroup = GetVisibilityGroupHash( spriteLineSetData->m_visibilityGroup );

		for( auto slsiit = spriteLineSetData->m_items.begin(); slsiit != spriteLineSetData->m_items.end(); ++slsiit )
		{
			EveSOFDataHullSpriteLineSetItemPtr spriteLineSetItemData = ( *slsiit );

			HullSpriteLineSetItemData hslsid;
			hslsid.blinkPhaseShift = spriteLineSetItemData->m_blinkPhaseShift;
			hslsid.blinkPhase = spriteLineSetItemData->m_blinkPhase;
			hslsid.blinkRate = spriteLineSetItemData->m_blinkRate;
			hslsid.boneIndex = spriteLineSetItemData->m_boneIndex;
			hslsid.falloff = spriteLineSetItemData->m_falloff;
			hslsid.maxScale = spriteLineSetItemData->m_maxScale;
			hslsid.minScale = spriteLineSetItemData->m_minScale;
			hslsid.position = spriteLineSetItemData->m_position;
			hslsid.scaling = spriteLineSetItemData->m_scaling;
			hslsid.rotation = spriteLineSetItemData->m_rotation;
			hslsid.spacing = spriteLineSetItemData->m_spacing;
			hslsid.isCircle = spriteLineSetItemData->m_isCircle;
			hslsid.intensity = spriteLineSetItemData->m_intensity;
			hslsid.saturation = spriteLineSetItemData->m_saturation;
			hslsid.colorType = spriteLineSetItemData->m_colorType;
			if( spriteLineSetItemData->m_light )
			{
				hslsid.light = std::make_unique<PointLightAttachment>( *spriteLineSetItemData->m_light );
			}
			hslsd.items.push_back( std::move( hslsid ) );
		}
		hd.spriteLineSets.push_back( std::move( hslsd ) );
	}

	// hazesets
	hd.hazeSets.clear();
	for( auto hsit = srcData->m_hazeSets.begin(); hsit != srcData->m_hazeSets.end(); ++hsit )
	{
		EveSOFDataHullHazeSetPtr hazeSetData = ( *hsit );

		HullHazeSetData hhsd;
		std::string visGroupName( hazeSetData->m_visibilityGroup.c_str() );
		hhsd.visibilityGroup = CcpHashFNV1( visGroupName.c_str(), visGroupName.size() );
		hhsd.hazeType = hazeSetData->m_hazeType;
		hhsd.skinned = hazeSetData->m_skinned;

		for( auto hsiit = hazeSetData->m_items.begin(); hsiit != hazeSetData->m_items.end(); ++hsiit )
		{
			EveSOFDataHullHazeSetItemPtr hazeSetItemData = ( *hsiit );

			HullHazeSetItemData hhsid;
			hhsid.position = hazeSetItemData->m_position;
			hhsid.scaling = hazeSetItemData->m_scaling;
			hhsid.rotation = hazeSetItemData->m_rotation;
			hhsid.boneIndex = hazeSetItemData->m_boneIndex;
			hhsid.colorType = hazeSetItemData->m_colorType;
			hhsid.hazeBrightness = hazeSetItemData->m_hazeBrightness;
			hhsid.hazeFalloff = hazeSetItemData->m_hazeFalloff;
			hhsid.sourceBrightness = hazeSetItemData->m_sourceBrightness;
			hhsid.sourceSize = hazeSetItemData->m_sourceSize;
			hhsid.boosterGainInfluence = hazeSetItemData->m_boosterGainInfluence;
			hhsid.saturation = hazeSetItemData->m_saturation;
			hhsid.lights = std::vector<PointLightAttachment>();
			hhsid.lights.reserve( hazeSetItemData->m_lights.size() );

			for( auto& light:  hazeSetItemData->m_lights ) 
			{
				hhsid.lights.push_back( PointLightAttachment( *light ) );
			}
			hhsd.items.push_back( std::move( hhsid ) );
		}

		hd.hazeSets.push_back( std::move( hhsd ) );
	}

	hd.banners.clear();
	std::map<EveSOFDataHullBanner::Usage, size_t> setIndices;
	int32_t bannerIndex = 0;
	for( auto hsit = srcData->m_banners.begin(); hsit != srcData->m_banners.end(); ++hsit )
	{
		auto sourceBanner = *hsit;

		HullBannerData* set = nullptr;
		auto found = setIndices.find( sourceBanner->m_usage );
		if( found == end( setIndices ) )
		{
			setIndices[sourceBanner->m_usage] = hd.banners.size();
			hd.banners.push_back( HullBannerData() );
			set = &hd.banners.back();
			set->usage = sourceBanner->m_usage;
		}
		else
		{
			set = &hd.banners[found->second];
		}

		std::string visGroupName( sourceBanner->m_visibilityGroup.c_str() );

		HullBannerItemData hbsid;
		hbsid.item.position = sourceBanner->m_position;
		hbsid.item.scaling = sourceBanner->m_scaling;
		hbsid.item.rotation = sourceBanner->m_rotation;
		hbsid.item.angleX = sourceBanner->m_angleX;
		hbsid.item.angleY = sourceBanner->m_angleY;
		hbsid.item.bone = sourceBanner->m_boneIndex;
		hbsid.item.reference = bannerIndex++;
		hbsid.visibilityGroup = CcpHashFNV1( visGroupName.c_str(), visGroupName.size() );
		// lights
		hbsid.bannerLight.radiusMultiplier = sourceBanner->m_lightOverride->m_radiusMultiplier;
		hbsid.bannerLight.brightness = sourceBanner->m_lightOverride->m_brightness;
		hbsid.bannerLight.innerRadiusMultiplier = sourceBanner->m_lightOverride->m_innerRadiusMultiplier;
		hbsid.bannerLight.noiseAmplitude = sourceBanner->m_lightOverride->m_noiseAmplitude;
		hbsid.bannerLight.noiseFrequency = sourceBanner->m_lightOverride->m_noiseFrequency;
		hbsid.bannerLight.noiseOctaves = sourceBanner->m_lightOverride->m_noiseOctaves;
		hbsid.bannerLight.saturation = sourceBanner->m_lightOverride->m_saturation;

		set->items.push_back( hbsid );
	}

	hd.bannerSets.clear();
	std::map<uint32_t, size_t> bannerSetIndices;
	bannerIndex = 0;
	for( auto hsit = srcData->m_bannerSets.begin(); hsit != srcData->m_bannerSets.end(); ++hsit )
	{
		auto sourceBannerSet = *hsit;

		HullBannerSetData set = HullBannerSetData();

		std::string visGroupName( sourceBannerSet->m_visibilityGroup.c_str() );
		set.visibilityGroup = CcpHashFNV1( visGroupName.c_str(), visGroupName.size() );
		set.bannerTypes = std::map<EveSOFDataHullBannerSetItem::Usage, std::vector<HullBannerSetItemData>>();

		for( auto hsiit = begin( sourceBannerSet->m_banners ); hsiit != end( sourceBannerSet->m_banners ); ++hsiit )
		{
			auto bannerItem = *hsiit;

			HullBannerSetItemData data;
			data.item.position = bannerItem->m_position;
			data.item.scaling = bannerItem->m_scaling;
			data.item.rotation = bannerItem->m_rotation;
			data.item.angleX = bannerItem->m_angleX;
			data.item.angleY = bannerItem->m_angleY;
			data.item.bone = bannerItem->m_boneIndex;
			data.item.reference = bannerIndex++;
			data.light = nullptr;
			if( bannerItem->m_light ) 
			{
				data.light = std::make_shared<PointLightAttachment>( *bannerItem->m_light );
			}

			set.bannerTypes[bannerItem->m_usage].push_back( std::move( data) );
		}
		hd.bannerSets.push_back( std::move( set ) );
	}

	// default hull pattern
	EveSOFDataPatternPerHullPtr defaultPattern = srcData->m_defaultPattern;
	// only one layer for the default hull one (yet...)
	EveSOFUtils::GeneratePatternProjectionData( &hd.defaultPattern, defaultPattern ? defaultPattern->m_transformLayer1 : nullptr );

	// hull decal sets
	hd.hullDecalSets.clear();
	for( auto hds = srcData->m_decalSets.begin(); hds != srcData->m_decalSets.end(); ++hds )
	{
		EveSOFDataHullDecalSetPtr hullDecalSet = ( *hds );
		HullDecalSetData decalSetData;
		decalSetData.visibilityGroup = GetVisibilityGroupHash( hullDecalSet->m_visibilityGroup );
		decalSetData.items.clear();

		for( const auto& itemPtr : hullDecalSet->m_items )
		{
			HullDecalSetItemData itemData;
			itemData.boneIndex = itemPtr->m_boneIndex;
			itemData.glowColorType = itemPtr->m_glowColorType;
			itemData.meshIndex = itemPtr->m_meshIndex;
			itemData.position = itemPtr->m_position;
			itemData.rotation = itemPtr->m_rotation;
			itemData.scaling = itemPtr->m_scaling;
			itemData.usage = itemPtr->m_usage;
			itemData.logoType = itemPtr->m_logoType;

			for( const auto buffer : itemPtr->m_indexBuffers )
			{
				itemData.indexBuffers.push_back( buffer->m_indexBuffer );
			}

			for( auto hdtit = itemPtr->m_textures.begin(); hdtit != itemPtr->m_textures.end(); ++hdtit )
			{
				EveSOFDataTexturePtr textureData = ( *hdtit );

				TextureData td;
				td.resFilePath = textureData->m_resFilePath;
				itemData.textures[textureData->m_name] = td;
			}
			for( auto hdpit = itemPtr->m_parameters.begin(); hdpit != itemPtr->m_parameters.end(); ++hdpit )
			{
				EveSOFDataParameterPtr parameterData = ( *hdpit );

				itemData.parameters[parameterData->m_name] = parameterData->m_value;
			}
			decalSetData.items.push_back( itemData );
		}
		hd.hullDecalSets.push_back( decalSetData );
	}

	// hull light sets
	hd.hullLightSets.clear();
	for( auto ls = srcData->m_lightSets.begin(); ls != srcData->m_lightSets.end(); ++ls )
	{
		EveSOFDataHullLightSetPtr lightSet = ( *ls );
		HullLightSetData lightSetData;
		lightSetData.visibilityGroup = GetVisibilityGroupHash( lightSet->m_visibilityGroup );
		lightSetData.items.clear();

		for( auto lsi = lightSet->m_items.begin(); lsi != lightSet->m_items.end(); ++lsi )
		{
			lightSetData.items.push_back( ( *lsi )->m_data );
		}
		hd.hullLightSets.push_back( lightSetData );
	}

	// meshareas
	hd.opaqueAreas.clear();
	for( auto mait = srcData->m_opaqueAreas.begin(); mait != srcData->m_opaqueAreas.end(); ++mait )
	{
		EveSOFDataHullAreaPtr areaData = ( *mait );
		hd.opaqueAreas.push_back( LoadHullAreaData( areaData ) );
	}
	hd.decalAreas.clear();
	for( auto mait = srcData->m_decalAreas.begin(); mait != srcData->m_decalAreas.end(); ++mait )
	{
		EveSOFDataHullAreaPtr areaData = ( *mait );
		hd.decalAreas.push_back( LoadHullAreaData( areaData ) );
	}
	hd.transparentAreas.clear();
	for( auto mait = srcData->m_transparentAreas.begin(); mait != srcData->m_transparentAreas.end(); ++mait )
	{
		EveSOFDataHullAreaPtr areaData = ( *mait );
		hd.transparentAreas.push_back( LoadHullAreaData( areaData ) );
	}
	hd.additiveAreas.clear();
	for( auto mait = srcData->m_additiveAreas.begin(); mait != srcData->m_additiveAreas.end(); ++mait )
	{
		EveSOFDataHullAreaPtr areaData = ( *mait );
		hd.additiveAreas.push_back( LoadHullAreaData( areaData ) );
	}
	hd.distortionAreas.clear();
	for( auto mait = srcData->m_distortionAreas.begin(); mait != srcData->m_distortionAreas.end(); ++mait )
	{
		EveSOFDataHullAreaPtr areaData = ( *mait );
		hd.distortionAreas.push_back( LoadHullAreaData( areaData ) );
	}

	// build a lookup for textures via meshIndices
	hd.meshIndexToOpaqueAreaLookup.clear();
	for( size_t i = 0; i < hd.opaqueAreas.size(); ++i )
	{
		hd.meshIndexToOpaqueAreaLookup[hd.opaqueAreas[i].index] = i;
	}

	// turret locators
	hd.locatorTurrets.clear();
	for( auto tlit = srcData->m_locatorTurrets.begin(); tlit != srcData->m_locatorTurrets.end(); ++tlit )
	{
		EveSOFDataHullLocatorPtr locatorData = ( *tlit );

		LocatorData ld;
		ld.name = locatorData->m_name;
		ld.transform = locatorData->m_transform;
		hd.locatorTurrets.push_back( ld );
	}

	// locator sets
	unsigned uniqueID = 0;
	hd.locatorSets.clear();
	for( auto lsit = srcData->m_locatorSets.begin(); lsit != srcData->m_locatorSets.end(); ++lsit )
	{
		IEveSOFDataHullLocatorSetPtr locatorSetOrGroup = ( *lsit );
		LoadLocatorData( hd, srcData, locatorSetOrGroup, uniqueID );
	}

	// children
	hd.children.clear();
	for( auto chit = srcData->m_children.begin(); chit != srcData->m_children.end(); ++chit )
	{
		EveSOFDataHullChildPtr child = ( *chit );
		HullChild hc;
		hc.redFilePath = child->m_redFilePath;
		hc.lowestLodVisible = child->m_lowestLodVisible;
		hc.translation = child->m_translation;
		hc.rotation = child->m_rotation;
		hc.scaling = child->m_scaling;
		hc.id = child->m_id;
		hc.groupIndex = child->m_groupIndex;
		hc.buildFilter = child->m_buildFilter;

		hd.children.push_back( hc );
	}

	hd.childSets.clear();
	for( auto csit = srcData->m_childSets.begin(); csit != srcData->m_childSets.end(); ++csit )
	{
		EveSOFDataHullChildSetPtr childSet = ( *csit );
		HullChildSetData hcsd;
		;
		hcsd.visibilityGroup = GetVisibilityGroupHash( childSet->m_visibilityGroup );
		for( auto cit = childSet->m_items.begin(); cit != childSet->m_items.end(); ++cit )
		{
			EveSOFDataHullChildSetItemPtr child = ( *cit );
			HullChildSetItemData hcd;
			hcd.redFilePath = child->m_redFilePath;
			hcd.lowestLodVisible = child->m_lowestLodVisible;
			hcd.translation = child->m_translation;
			hcd.rotation = child->m_rotation;
			hcd.scaling = child->m_scaling;
			hcd.buildFilter = child->m_buildFilter;
			hcsd.items.push_back( hcd );
		}

		hd.childSets.push_back( hcsd );
	}

	// instanced meshes
	hd.instancedMeshes.clear();
	for( auto imit = srcData->m_instancedMeshes.begin(); imit != srcData->m_instancedMeshes.end(); ++imit )
	{
		EveSOFDataInstancedMeshPtr instMesh = ( *imit );
		HullInstancedMesh him;
		him.name = instMesh->m_name;
		him.lowestLodVisible = instMesh->m_lowestLodVisible;
		him.geometryResPath = instMesh->m_geometryResPath;
		him.instances.reserve( instMesh->m_instances.size() );
		CcpMath::AxisAlignedBox boundingBox = CcpMath::AxisAlignedBox();
		for( auto iit = instMesh->m_instances.begin(); iit != instMesh->m_instances.end(); ++iit )
		{
			HullMeshInstance instance;
			Matrix transform( XMMatrixTranspose( XMMatrixTransformation( Vector3( 0, 0, 0 ), Quaternion( 0, 0, 0, 1 ), iit->scaling, Vector3( 0, 0, 0 ), iit->rotation, iit->translation ) ) );
			instance.transform0 = *reinterpret_cast<Vector4*>( &transform.GetX() );
			instance.transform1 = *reinterpret_cast<Vector4*>( &transform.GetY() );
			instance.transform2 = *reinterpret_cast<Vector4*>( &transform.GetZ() );
			instance.lastTransform0 = *reinterpret_cast<Vector4*>( &transform.GetX() );
			instance.lastTransform1 = *reinterpret_cast<Vector4*>( &transform.GetY() );
			instance.lastTransform2 = *reinterpret_cast<Vector4*>( &transform.GetZ() );
			instance.boneIndex = iit->boneIndex;
			him.instances.push_back( instance );

			CcpMath::AxisAlignedBox instanceBox( -iit->scaling, iit->scaling );
			instanceBox.Transform( transform );
			boundingBox.Include( instanceBox );
		}
		him.shader = instMesh->m_shader;
		for( auto tit = instMesh->m_textures.begin(); tit != instMesh->m_textures.end(); ++tit )
		{
			EveSOFDataTexturePtr textureData = ( *tit );
			TextureData td;
			td.resFilePath = textureData->m_resFilePath;
			him.textures[textureData->m_name] = td;
		}
		him.bounds = boundingBox;
		him.displayModifier = instMesh->m_displayModifier;
		hd.instancedMeshes.push_back( him );
	}

	// animations
	hd.animations.clear();
	for( auto chit = srcData->m_animations.begin(); chit != srcData->m_animations.end(); ++chit )
	{
		EveSOFDataHullAnimationPtr anim = ( *chit );
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

	hd.soundEmitters.clear();
	for( auto cit = begin( srcData->m_soundEmitters ); cit != end( srcData->m_soundEmitters ); ++cit )
	{
		HullSoundEmitter emitter;
		emitter.attenuationScalingFactor = ( *cit )->m_attenuationScalingFactor;
		emitter.name = ( *cit )->m_name;
		emitter.prefix = ( *cit )->m_prefix;
		emitter.position = ( *cit )->m_position;
		emitter.rotation = ( *cit )->m_rotation;
		hd.soundEmitters.push_back( emitter );
	}

	hd.controllers.clear();
	for( auto cit = begin( srcData->m_controllers ); cit != end( srcData->m_controllers ); ++cit )
	{
		hd.controllers.push_back( { BlueSharedString( ( *cit )->m_path ), ( *cit )->m_buildFilter } );
	}

	// model curves
	hd.modelRotationCurvePath = srcData->m_modelRotationCurvePath;
	hd.modelTranslationCurvePath = srcData->m_modelTranslationCurvePath;
}


void EveSOFDataMgr::LoadLocatorData( HullData& hd, EveSOFDataHullPtr srcData, IEveSOFDataHullLocatorSetPtr locatorSetOrGroup, uint32_t& uniqueID ) const
{
	if( EveSOFDataHullLocatorSetPtr locatorSet = BlueCastPtr( locatorSetOrGroup ) )
	{
		for( auto lit = locatorSet->m_locators.begin(); lit != locatorSet->m_locators.end(); ++lit )
		{
			EveSOFDataTransformPtr locatorData = ( *lit );

			LocatorDirectionData ldd;
			ldd.position = locatorData->m_position;
			ldd.rotation = locatorData->m_rotation;
			ldd.scaling = locatorData->m_scaling;
			ldd.boneIndex = locatorData->m_boneIndex;
			ldd.uniqueID = uniqueID++;
			hd.locatorSets[locatorSet->m_name].push_back( ldd );
		}
	}
	else if( EveSOFDataHullLocatorSetGroupPtr group = BlueCastPtr( locatorSetOrGroup ) )
	{
		for( auto setOrGroup = group->m_locatorSets.begin(); setOrGroup != group->m_locatorSets.end(); ++setOrGroup )
		{
			LoadLocatorData( hd, srcData, *setOrGroup, uniqueID );
		}
	}
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
		EveSOFDataFactionPtr factionData = ( *it );

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
		m_factionData[( *it )->m_name] = fd;
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
	fd.materialUsageList[0] = srcData->m_materialUsageMtl1;
	fd.materialUsageList[1] = srcData->m_materialUsageMtl2;
	fd.materialUsageList[2] = srcData->m_materialUsageMtl3;
	fd.materialUsageList[3] = srcData->m_materialUsageMtl4;

	// colors
	memset( &fd.colorData, 0, sizeof( ColorData ) );
	if( srcData->m_colorSet )
	{
		for( size_t i = 0; i < SOFDataFactionColorChooser::TYPE_MAX; ++i )
		{
			// spriteset colors
			fd.colorData.colors[i] = srcData->m_colorSet->m_colors[i];
		}
	}

	// colors
	if( srcData->m_logoSet )
	{
		fd.logoSetData = LogoSetData();
		for( size_t i = 0; i < EveSOFDataLogoSet::TYPE_MAX; ++i )
		{
			// logo information
			EveSOFDataLogoPtr srcLogo = srcData->m_logoSet->m_logos[i];

			if( srcLogo )
			{
				LogoData destLogo = LogoData();
				destLogo.textures.clear();

				for( auto srcTexture = srcLogo->m_textures.begin(); srcTexture != srcLogo->m_textures.end(); ++srcTexture )
				{
					auto tex = ( *srcTexture );
					TextureData td;
					td.resFilePath = tex->m_resFilePath;
					destLogo.textures[tex->m_name] = td;
				}
				fd.logoSetData.logos[i] = destLogo;
			}
		}
	}

	// vis groups
	fd.visibilityData.clear();
	if( srcData->m_visibilityGroupSet )
	{
		for( auto vgsit = srcData->m_visibilityGroupSet->m_visibilityGroups.begin(); vgsit != srcData->m_visibilityGroupSet->m_visibilityGroups.end(); ++vgsit )
		{
			// Should this also use the GetVisibilityGroupHash function?
			uint32_t h = CcpHashFNV1( ( *vgsit )->m_str.c_str(), ( *vgsit )->m_str.size() );
			fd.visibilityData.insert( h );
		}
	}

	// spotlight set colors
	fd.spotlightSetsColors.clear();
	for( auto spotcit = srcData->m_spotlightSets.begin(); spotcit != srcData->m_spotlightSets.end(); ++spotcit )
	{
		EveSOFDataFactionSpotlightSetPtr spotlightSetData = ( *spotcit );

		FactionSpotlightSetColorData spotcd;
		spotcd.coneColor = spotlightSetData->m_coneColor;
		spotcd.flareColor = spotlightSetData->m_flareColor;
		spotcd.spriteColor = spotlightSetData->m_spriteColor;

		fd.spotlightSetsColors[spotlightSetData->m_groupIndex] = spotcd;
	}

	// planeset set colors
	fd.planeSetsColors.clear();
	for( auto plcit = srcData->m_planeSets.begin(); plcit != srcData->m_planeSets.end(); ++plcit )
	{
		EveSOFDataFactionPlaneSetPtr planeSetData = ( *plcit );

		FactionPlaneSetColorData plscd;
		plscd.color = planeSetData->m_color;

		fd.planeSetsColors[planeSetData->m_groupIndex] = plscd;
	}

	// child faction data
	fd.childData.clear();
	for( auto ccit = srcData->m_children.begin(); ccit != srcData->m_children.end(); ++ccit )
	{
		auto childData = *ccit;

		FactionChildData fcd;
		fcd.isVisible = childData->m_isVisible;
		fd.childData[childData->m_groupIndex] = fcd;
	}

	// area parameters
	fd.areaMaterials.materialNames.clear();
	fd.areaMaterials.glowColor.clear();
	if( srcData->m_areaTypes )
	{
		for( uint32_t i = 0; i < EveSOFDataArea::TYPE_MAX; ++i )
		{
			// material lib names
			EveSOFDataAreaMaterial* areaMaterial = srcData->m_areaTypes->m_materials[i];
			if( areaMaterial )
			{
				for( uint32_t j = 0; j < EveSOFDataAreaMaterial::MATERIAL_MAX; ++j )
				{
					if( !areaMaterial->m_material[j].empty() )
					{
						fd.areaMaterials.materialNames[std::make_pair( i, j )] = BlueSharedString( areaMaterial->m_material[j] );
					}
				}

				// glow colortype
				fd.areaMaterials.glowColor[std::make_pair( i, "GeneralGlowColor" )] = areaMaterial->m_glowColorType;
			}
		}
	}

	// pattern data
	EveSOFUtils::GeneratePatternLayerData( &fd.defaultPatternInfo, srcData->m_defaultPattern, nullptr );
	fd.defaultPatternLayer1MaterialName = srcData->m_defaultPatternLayer1MaterialName;
	fd.defaultPatternLayer2MaterialName = srcData->m_defaultPatternLayer2MaterialName;
	fd.defaultPatternName = srcData->m_defaultPatternName;
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
		EveSOFDataRacePtr raceData = ( *it );

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
		m_raceData[( *it )->m_name] = rd;
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
	if( srcData->m_booster )
	{
		rd.boosters.scale = srcData->m_booster->m_scale;
		rd.boosters.glowScale = srcData->m_booster->m_glowScale;
		rd.boosters.glowColor = srcData->m_booster->m_glowColor;
		rd.boosters.haloColor = srcData->m_booster->m_haloColor;
		rd.boosters.haloScaleX = srcData->m_booster->m_haloScaleX;
		rd.boosters.haloScaleY = srcData->m_booster->m_haloScaleY;
		rd.boosters.symHaloScale = srcData->m_booster->m_symHaloScale;
		rd.boosters.trailColor = srcData->m_booster->m_trailColor;
		rd.boosters.warpGlowColor = srcData->m_booster->m_warpGlowColor;
		rd.boosters.warpHaloColor = srcData->m_booster->m_warpHaloColor;
		rd.boosters.trailSize = srcData->m_booster->m_trailSize;

		auto copyShape = [=]( RaceBoosterDataShape& dest, EveSOFDataBoosterShape* src ) {
			dest.noiseFunction = src->m_noiseFunction;
			dest.noiseSpeed = src->m_noiseSpeed;
			dest.noiseAmplitureStart = src->m_noiseAmplitureStart;
			dest.noiseAmplitureEnd = src->m_noiseAmplitureEnd;
			dest.noiseFrequency = src->m_noiseFrequency;
			dest.color = src->m_color;
		};

		copyShape( rd.boosters.shape0, srcData->m_booster->m_shape0 );
		copyShape( rd.boosters.shape1, srcData->m_booster->m_shape1 );
		copyShape( rd.boosters.warpShape0, srcData->m_booster->m_warpShape0 );
		copyShape( rd.boosters.warpShape1, srcData->m_booster->m_warpShape1 );

		rd.boosters.shapeAtlasResPath = srcData->m_booster->m_shapeAtlasResPath;
		rd.boosters.gradient0ResPath = srcData->m_booster->m_gradient0ResPath;
		rd.boosters.gradient1ResPath = srcData->m_booster->m_gradient1ResPath;
		rd.boosters.shapeAtlasHeight = srcData->m_booster->m_shapeAtlasHeight;
		rd.boosters.shapeAtlasCount = srcData->m_booster->m_shapeAtlasCount;
		rd.boosters.lightOffset = srcData->m_booster->m_lightOffset;
		rd.boosters.lightRadius = srcData->m_booster->m_lightRadius;
		rd.boosters.lightWarpRadius = srcData->m_booster->m_lightWarpRadius;
		rd.boosters.lightFlickerAmplitude = srcData->m_booster->m_lightFlickerAmplitude;
		rd.boosters.lightFlickerFrequency = srcData->m_booster->m_lightFlickerFrequency;
		rd.boosters.lightColor = srcData->m_booster->m_lightColor;
		rd.boosters.lightWarpColor = srcData->m_booster->m_lightWarpColor;
	}

	// shader data
	rd.areaMaterials.glowColor.clear();
	rd.areaMaterials.materialNames.clear();
	rd.areaMaterials.glowColor[std::make_pair( EveSOFDataArea::TYPE_PRIMARY, "GeneralHeatGlowColor" )] = srcData->m_hullPrimaryHeatColorType;
	rd.areaMaterials.glowColor[std::make_pair( EveSOFDataArea::TYPE_REACTOR, "GeneralHeatGlowColor" )] = srcData->m_hullReactorHeatColorType;

	// damage data
	rd.damage.armorDamageParameters.clear();
	rd.damage.armorDamageTextures.clear();
	rd.damage.shieldDamageParameters.clear();
	rd.damage.shieldDamageTextures.clear();
	if( srcData->m_damage )
	{
		for( auto adpit = srcData->m_damage->m_armorImpactParameters.begin(); adpit != srcData->m_damage->m_armorImpactParameters.end(); ++adpit )
		{
			EveSOFDataParameterPtr param = ( *adpit );

			rd.damage.armorDamageParameters[param->m_name] = param->m_value;
		}
		for( auto adtit = srcData->m_damage->m_armorImpactTextures.begin(); adtit != srcData->m_damage->m_armorImpactTextures.end(); ++adtit )
		{
			EveSOFDataTexturePtr tex = ( *adtit );

			TextureData td;
			td.resFilePath = tex->m_resFilePath;
			rd.damage.armorDamageTextures[tex->m_name] = td;
		}
		for( auto sdpit = srcData->m_damage->m_shieldImpactParameters.begin(); sdpit != srcData->m_damage->m_shieldImpactParameters.end(); ++sdpit )
		{
			EveSOFDataParameterPtr param = ( *sdpit );

			rd.damage.shieldDamageParameters[param->m_name] = param->m_value;
		}
		for( auto sdtit = srcData->m_damage->m_shieldImpactTextures.begin(); sdtit != srcData->m_damage->m_shieldImpactTextures.end(); ++sdtit )
		{
			EveSOFDataTexturePtr tex = ( *sdtit );

			TextureData td;
			td.resFilePath = tex->m_resFilePath;
			rd.damage.shieldDamageTextures[tex->m_name] = td;
		}
	}
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
		EveSOFDataMaterialPtr materialData = ( *it );

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
		m_materialData[( *it )->m_name] = md;
	}

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Fill a non-trinity pattern data struct with all the data from the trinity
//   data struct
// --------------------------------------------------------------------------------
void EveSOFDataMgr::GeneratePatternData( PatternData& pd, EveSOFDataPatternPtr srcData ) const
{
	pd.applicationData.clear();
	pd.sof6 = srcData->m_sof6;
	
	// per-layer data
	PatternLayerData pld1, pld2;

	bool hasLayer1 = srcData->m_layer1 != nullptr;
	bool hasLayer2 = srcData->m_layer2 != nullptr;

	// generate layer data, even if we don't have the layer (we still need the data that it isn't there)
	EveSOFUtils::GeneratePatternLayerData( &pld1, srcData->m_layer1, nullptr );
	EveSOFUtils::GeneratePatternLayerData( &pld2, srcData->m_layer2, nullptr );

	// pattern projections, TODO remove after PHASE-6
	for( auto ppit = srcData->m_projections.begin(); ppit != srcData->m_projections.end(); ++ppit )
	{
		EveSOFDataPatternPerHullPtr hullData = ( *ppit );

		auto hullApplicationData = PatternApplicationData();

		PatternProjectionData ppd1, ppd2;
		EveSOFUtils::GeneratePatternProjectionData( &ppd1, hullData->m_transformLayer1 );
		EveSOFUtils::GeneratePatternProjectionData( &ppd2, hullData->m_transformLayer2 );

		hullApplicationData.layerAndProjection.push_back( std::make_pair( pld1, ppd1 ) );
		hullApplicationData.layerAndProjection.push_back( std::make_pair( pld2, ppd2 ) );

		pd.old_applicationData[hullData->m_name] = hullApplicationData;
	}


	for( auto groupIt = srcData->m_applicationGroups.begin(); groupIt != srcData->m_applicationGroups.end(); ++groupIt )
	{
		auto group = *groupIt;

		// create layer1 and layer2
		PatternLayerData layer1, layer2;

		EveSOFUtils::GeneratePatternLayerData( &layer1, srcData->m_layer1, group->m_layer1Properties );
		EveSOFUtils::GeneratePatternLayerData( &layer2, srcData->m_layer2, group->m_layer2Properties );

		// go over all the hulls in the group and generate the pattern application list
		for( auto hullIt = group->m_projections.begin(); hullIt != group->m_projections.end(); ++hullIt )
		{
			auto applicationInfo = PatternApplicationData();
			auto hull = *hullIt;

			if( srcData->m_layer1 && group->m_layer1Properties && hull->m_transformLayer1 )
			{
				PatternProjectionData projectionData;
				EveSOFUtils::GeneratePatternProjectionData( &projectionData, hull->m_transformLayer1 );
				applicationInfo.layerAndProjection.push_back( std::make_pair( layer1, projectionData ) );
			}
			if( srcData->m_layer2 && group->m_layer2Properties && hull->m_transformLayer2 )
			{
				PatternProjectionData projectionData;
				EveSOFUtils::GeneratePatternProjectionData( &projectionData, hull->m_transformLayer2 );

				applicationInfo.layerAndProjection.push_back( std::make_pair( layer2, projectionData ) );
			}

			pd.applicationData[hull->m_name] = applicationInfo;
		}
	}
}

// helper function so that we can recursively unpack placement data
EveSOFDataMgr::ExtensionPlacementData EveSOFDataMgr::UnpackPlacementData( IEveSOFDataHullExtensionPlacementPtr placementOrGroup ) const
{
	auto placementData = ExtensionPlacementData();

	if (EveSOFDataHullExtensionPlacementPtr placement = BlueCastPtr( placementOrGroup ))
	{
		placementData.isAGroup = false;
		placementData.enabled = placement->m_enabled;

		if( placement->m_descriptor == nullptr )
		{
			CCP_LOGERR( "Layout placement (%s) has no dna descriptor", placement->m_name.c_str() );
		}
		if( placement->m_descriptor->m_hull.empty() )
		{
			CCP_LOGERR( "Layout placement (%s) has no hull", placement->m_name.c_str() );
		}

		auto descriptor = DNADescriptorData();

		descriptor.hull = BlueSharedString( placement->m_descriptor->m_hull );
		descriptor.faction = BlueSharedString( placement->m_descriptor->m_faction );
		descriptor.race = BlueSharedString( placement->m_descriptor->m_race );
		descriptor.pattern = BlueSharedString( placement->m_descriptor->m_pattern );
		descriptor.material1 = BlueSharedString( placement->m_descriptor->m_material1 );
		descriptor.material2 = BlueSharedString( placement->m_descriptor->m_material2 );
		descriptor.material3 = BlueSharedString( placement->m_descriptor->m_material3 );
		descriptor.material4 = BlueSharedString( placement->m_descriptor->m_material4 );
		descriptor.layout = BlueSharedString( placement->m_descriptor->m_layout );

		placementData.name = BlueSharedString( placement->m_name );
		placementData.offset = placement->m_offset;
		placementData.locatorSetName = BlueSharedString( placement->m_locatorSetName );
		placementData.descriptor = descriptor;
		placementData.isInstanced = placement->m_isInstanced;
		placementData.extendsBoundingSphere = placement->m_extendsBoundingSphere;
		placementData.extendsShieldEllipsoid = placement->m_extendsShieldEllipsoid;

		placementData.hasDistribution = placement->m_distribution != nullptr;
		if( placementData.hasDistribution )
		{
			placementData.distribution = generateDistributionData( placement->m_distribution );
		}

		for( auto placementCondition : placement->m_distributionConditions )
		{
			placementData.placementConditions.push_back( generateDistributionConditionData( placementCondition ) );
		}
	}
	if( EveSOFDataHullExtensionPlacementGroupPtr placement = BlueCastPtr( placementOrGroup ) )
	{
		placementData.isAGroup = true;
		
		placementData.name = BlueSharedString( placement->m_name );
		placementData.enabled = placement->m_enabled;

		for( auto counter : placement->m_depletionCounters )
		{
			auto counterStruct = ExtensionPlacementDepletionCounter();
			counterStruct.counterName = BlueSharedString( counter->m_name );
			counterStruct.counterValue = counter->m_value;
			placementData.depletionCounters.push_back( counterStruct );
		}

		for( auto placementOrGroup : placement->m_placements )
		{
			placementData.placements.push_back( UnpackPlacementData( placementOrGroup ) );
		}

		for( auto placementCondition : placement->m_distributionConditions )
		{
			placementData.placementConditions.push_back( generateDistributionConditionData( placementCondition ) );
		}
	}
	return placementData;
}

// --------------------------------------------------------------------------------
// Description:
//   Fill a non-trinity layout data struct with all the data from the trinity
//   data struct
// --------------------------------------------------------------------------------
void EveSOFDataMgr::GenerateLayoutData( LayoutData& ld, EveSOFDataLayoutPtr srcData ) const
{
	ld.name = BlueSharedString( srcData->m_name );
	ld.seed = srcData->m_seed;
	ld.scrambleSeed = srcData->m_randomizeSeedOnLoad;

	for( auto counter : srcData->m_depletionCounters )
	{
		auto counterStruct = ExtensionPlacementDepletionCounter();
		counterStruct.counterName = BlueSharedString( counter->m_name );
		counterStruct.counterValue = counter->m_value;
		ld.depletionCounters.push_back( counterStruct );
	}

	for( auto placement : srcData->m_placements )
	{
		auto placementData = UnpackPlacementData( placement );
		ld.placements.push_back( placementData );
	}
}

EveSOFDataMgr::ExtensionPlacementDistribution EveSOFDataMgr::generateDistributionData( EveSOFDataHullExtensionPlacementDistributionPlacementPtr distributionObj ) const
{
	auto placementDistribution = ExtensionPlacementDistribution();

	placementDistribution.completeness = distributionObj->m_completeness;
	placementDistribution.placementBias = distributionObj->m_placementBias;
	placementDistribution.centerBias = distributionObj->m_centerBias;
	placementDistribution.cap = distributionObj->m_cap;
	placementDistribution.randomRotationStepSizeYPR = distributionObj->m_randomRotationStepSizeYPR;
	placementDistribution.randomRotationMaxSteps = distributionObj->m_randomRotationMaxSteps;
	placementDistribution.randomScaleMin = distributionObj->m_randomScaleMin;
	placementDistribution.randomScaleMax = distributionObj->m_randomScaleMax; 
	placementDistribution.uniformScaling = distributionObj->m_uniformScale;
	placementDistribution.occupyLocators = distributionObj->m_occupyLocators;

	return placementDistribution;
}


EveSOFDataMgr::ExtensionPlacementDistributionCondition EveSOFDataMgr::generateDistributionConditionData( IEveSOFDataHullExtensionPlacementDistributionPtr distributionObj ) const
{
	auto placementCondition = ExtensionPlacementDistributionCondition();

	if( EveSOFDataHullExtensionPlacementDistributionParentMatchPtr parentMatch = BlueCastPtr( distributionObj ) )
	{
		if( parentMatch->m_parentDescriptor != nullptr )
		{
			placementCondition.distributionType = PARENT_MATCH;

			auto DNADiscriptor = DNADescriptorData();
			DNADiscriptor.hull = BlueSharedString( parentMatch->m_parentDescriptor->m_hull );
			DNADiscriptor.faction = BlueSharedString( parentMatch->m_parentDescriptor->m_faction );
			DNADiscriptor.race = BlueSharedString( parentMatch->m_parentDescriptor->m_race );
			DNADiscriptor.pattern = BlueSharedString( parentMatch->m_parentDescriptor->m_pattern );
			DNADiscriptor.material1 = BlueSharedString( parentMatch->m_parentDescriptor->m_material1 );
			DNADiscriptor.material2 = BlueSharedString( parentMatch->m_parentDescriptor->m_material2 );
			DNADiscriptor.material3 = BlueSharedString( parentMatch->m_parentDescriptor->m_material3 );
			DNADiscriptor.material4 = BlueSharedString( parentMatch->m_parentDescriptor->m_material4 );
			DNADiscriptor.layout = BlueSharedString( parentMatch->m_parentDescriptor->m_layout );

			placementCondition.spaceObjectParentDescriptor = DNADiscriptor;
		}
	}
	else if( EveSOFDataHullExtensionPlacementDistributionDepletionCounterPtr DepletionCounterDistribution = BlueCastPtr( distributionObj ) )
	{
		placementCondition.distributionType = DEPLETION_COUNTER;

		for( auto counter : DepletionCounterDistribution->m_depletionCounters )
		{
			auto counterStruct = ExtensionPlacementDepletionCounter();
			counterStruct.counterName = BlueSharedString( counter->m_name );
			counterStruct.counterValue = counter->m_value;
			placementCondition.depletionCounters.push_back( counterStruct );
		}
	}
	else if( EveSOFDataHullExtensionPlacementDistributionRandomChancePtr randomChanceCondition = BlueCastPtr( distributionObj ) )
	{
		placementCondition.distributionType = RANDOM_INCLUCION;
		placementCondition.triggerChance = randomChanceCondition->m_chanceOfUsage;
	}
	else if( EveSOFDataHullExtensionPlacementDistributionMapGraphicSettingsPtr displayFilterCondition = BlueCastPtr( distributionObj ) )
	{
		placementCondition.distributionType = GRAPHIC_SETTING_MAP;
		placementCondition.displayModifier = displayFilterCondition->m_displayFilter;
	}

	return placementCondition;
}

// --------------------------------------------------------------------------------
// Description:
//   Init pattern-specific data
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::LoadPatternData( EveSOFDataPtr srcData )
{
	// store that data from that object internally
	for( EveSOFDataPatternVector::const_iterator it = srcData->m_pattern.begin(); it != srcData->m_pattern.end(); ++it )
	{
		EveSOFDataPatternPtr patternData = ( *it );

		// if this pattern is already there, we have a problem!
		if( m_patternData.find( patternData->m_name ) != m_patternData.end() )
		{
			CCP_LOGERR( "Found a duplicate pattern name: %s", patternData->m_name.c_str() );
			return false;
		}

		// fill the non-trinity struct with the provided data
		PatternData pd;
		GeneratePatternData( pd, patternData );

		// put it into the main map
		m_patternData[( *it )->m_name] = pd;
	}

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Init pattern-specific data
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::LoadLayoutData( EveSOFDataPtr srcData )
{
	// store that data from that object internally
	for( EveSOFDataLayoutVector::const_iterator it = srcData->m_layout.begin(); it != srcData->m_layout.end(); ++it )
	{
		EveSOFDataLayoutPtr layoutData = ( *it );

		// if this pattern is already there, we have a problem!
		if( m_layoutData.find( layoutData->m_name ) != m_layoutData.end() )
		{
			CCP_LOGERR( "Found a duplicate layout name: %s", layoutData->m_name.c_str() );
			return false;
		}

		// fill the non-trinity struct with the provided data
		LayoutData ld;
		GenerateLayoutData( ld, layoutData );

		// put it into the main map
		m_layoutData[( *it )->m_name] = ld;
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
	rd.parameters.clear();
	for( auto mpit = srcData->m_parameters.begin(); mpit != srcData->m_parameters.end(); ++mpit )
	{
		EveSOFDataParameterPtr parameterData = ( *mpit );

		rd.parameters[parameterData->m_name] = parameterData->m_value;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Init generic data
// --------------------------------------------------------------------------------
bool EveSOFDataMgr::LoadGenericData( EveSOFDataPtr srcData )
{
	if( !srcData->m_generic )
	{
		return false;
	}

	GenerateGenericData( m_genericData, srcData->m_generic );

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Fill a non-trinity generic data struct with all the data from the trinity
//   data struct
// --------------------------------------------------------------------------------
void EveSOFDataMgr::GenerateGenericData( GenericData& gd, EveSOFDataGenericPtr srcData ) const
{
	// default textures
	gd.resPathDefaultAlliance = srcData->m_resPathDefaultAlliance;
	gd.resPathDefaultCorp = srcData->m_resPathDefaultCorp;
	gd.resPathDefaultCeo = srcData->m_resPathDefaultCeo;

	// shader locations
	gd.shaderPrefix = srcData->m_shaderPrefix;
	gd.shaderPrefixAnimated = srcData->m_shaderPrefixAnimated;
	gd.areaShaderLocation = srcData->m_areaShaderLocation;
	gd.decalShaderLocation = srcData->m_decalShaderLocation;
	std::copy( std::begin( srcData->m_decalMinScreenSizes ), std::end( srcData->m_decalMinScreenSizes ), std::begin( gd.decalMinScreenSize ) );

	// damage
	if( srcData->m_damage )
	{
		gd.damage.flickerPerlinAlpha = srcData->m_damage->m_flickerPerlinAlpha;
		gd.damage.flickerPerlinBeta = srcData->m_damage->m_flickerPerlinBeta;
		gd.damage.flickerPerlinN = srcData->m_damage->m_flickerPerlinN;
		gd.damage.flickerPerlinSpeed = srcData->m_damage->m_flickerPerlinSpeed;

		gd.damage.armorParticleRate = srcData->m_damage->m_armorParticleRate;
		gd.damage.armorParticleAngle = srcData->m_damage->m_armorParticleAngle;
		gd.damage.armorParticleMinMaxSpeed = srcData->m_damage->m_armorParticleMinMaxSpeed;
		gd.damage.armorParticleMinMaxLifeTime = srcData->m_damage->m_armorParticleMinMaxLifeTime;
		gd.damage.armorParticleSizes = srcData->m_damage->m_armorParticleSizes;
		gd.damage.armorParticleColors[0] = srcData->m_damage->m_armorParticleColor0;
		gd.damage.armorParticleColors[1] = srcData->m_damage->m_armorParticleColor1;
		gd.damage.armorParticleColors[2] = srcData->m_damage->m_armorParticleColor2;
		gd.damage.armorParticleColors[3] = srcData->m_damage->m_armorParticleColor3;
		gd.damage.armorParticleTextureIndex = srcData->m_damage->m_armorParticleTextureIndex;
		gd.damage.armorParticleVelocityStretchRotation = srcData->m_damage->m_armorParticleVelocityStretchRotation;
		gd.damage.armorParticleDrag = srcData->m_damage->m_armorParticleDrag;
		gd.damage.armorParticleTurbulenceAmplitude = srcData->m_damage->m_armorParticleTurbulenceAmplitude;
		gd.damage.armorParticleTurbulenceFrequency = srcData->m_damage->m_armorParticleTurbulenceFrequency;
		gd.damage.armorParticleColorMidPoint = srcData->m_damage->m_armorParticleColorMidPoint;

		gd.damage.armorShader = srcData->m_damage->m_armorShader;
		gd.damage.shieldShaderEllipsoid = srcData->m_damage->m_shieldShaderEllipsoid;
		gd.damage.shieldShaderHull = srcData->m_damage->m_shieldShaderHull;
		gd.damage.shieldGeometryResFilePath = srcData->m_damage->m_shieldGeometryResFilePath;
	}

	// damage
	if( srcData->m_hullDamage )
	{
		gd.hullDamage.hullParticleRate = srcData->m_hullDamage->m_hullParticleRate;
		gd.hullDamage.hullParticleAngle = srcData->m_hullDamage->m_hullParticleAngle;
		gd.hullDamage.hullParticleInnerAngle = srcData->m_hullDamage->m_hullParticleInnerAngle;
		gd.hullDamage.hullParticleMinMaxSpeed = srcData->m_hullDamage->m_hullParticleMinMaxSpeed;
		gd.hullDamage.hullParticleMinMaxLifeTime = srcData->m_hullDamage->m_hullParticleMinMaxLifeTime;
		gd.hullDamage.hullParticleSizes = srcData->m_hullDamage->m_hullParticleSizes;
		gd.hullDamage.hullParticleColors[0] = srcData->m_hullDamage->m_hullParticleColor0;
		gd.hullDamage.hullParticleColors[1] = srcData->m_hullDamage->m_hullParticleColor1;
		gd.hullDamage.hullParticleColors[2] = srcData->m_hullDamage->m_hullParticleColor2;
		gd.hullDamage.hullParticleColors[3] = srcData->m_hullDamage->m_hullParticleColor3;
		gd.hullDamage.hullParticleTextureIndex = srcData->m_hullDamage->m_hullParticleTextureIndex;
		gd.hullDamage.hullParticleVelocityStretchRotation = srcData->m_hullDamage->m_hullParticleVelocityStretchRotation;
		gd.hullDamage.hullParticleDrag = srcData->m_hullDamage->m_hullParticleDrag;
		gd.hullDamage.hullParticleTurbulenceAmplitude = srcData->m_hullDamage->m_hullParticleTurbulenceAmplitude;
		gd.hullDamage.hullParticleTurbulenceFrequency = srcData->m_hullDamage->m_hullParticleTurbulenceFrequency;
		gd.hullDamage.hullParticleColorMidpoint = srcData->m_hullDamage->m_hullParticleColorMidpoint;
	}

	// swarm
	if( srcData->m_swarm )
	{
		gd.swarmBehavior = srcData->m_swarm->m_behavior;
	}

	{
		EveSOFDataGenericShader* shaderData = &srcData->m_bannerShader;

		GenericBannerShaderData gsd;
		for( auto tdit = shaderData->m_defaultTextures.begin(); tdit != shaderData->m_defaultTextures.end(); ++tdit )
		{
			EveSOFDataTexturePtr textureData = ( *tdit );
			gsd.defaultTextures[textureData->m_name].resFilePath = textureData->m_resFilePath;
		}

		for( auto pdit = shaderData->m_defaultParameters.begin(); pdit != shaderData->m_defaultParameters.end(); ++pdit )
		{
			EveSOFDataParameterPtr paramData = ( *pdit );
			gsd.defaultParameters[paramData->m_name] = paramData->m_value;
		}

		gsd.shader = shaderData->m_shader.c_str();

		gd.bannerShader = gsd;
	}

	// shader material name prefixes
	gd.materialPrefixes.clear();
	for( auto mpit = srcData->m_materialPrefixes.begin(); mpit != srcData->m_materialPrefixes.end(); ++mpit )
	{
		EveSOFDataGenericStringPtr str = ( *mpit );

		gd.materialPrefixes.push_back( str->m_str );
	}

	// shader custom material name prefixes
	gd.patternMaterialPrefixes.clear();
	for( auto pmpit = srcData->m_patternMaterialPrefixes.begin(); pmpit != srcData->m_patternMaterialPrefixes.end(); ++pmpit )
	{
		EveSOFDataGenericStringPtr str = ( *pmpit );

		gd.patternMaterialPrefixes.push_back( str->m_str );
	}

	// area shader-specific data
	gd.areaShaderData.clear();
	for( auto asit = srcData->m_areaShaders.begin(); asit != srcData->m_areaShaders.end(); ++asit )
	{
		EveSOFDataGenericShaderPtr shaderData = ( *asit );

		GenericShaderData gsd;
		for( auto tdit = shaderData->m_defaultTextures.begin(); tdit != shaderData->m_defaultTextures.end(); ++tdit )
		{
			EveSOFDataTexturePtr textureData = ( *tdit );
			gsd.defaultTextures[textureData->m_name].resFilePath = textureData->m_resFilePath;
		}

		for( auto pdit = shaderData->m_defaultParameters.begin(); pdit != shaderData->m_defaultParameters.end(); ++pdit )
		{
			EveSOFDataParameterPtr paramData = ( *pdit );
			gsd.defaultParameters[paramData->m_name] = paramData->m_value;
		}

		for( auto spit = shaderData->m_parameters.begin(); spit != shaderData->m_parameters.end(); ++spit )
		{
			EveSOFDataGenericStringPtr paramData = ( *spit );
			gsd.parameters.push_back( BlueSharedString( paramData->m_str ) );
		}

		gsd.transparencyTextureName = shaderData->m_transparencyTextureName;
		gsd.doGenerateDepthArea = shaderData->m_doGenerateDepthArea;

		gd.areaShaderData[shaderData->m_shader] = gsd;
	}

	// decal shader-specific data
	gd.decalShaderData.clear();
	for( auto dsit = srcData->m_decalShaders.begin(); dsit != srcData->m_decalShaders.end(); ++dsit )
	{
		EveSOFDataGenericDecalShaderPtr shaderData = ( *dsit );

		GenericDecalShaderData gdsd;
		for( auto tdit = shaderData->m_defaultTextures.begin(); tdit != shaderData->m_defaultTextures.end(); ++tdit )
		{
			EveSOFDataTexturePtr textureData = ( *tdit );
			gdsd.defaultTextures[textureData->m_name].resFilePath = textureData->m_resFilePath;
		}

		for( auto tpit = shaderData->m_parentTextures.begin(); tpit != shaderData->m_parentTextures.end(); ++tpit )
		{
			EveSOFDataGenericStringPtr textureName = ( *tpit );
			gdsd.parentTextures.push_back( BlueSharedString( textureName->m_str ) );
		}

		for( auto spit = shaderData->m_parameters.begin(); spit != shaderData->m_parameters.end(); ++spit )
		{
			EveSOFDataGenericStringPtr paramData = ( *spit );
			gdsd.parameters.push_back( BlueSharedString( paramData->m_str ) );
		}
		gdsd.additive = shaderData->m_additive;

		gd.decalShaderData[shaderData->m_shader] = gdsd;
	}

	// generic wreck material
	if( srcData->m_genericWreckMaterial )
	{
		for( uint32_t i = 0; i < EveSOFDataAreaMaterial::MATERIAL_MAX; ++i )
		{
			gd.genericWreckMaterialData.materialNames[std::make_pair( EveSOFDataArea::TYPE_WRECK, i )] = BlueSharedString( srcData->m_genericWreckMaterial->m_material[i] );
		}

		gd.genericWreckMaterialData.glowColor.clear();
		gd.genericWreckMaterialData.glowColor[std::make_pair( EveSOFDataArea::TYPE_WRECK, "GeneralGlowColor" )] = srcData->m_genericWreckMaterial->m_glowColorType;
	}

	// variants
	gd.variants.clear();
	for( auto vit = srcData->m_variants.begin(); vit != srcData->m_variants.end(); ++vit )
	{
		EveSOFDataGenericVariantPtr variantData = ( *vit );
		if( variantData->m_hullArea )
		{
			VariantData vd;
			vd.hullAreaData = LoadHullAreaData( variantData->m_hullArea );
			vd.isTransparent = variantData->m_isTransparent;

			gd.variants[variantData->m_name] = vd;
		}
	}
}
