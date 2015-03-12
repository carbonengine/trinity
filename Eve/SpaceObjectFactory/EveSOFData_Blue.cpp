////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "EveSOFData.h"

BLUE_DEFINE( EveSOFDataBooster );
const Be::ClassInfo* EveSOFDataBooster::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataBooster, "" )
        MAP_INTERFACE( EveSOFDataBooster )

		MAP_ATTRIBUTE( "color", m_color, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scale", m_scale, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "glowColor", m_glowColor, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "glowScale", m_glowScale, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "haloColor", m_haloColor, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "haloScaleX", m_haloScaleX, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "haloScaleY", m_haloScaleY, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "symHaloScale", m_symHaloScale, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "textureResPath", m_textureResPath, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "trailColor", m_trailColor, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "trailSize", m_trailSize, "", Be::READWRITE | Be::PERSIST )

    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHullSpotlightSetItem );
const Be::ClassInfo* EveSOFDataHullSpotlightSetItem::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullSpotlightSetItem, "" )
        MAP_INTERFACE( EveSOFDataHullSpotlightSetItem )

		MAP_ATTRIBUTE( "transform", m_transform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_boneIndex, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boosterGainInfluence", m_boosterGainInfluence, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "groupIndex", m_groupIndex, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "spriteScale", m_spriteScale, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "coneIntensity", m_coneIntensity, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "flareIntensity", m_flareIntensity, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "spriteIntensity", m_spriteIntensity, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHullSpotlightSet );
const Be::ClassInfo* EveSOFDataHullSpotlightSet::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullSpotlightSet, "" )
        MAP_INTERFACE( EveSOFDataHullSpotlightSet )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "skinned", m_skinned, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "zOffset", m_zOffset, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "coneTextureResPath", m_coneTextureResPath, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "glowTextureResPath", m_glowTextureResPath, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "items", m_items, "The items in this spotlightset", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHullPlaneSetItem );
const Be::ClassInfo* EveSOFDataHullPlaneSetItem::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullPlaneSetItem, "" )
        MAP_INTERFACE( EveSOFDataHullPlaneSetItem )

		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "color", m_color, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer1Transform", m_layer1Transform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer1Scroll", m_layer1Scroll, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer2Transform", m_layer2Transform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer2Scroll", m_layer2Scroll, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_boneIndex, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "groupIndex", m_groupIndex, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHullPlaneSet );
const Be::ClassInfo* EveSOFDataHullPlaneSet::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullPlaneSet, "" )
        MAP_INTERFACE( EveSOFDataHullPlaneSet )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer1MapResPath", m_layer1MapResPath, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer2MapResPath", m_layer2MapResPath, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maskMapResPath", m_maskMapResPath, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "planeData", m_planeData, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "skinned", m_skinned, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "items", m_items, "The items in this planeset", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataFactionSpriteSet );
const Be::ClassInfo* EveSOFDataFactionSpriteSet::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataFactionSpriteSet, "" )
        MAP_INTERFACE( EveSOFDataFactionSpriteSet )

		MAP_ATTRIBUTE( "groupIndex", m_groupIndex, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isVisible", m_isVisible, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "color", m_color, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataFactionSpotlightSet );
const Be::ClassInfo* EveSOFDataFactionSpotlightSet::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataFactionSpotlightSet, "" )
        MAP_INTERFACE( EveSOFDataFactionSpotlightSet )

		MAP_ATTRIBUTE( "groupIndex", m_groupIndex, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "coneColor", m_coneColor, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "spriteColor", m_spriteColor, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "flareColor", m_flareColor, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataFactionPlaneSet );
const Be::ClassInfo* EveSOFDataFactionPlaneSet::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataFactionPlaneSet, "" )
        MAP_INTERFACE( EveSOFDataFactionPlaneSet )

		MAP_ATTRIBUTE( "groupIndex", m_groupIndex, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "color", m_color, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHullSpriteSetItem );
const Be::ClassInfo* EveSOFDataHullSpriteSetItem::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullSpriteSetItem, "" )
        MAP_INTERFACE( EveSOFDataHullSpriteSetItem )

		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "blinkRate", m_blinkRate, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "blinkPhase", m_blinkPhase, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minScale", m_minScale, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxScale", m_maxScale, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "falloff", m_falloff, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_boneIndex, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "groupIndex", m_groupIndex, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHullSpriteSet );
const Be::ClassInfo* EveSOFDataHullSpriteSet::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullSpriteSet, "" )
        MAP_INTERFACE( EveSOFDataHullSpriteSet )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "skinned", m_skinned, "Is this spriteset bone-animated.", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "items", m_items, "The items in this spriteset", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHullBooster );
const Be::ClassInfo* EveSOFDataHullBooster::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullBooster, "" )
        MAP_INTERFACE( EveSOFDataHullBooster )

		MAP_ATTRIBUTE( "alwaysOn", m_alwaysOn, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hasTrails", m_hasTrails, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "items", m_items, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHullBoosterItem );
const Be::ClassInfo* EveSOFDataHullBoosterItem::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullBoosterItem, "" )
        MAP_INTERFACE( EveSOFDataHullBoosterItem )

		MAP_ATTRIBUTE( "transform", m_transform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "functionality", m_functionality, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hasTrail", m_hasTrail, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataTexture );
const Be::ClassInfo* EveSOFDataTexture::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataTexture, "" )
        MAP_INTERFACE( EveSOFDataTexture )

		MAP_ATTRIBUTE( "resFilePath", m_resFilePath, "Resource path.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "name", m_name, "Parameter name of this texture", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHullArea );
const Be::ClassInfo* EveSOFDataHullArea::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullArea, "" )
        MAP_INTERFACE( EveSOFDataHullArea )

		MAP_ATTRIBUTE( "index", m_index, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "count", m_count, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shader", m_shader, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "textures", m_textures, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "parameters", m_parameters, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataHullLocator );
const Be::ClassInfo* EveSOFDataHullLocator::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullLocator, "" )
        MAP_INTERFACE( EveSOFDataHullLocator )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "transform", m_transform, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataTransform );
const Be::ClassInfo* EveSOFDataTransform::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataTransform, "" )
        MAP_INTERFACE( EveSOFDataTransform )

		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataHullChild );
const Be::ClassInfo* EveSOFDataHullChild::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullChild, "" )
        MAP_INTERFACE( EveSOFDataHullChild )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "redFilePath", m_redFilePath, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "id", m_id, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataHullAnimation );
const Be::ClassInfo* EveSOFDataHullAnimation::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullAnimation, "" )
        MAP_INTERFACE( EveSOFDataHullAnimation )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "startRotationValue", m_startRotationValue, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "endRotationValue", m_endRotationValue, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "startRotationTime", m_startRotationTime, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "endRotationTime", m_endRotationTime, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "startTranslationValue", m_startTranslationValue, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "endTranslationValue", m_endTranslationValue, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "startTranslationTime", m_startTranslationTime, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "endTranslationTime", m_endTranslationTime, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "id", m_id, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "startRate", m_startRate, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "endRate", m_endRate, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHull );
const Be::ClassInfo* EveSOFDataHull::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHull, "" )
        MAP_INTERFACE( EveSOFDataHull )

		MAP_ATTRIBUTE( "name", m_name, "The hull name, eg cb2_t1. This functions as an ID.", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "description", m_description, "A description string. NOT used by the SOF, it's just for debugging purposes.", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "geometryResFilePath", m_geometryResFilePath, "The res file path to the gr2 file", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boundingSphere", m_boundingSphere, "The actual size of the gemoetry", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isSkinned", m_isSkinned, "Does this hull need skinned shaders?", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "opaqueAreas", m_opaqueAreas, "The opaque areas on this mesh", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "decalAreas", m_decalAreas, "The decal areas on this mesh", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "transparentAreas", m_transparentAreas, "The transparent areas on this mesh", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "additiveAreas", m_additiveAreas, "The additive areas on this mesh", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "depthAreas", m_depthAreas, "The depth areas on this mesh", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "distortionAreas", m_distortionAreas, "The distortion areas on this mesh", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "spriteSets", m_spriteSets, "The spritesets", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "spotlightSets", m_spotlightSets, "The spotlightsets", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "planeSets", m_planeSets, "The planesets", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "hullDecals", m_hullDecals, "The hull decals", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "booster", m_booster, "The booster", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "audioPosition", m_audioPosition, "The audio position", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "children", m_children, "List of children", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "animations", m_animations, "List of animations", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "locatorTurrets", m_locatorTurrets, "Turret locators", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "damageLocators", m_damageLocators, "Damage locators", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "modelRotationCurvePath", m_modelRotationCurvePath, "Model rotation curve path", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "modelTranslationCurvePath", m_modelTranslationCurvePath, "Model translation curve path", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataParameter );
const Be::ClassInfo* EveSOFDataParameter::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataParameter, "" )
        MAP_INTERFACE( EveSOFDataParameter )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "value", m_value, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataFactionHullArea );
const Be::ClassInfo* EveSOFDataFactionHullArea::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataFactionHullArea, "" )
        MAP_INTERFACE( EveSOFDataFactionHullArea )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "parameters", m_parameters, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataFactionDecal );
const Be::ClassInfo* EveSOFDataFactionDecal::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataFactionDecal, "" )
        MAP_INTERFACE( EveSOFDataFactionDecal )

		MAP_ATTRIBUTE( "groupIndex", m_groupIndex, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isVisible", m_isVisible, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shader", m_shader, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "textures", m_textures, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "parameters", m_parameters, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHullDecal );
const Be::ClassInfo* EveSOFDataHullDecal::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullDecal, "" )
        MAP_INTERFACE( EveSOFDataHullDecal )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shader", m_shader, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "groupIndex", m_groupIndex, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_boneIndex, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "parameters", m_parameters, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "textures", m_textures, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "indexBuffer", m_indexBuffer, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataFaction );
const Be::ClassInfo* EveSOFDataFaction::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataFaction, "" )
        MAP_INTERFACE( EveSOFDataFaction )

		MAP_ATTRIBUTE( "name", m_name, "The faction name, eg sarum. This functions as an ID.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "description", m_description, "A description string. NOT used by the SOF, it's just for debugging purposes.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "resPathInsert", m_resPathInsert, "Insert string to build texture res path.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "areas", m_areas, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "decals", m_decals, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "spriteSets", m_spriteSets, "All the groups of sprite sets.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "spotlightSets", m_spotlightSets, "All the groups of spotlight sets.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "planeSets", m_planeSets, "All the groups of plane sets.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "materialUsageMtl1", m_materialUsageMtl1, "Material usage of Mtl1", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "materialUsageMtl2", m_materialUsageMtl2, "Material usage of Mtl2", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "materialUsageMtl3", m_materialUsageMtl3, "Material usage of Mtl3", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "materialUsageMtl4", m_materialUsageMtl4, "Material usage of Mtl4", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataRace );
const Be::ClassInfo* EveSOFDataRace::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataRace, "" )
        MAP_INTERFACE( EveSOFDataRace )

		MAP_ATTRIBUTE( "name", m_name, "The race name, eg caldari. This functions as an ID.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "booster", m_booster, "All the booster data for this race.", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataMaterial );
const Be::ClassInfo* EveSOFDataMaterial::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataMaterial, "" )
        MAP_INTERFACE( EveSOFDataMaterial )

		MAP_ATTRIBUTE( "name", m_name, "The material name. This functions as an ID.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "parameters", m_parameters, "All the material parameters.", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataGenericString );
const Be::ClassInfo* EveSOFDataGenericString::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataGenericString, "" )
        MAP_INTERFACE( EveSOFDataGenericString )

		MAP_ATTRIBUTE( "str", m_str, "The actual string", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataGenericShader );
const Be::ClassInfo* EveSOFDataGenericShader::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataGenericShader, "" )
        MAP_INTERFACE( EveSOFDataGenericShader )

		MAP_ATTRIBUTE( "shader", m_shader, "The actual shader", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "textures", m_textures, "Generic textures for this shader", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataGeneric );
const Be::ClassInfo* EveSOFDataGeneric::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataGeneric, "" )
        MAP_INTERFACE( EveSOFDataGeneric )

		MAP_ATTRIBUTE( "areaShaderLocation", m_areaShaderLocation, "The location of all the area shaders", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "decalShaderLocation", m_decalShaderLocation, "The location of all the decal shaders", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shaderPrefix", m_shaderPrefix, "A prefix for all shaders", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shaderPrefixAnimated", m_shaderPrefixAnimated, "A prefix for all skinned shaders", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "materialPrefixes", m_materialPrefixes, "List of all the support material prefixes", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shaders", m_shaders, "List of all the shaders and their generic data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullAreas", m_hullAreas, "Global hull area shader data", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFData );
const Be::ClassInfo* EveSOFData::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFData, "" )
        MAP_INTERFACE( EveSOFData )

		MAP_ATTRIBUTE( "generic", m_generic, "All the generic data we have in EVE", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hull", m_hull, "All the hull data we have in EVE", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "faction", m_faction, "All the factions data we have in EVE", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "race", m_race, "All the racial data we have in EVE", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "material", m_material, "All the material data we have in EVE", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}
