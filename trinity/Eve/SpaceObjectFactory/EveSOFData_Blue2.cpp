// June 2022 we had our first Blue class get too big for our compiler
// Thus, this first introduction of _Blue2. The future is now.

// This file is a continuation of the exposure in EveSOFData_Blue.cpp

#include "EveSOFData.h"

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
		MAP_ATTRIBUTE( "parameters", m_parameters, "", Be::READ | Be::PERSIST )
    EXPOSURE_END()
}


Be::VarChooser DisplayFlagModifierChooser[] = {
	{ "None_", BeCast( EveSOFDataInstancedMesh::SHADER_ALL ), "Visible to users with all shader settings" },
	{ "Medium_and_High", BeCast( EveSOFDataInstancedMesh::SHADER_HIGHMID ), "Visible for users with shader settings on Medium or High" },
	{ "Low_and_Medium", BeCast( EveSOFDataInstancedMesh::SHADER_LOWMID ), "Visible for users with shader settings on Low or Medium" },
	{ "High", BeCast( EveSOFDataInstancedMesh::SHADER_HIGH ), "Only visible for users with shader settings on High" },
	{ "Medium", BeCast( EveSOFDataInstancedMesh::SHADER_MED ), "Only visible for users with shader settings on Medium" },
	{ "Low", BeCast( EveSOFDataInstancedMesh::SHADER_LOW ), "Only visible for users with shader settings on Low" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "DisplayModifierChooser", EveSOFDataInstancedMesh::DisplayQualityModifier, DisplayFlagModifierChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

BLUE_DEFINE( EveSOFDataInstancedMesh );
const Be::ClassInfo* EveSOFDataInstancedMesh::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataInstancedMesh, "" )
		MAP_INTERFACE( EveSOFDataInstancedMesh )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lowestLodVisible", m_lowestLodVisible, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "displayModifier", m_displayModifier, "Selects when this instance is shown based on shader quality", Be::READWRITE | Be::PERSIST | Be::ENUM, DisplayFlagModifierChooser )
		MAP_ATTRIBUTE( "geometryResPath", m_geometryResPath, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "instances", m_instances, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "shader", m_shader, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "textures", m_textures, "", Be::READ | Be::PERSIST )
	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataFactionChild );
const Be::ClassInfo* EveSOFDataFactionChild::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataFactionChild, "" )
        MAP_INTERFACE( EveSOFDataFactionChild )

		MAP_ATTRIBUTE( "groupIndex", m_groupIndex, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isVisible", m_isVisible, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataHullDecalSet );
const Be::ClassInfo* EveSOFDataHullDecalSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataHullDecalSet, "" )
		MAP_INTERFACE( EveSOFDataHullDecalSet )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "visibilityGroup", m_visibilityGroup, "Name for visibility group to toggle visibility for the whole set.\n:jessica-widget: visibilitygroup", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "items", m_items, "The items in this decalset", Be::READ | Be::PERSIST )
	EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataDecalIndexBuffer );
const Be::ClassInfo* EveSOFDataDecalIndexBuffer::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataDecalIndexBuffer, "" )
		MAP_INTERFACE( EveSOFDataDecalIndexBuffer )
		MAP_INTERFACE( ICustomPersist )

		MAP_METHOD_AND_WRAP( "GetIndices", GetIndices, "Gets the index buffer array" )
		MAP_METHOD_AND_WRAP( "AddIndex", AddIndex, "Add an index to the index buffer" )
		MAP_ATTRIBUTE_AS_CUSTOM_BINARY_BLOCK( "indexBuffer" )
	EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHullLightSet );
const Be::ClassInfo* EveSOFDataHullLightSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataHullLightSet, "" )
		MAP_INTERFACE( EveSOFDataHullLightSet )
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "visibilityGroup", m_visibilityGroup, "Name for visibility group to toggle visibility for the whole set.\n:jessica-widget: visibilitygroup", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "items", m_items, "The items in this decalset", Be::READ | Be::PERSIST )
		EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataLogo );
const Be::ClassInfo* EveSOFDataLogo::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataLogo, "" )
		MAP_INTERFACE( EveSOFDataLogo )
		MAP_ATTRIBUTE( "textures", m_textures, "", Be::READ | Be::PERSIST )
		EXPOSURE_END()
}




BLUE_DEFINE( EveSOFDataBlink );
const Be::ClassInfo* EveSOFDataBlink::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataBlink, "" )
		MAP_INTERFACE( EveSOFDataBlink )
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
		MAP_ATTRIBUTE( "areaTypes", m_areaTypes, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "colorSet", m_colorSet, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "logoSet", m_logoSet, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "spotlightSets", m_spotlightSets, "All the groups of spotlight sets.", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "planeSets", m_planeSets, "All the groups of plane sets.", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "children", m_children, "All the groups of children.", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "visibilityGroupSet", m_visibilityGroupSet, "All visibility groups enabled on this faction.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "materialUsageMtl1", m_materialUsageMtl1, "Material usage of Mtl1\n:jessica-group: MaterialUsage", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "materialUsageMtl2", m_materialUsageMtl2, "Material usage of Mtl2\n:jessica-group: MaterialUsage", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "materialUsageMtl3", m_materialUsageMtl3, "Material usage of Mtl3\n:jessica-group: MaterialUsage", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "materialUsageMtl4", m_materialUsageMtl4, "Material usage of Mtl4\n:jessica-group: MaterialUsage", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "defaultPattern", m_defaultPattern, "The default pattern data for this faction\n:jessica-group: DefaultPattern", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "defaultPatternLayer1MaterialName", m_defaultPatternLayer1MaterialName, "The default pattern material name for this faction and layer 1\n:jessica-group: DefaultPattern\n:jessica-widget: materialpicker", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "defaultPatternLayer2MaterialName", m_defaultPatternLayer2MaterialName, "The default pattern material name for this faction and layer 2\n:jessica-group: DefaultPattern\n:jessica-sub-group: WIP \n:jessica-widget: materialpicker", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "defaultPatternName", m_defaultPatternName, "The default pattern used for this faction \n:jessica-widget: patternpicker \n:jessica-group: DefaultPattern\n:jessica-sub-group: WIP", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataRace );
const Be::ClassInfo* EveSOFDataRace::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataRace, "" )
        MAP_INTERFACE( EveSOFDataRace )

		MAP_ATTRIBUTE( "name", m_name, "The race name, eg caldari. This functions as an ID.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "booster", m_booster, "All the booster data for this race.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "hullPrimaryHeatColorType", m_hullPrimaryHeatColorType, "Per race heat (booster) color for primary hulls", Be::READWRITE | Be::PERSIST | Be::ENUM, SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "hullReactorHeatColorType", m_hullReactorHeatColorType, "Per race heat (booster) color for reactor hulls", Be::READWRITE | Be::PERSIST | Be::ENUM, SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser )
		MAP_ATTRIBUTE( "damage", m_damage, "Pre race damage system data", Be::READWRITE | Be::PERSIST )
		EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataMaterial );
const Be::ClassInfo* EveSOFDataMaterial::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataMaterial, "" )
        MAP_INTERFACE( EveSOFDataMaterial )

		MAP_ATTRIBUTE( "name", m_name, "The material name. This functions as an ID.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "parameters", m_parameters, "All the material parameters.", Be::READ | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataPattern );
const Be::ClassInfo* EveSOFDataPattern::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataPattern, "" )
		MAP_INTERFACE( EveSOFDataPattern )

		MAP_ATTRIBUTE( "sof6", m_sof6, "Used with SOF6", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "name", m_name, "The pattern name. This functions as an ID.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer1", m_layer1, "Pattern data for layer #1.", Be::READWRITE | Be::PERSIST)
		MAP_ATTRIBUTE( "layer2", m_layer2, "Pattern data for layer #2.", Be::READWRITE | Be::PERSIST)
		MAP_ATTRIBUTE( "projections", m_projections, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "applicationGroups", m_applicationGroups, "Groups that define how the pattern is applied to its hulls", Be::READ | Be::PERSIST )

	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataPatternTransform );
const Be::ClassInfo* EveSOFDataPatternTransform::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataPatternTransform, "" )
		MAP_INTERFACE( EveSOFDataPatternTransform )

		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isMirrored", m_isMirrored, "This pattern is mirrored across all this ship's hull.", Be::READWRITE | Be::PERSIST )
		EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataPatternMaterialOverride );
const Be::ClassInfo* EveSOFDataPatternMaterialOverride::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataPatternMaterialOverride, "" )
		MAP_INTERFACE( EveSOFDataPatternMaterialOverride )

		MAP_ATTRIBUTE( "isTargetMtl1", m_isTargetMtl1, "This pattern goes onto material 1", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isTargetMtl2", m_isTargetMtl2, "This pattern goes onto material 2", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isTargetMtl3", m_isTargetMtl3, "This pattern goes onto material 3", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isTargetMtl4", m_isTargetMtl4, "This pattern goes onto material 4", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataPatternApplicationGroup );
const Be::ClassInfo* EveSOFDataPatternApplicationGroup::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataPatternApplicationGroup, "" )
		MAP_INTERFACE( EveSOFDataPatternApplicationGroup )

		MAP_ATTRIBUTE( "name", m_name, "The name of the group", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer1Properties", m_layer1Properties, "Properties for layer 1 to define how to apply the layer to the hulls", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer2Properties", m_layer2Properties, "Properties for layer 2 to define how to apply the layer to the hulls", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "projections", m_projections, "The projections for different hulls", Be::READ | Be::PERSIST )

	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataPatternPerHull );
const Be::ClassInfo* EveSOFDataPatternPerHull::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataPatternPerHull, "" )
		MAP_INTERFACE( EveSOFDataPatternPerHull )

		MAP_ATTRIBUTE( "name", m_name, "The exact hull name. This functions as an ID.\n:jessica-widget: hullpicker", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "transformLayer1", m_transformLayer1, "Pattern projection transform for layer #1.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "transformLayer2", m_transformLayer2, "Pattern projection transform for layer #2.", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}

BLUE_DEFINE_INTERFACE( IEveSOFDataHullExtensionPlacement );

BLUE_DEFINE( EveSOFDataHullExtensionPlacement );
const Be::ClassInfo* EveSOFDataHullExtensionPlacement::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataHullExtensionPlacement, ":jessica-icon:far-folder" )
		MAP_INTERFACE( EveSOFDataHullExtensionPlacement )
		MAP_INTERFACE( IEveSOFDataHullExtensionPlacement )
		MAP_ATTRIBUTE( "name", m_name, "The name of the placement", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "enabled", m_enabled, "Mostly for use during Authoring to prevent loading segments while working on others", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "locatorSetName", m_locatorSetName, "The name of the locatorset to distribute the extension", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "offset", m_offset, "The offset of the extension (", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "distribution", m_distribution, "The distribution of the extensions. If empty, the extension are distributed to every locator", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "distributionConditions", m_distributionConditions, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "descriptor", m_descriptor, "The dna descriptor of the extension", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isInstanced", m_isInstanced, "is the mesh instanced? instanced meshes cannot be animated", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "extendsBoundingSphere", m_extendsBoundingSphere, "If true then it will grow the boundingsphere of the parent", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "extendsShieldEllipsoid", m_extendsShieldEllipsoid, "If true then it will grow the shield ellipsoid of the parent", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataHullExtensionPlacementGroup );
const Be::ClassInfo* EveSOFDataHullExtensionPlacementGroup::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataHullExtensionPlacementGroup, ":jessica-icon:far-folder-tree" )
		MAP_INTERFACE( EveSOFDataHullExtensionPlacementGroup )
		MAP_INTERFACE( IEveSOFDataHullExtensionPlacement )
		MAP_ATTRIBUTE( "name", m_name, "The name of the placement", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "enabled", m_enabled, "Mostly for use during Authoring to prevent loading segments while working on others", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "placements", m_placements, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "distributionConditions", m_distributionConditions, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "depletionCounters", m_depletionCounters, "", Be::READ | Be::PERSIST ) 
	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataLayout );
const Be::ClassInfo* EveSOFDataLayout::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataLayout, "" )
		MAP_INTERFACE( EveSOFDataLayout )
		MAP_ATTRIBUTE( "name", m_name, "The name of the layout", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "seed", m_seed, "used in all random processing for the placements", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "randomizeSeedOnLoad", m_randomizeSeedOnLoad, "this will non-proceduraly scramble the seed on load", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "depletionCounters", m_depletionCounters, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "placements", m_placements, "The placements of the layout", Be::READ | Be::PERSIST )

	EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataHullExtensionBucket );
const Be::ClassInfo* EveSOFDataHullExtensionBucket::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataHullExtensionBucket, "" )
		MAP_INTERFACE( EveSOFDataHullExtensionBucket )
		MAP_ATTRIBUTE( "name", m_name, "The name of the layout", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "depletionCounters", m_depletionCounters, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "placements", m_placements, "The placements of the layout", Be::READ | Be::PERSIST )
	EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataDistributionDepletionCounter );
const Be::ClassInfo* EveSOFDataDistributionDepletionCounter::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataDistributionDepletionCounter, ":jessica-icon:far-cookie" )
		MAP_INTERFACE( EveSOFDataDistributionDepletionCounter )
		MAP_ATTRIBUTE( "name", m_name, "The name the counter to Add or subtract from", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "value", m_value, "", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}

BLUE_DEFINE_INTERFACE( IEveSOFDataHullExtensionPlacementDistribution );

BLUE_DEFINE( EveSOFDataHullExtensionPlacementDistributionParentMatch );
const Be::ClassInfo* EveSOFDataHullExtensionPlacementDistributionParentMatch::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataHullExtensionPlacementDistributionParentMatch, ":jessica-icon:far-child" )
		MAP_INTERFACE( EveSOFDataHullExtensionPlacementDistributionParentMatch )
		MAP_INTERFACE( IEveSOFDataHullExtensionPlacementDistribution )
		MAP_ATTRIBUTE( "name", m_name, "The name of the distribution", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "parentDescriptor", m_parentDescriptor, "", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataHullExtensionPlacementDistributionDepletionCounter );
const Be::ClassInfo* EveSOFDataHullExtensionPlacementDistributionDepletionCounter::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataHullExtensionPlacementDistributionDepletionCounter, ":jessica-icon:far-cookie-bite" )
		MAP_INTERFACE( EveSOFDataHullExtensionPlacementDistributionDepletionCounter )
		MAP_INTERFACE( IEveSOFDataHullExtensionPlacementDistribution )
		MAP_ATTRIBUTE( "name", m_name, "The name the counter to Add or subtract from", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "depletionCounters", m_depletionCounters, "", Be::READ | Be::PERSIST )
	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataHullExtensionPlacementDistributionRandomChance );
const Be::ClassInfo* EveSOFDataHullExtensionPlacementDistributionRandomChance::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataHullExtensionPlacementDistributionRandomChance, ":jessica-icon:far-dice" )
		MAP_INTERFACE( EveSOFDataHullExtensionPlacementDistributionRandomChance )
		MAP_INTERFACE( IEveSOFDataHullExtensionPlacementDistribution )
		MAP_ATTRIBUTE( "name", m_name, "The name of the distribution", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "chanceOfUsage", m_chanceOfUsage, "[0:1], 1 = 100%", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataHullExtensionPlacementDistributionMapGraphicSettings );
const Be::ClassInfo* EveSOFDataHullExtensionPlacementDistributionMapGraphicSettings::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataHullExtensionPlacementDistributionMapGraphicSettings, ":jessica-icon:far-gear" )
		MAP_INTERFACE( EveSOFDataHullExtensionPlacementDistributionMapGraphicSettings )
		MAP_INTERFACE( IEveSOFDataHullExtensionPlacementDistribution )
		MAP_ATTRIBUTE( "name", m_name, "The name of the distribution", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "displayFilter", m_displayFilter, "Selects when this instance is picked based on shader quality", Be::READWRITE | Be::PERSIST | Be::ENUM, DisplayFlagModifierChooser )
	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataHullExtensionPlacementDistributionPlacement );
const Be::ClassInfo* EveSOFDataHullExtensionPlacementDistributionPlacement::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataHullExtensionPlacementDistributionPlacement, "ParentMatch\n:jessica-icon-color: (123, 28, 212)\n:jessica-icon:far-slot-machine" )
		MAP_INTERFACE( EveSOFDataHullExtensionPlacementDistributionPlacement )
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "completeness", m_completeness, "chance per locator of being utilized, 0.5=50%", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "placementBias", m_placementBias, "Vector to direct the spread of placements towards a direction", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "centerBias", m_centerBias, "0=doesn't care, -1=prioritizes edges, 1=starts from center", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "cap", m_cap, "cap on how many locators this distribution can utilize (0=uncapped)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "randomRotationStepSizeYPR", m_randomRotationStepSizeYPR, "step size for randomizing rotations, (yaw, pitch, roll) in degrees (90 = quarter circle)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "randomRotationMaxSteps", m_randomRotationMaxSteps, "max number of times the above stepSize can be added per locator", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "randomScaleMin", m_randomScaleMin, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "randomScaleMax", m_randomScaleMax, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "uniformScale", m_uniformScale, "when toggled the random value is on the linear axis between min and max", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "occupyLocators", m_occupyLocators, "when toggled the placement will reserve locator from other layouts", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDNADescriptor );
const Be::ClassInfo* EveSOFDNADescriptor::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDNADescriptor, ":jessica-icon:far-dna" )
		MAP_INTERFACE( EveSOFDNADescriptor )
		MAP_ATTRIBUTE( "hull", m_hull, "The hull extension of the dna (only hulls with buildclass BUILDCLASS_EXTENSION are permitted)\n:jessica-widget: hullextensionpicker", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "faction", m_faction, "The faction of the dna\n:jessica-widget: factionpicker", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "race", m_race, "The race of the dna\n:jessica-widget: racepicker", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layout", m_layout, "The layout of the dna\n:jessica-widget: layoutpicker", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "pattern", m_pattern, "The pattern of the dna\n:jessica-widget: patternpicker", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "material1", m_material1, "The 1. material of the dna\n:jessica-widget: materialpicker", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "material2", m_material2, "The 2. material of the dna\n:jessica-widget: materialpicker", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "material3", m_material3, "The 3. material of the dna\n:jessica-widget: materialpicker", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "material4", m_material4, "The 4. material of the dna\n:jessica-widget: materialpicker", Be::READWRITE | Be::PERSIST )
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

BLUE_DEFINE( EveSOFDataVisibilityGroup );
const Be::ClassInfo* EveSOFDataVisibilityGroup::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataVisibilityGroup, "" )
		MAP_INTERFACE( EveSOFDataVisibilityGroup )

		MAP_ATTRIBUTE( "name", m_name, "The name of the visibilty group", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "description", m_description, "", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataGenericDamage );
const Be::ClassInfo* EveSOFDataGenericDamage::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataGenericDamage, "" )
		MAP_INTERFACE( EveSOFDataGenericDamage )

		MAP_ATTRIBUTE( "flickerPerlinSpeed", m_flickerPerlinSpeed, "Hull damage perlin noise flicker speed", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "flickerPerlinAlpha", m_flickerPerlinAlpha, "Hull damage perlin noise flicker speed", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "flickerPerlinBeta", m_flickerPerlinBeta, "Hull damage perlin noise flicker speed", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "flickerPerlinN", m_flickerPerlinN, "Hull damage perlin noise flicker speed", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorParticleRate", m_armorParticleRate, "Armor damage impacte particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorParticleAngle", m_armorParticleAngle, "Armor damage impacte particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorParticleMinMaxSpeed", m_armorParticleMinMaxSpeed, "Armor damage impacte particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorParticleMinMaxLifeTime", m_armorParticleMinMaxLifeTime, "Armor damage impacte particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorParticleSizes", m_armorParticleSizes, "Armor damage impacte particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorParticleColor0", m_armorParticleColor0, "Armor damage impacte particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorParticleColor1", m_armorParticleColor1, "Armor damage impacte particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorParticleColor2", m_armorParticleColor2, "Armor damage impacte particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorParticleColor3", m_armorParticleColor3, "Armor damage impacte particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorParticleTextureIndex", m_armorParticleTextureIndex, "Armor damage impacte particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorParticleVelocityStretchRotation", m_armorParticleVelocityStretchRotation, "Armor damage impacte particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorParticleDrag", m_armorParticleDrag, "Armor damage impacte particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorParticleTurbulenceAmplitude", m_armorParticleTurbulenceAmplitude, "Armor damage impacte particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorParticleTurbulenceFrequency", m_armorParticleTurbulenceFrequency, "Armor damage impacte particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorParticleColorMidPoint", m_armorParticleColorMidPoint, "Armor damage impacte particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "armorShader", m_armorShader, "Shader for armor damage", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shieldShaderEllipsoid", m_shieldShaderEllipsoid, "Shader for elliptical shield impact", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shieldShaderHull", m_shieldShaderHull, "Shader for hull shield impact", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shieldGeometryResFilePath", m_shieldGeometryResFilePath, "Geometry for shield impact", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataGenericHullDamage );
const Be::ClassInfo* EveSOFDataGenericHullDamage::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataGenericHullDamage, "" )
		MAP_INTERFACE( EveSOFDataGenericHullDamage )

		MAP_ATTRIBUTE( "hullParticleRate", m_hullParticleRate, "hull damage impact particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullParticleAngle", m_hullParticleAngle, "hull damage impact particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullParticleColorMidpoint", m_hullParticleColorMidpoint, "hull damage impact particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullParticleInnerAngle", m_hullParticleInnerAngle, "hull damage impact particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullParticleMinMaxSpeed", m_hullParticleMinMaxSpeed, "hull damage impact particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullParticleMinMaxLifeTime", m_hullParticleMinMaxLifeTime, "hull damage impact particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullParticleSizes", m_hullParticleSizes, "hull damage impact particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullParticleColor0", m_hullParticleColor0, "hull damage impact particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullParticleColor1", m_hullParticleColor1, "hull damage impact particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullParticleColor2", m_hullParticleColor2, "hull damage impact particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullParticleColor3", m_hullParticleColor3, "hull damage impact particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullParticleTextureIndex", m_hullParticleTextureIndex, "hull damage impact particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullParticleVelocityStretchRotation", m_hullParticleVelocityStretchRotation, "hull damage impact particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullParticleDrag", m_hullParticleDrag, "hull damage impact particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullParticleTurbulenceAmplitude", m_hullParticleTurbulenceAmplitude, "hull damage impact particlesystem data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullParticleTurbulenceFrequency", m_hullParticleTurbulenceFrequency, "hull damage impact particlesystem data", Be::READWRITE | Be::PERSIST )
		EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataGenericSwarm );
const Be::ClassInfo* EveSOFDataGenericSwarm::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataGenericSwarm, "" )
		MAP_INTERFACE( EveSOFDataGenericSwarm )

		MAP_ATTRIBUTE( "speedMultiplier", m_behavior.m_speedMultiplier, "Max swarmer speed = shipSpeed*speedMultiplier+speedMinimum", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "speedMinimum", m_behavior.m_speedMinimum, "Max swarmer speed = shipSpeed*speedMultiplier+speedMinimum", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxDistance0", m_behavior.m_maxDistance0, "Max allowed distance from world position", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxDistance1", m_behavior.m_maxDistance1, "Max allowed distance from world position", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxTime", m_behavior.m_maxTime, "Maximum time for simulation per update", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "speed0", m_behavior.m_speed0, "Lower speed limit for max distance", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "speed1", m_behavior.m_speed1, "Upper speed limit for max distance", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "weightFormation", m_behavior.m_weightFormation, "Weight of triangle formation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "weightCohesion", m_behavior.m_weightCohesion, "Cohesion weight, steer swarmers to center of mass", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "weightSeparation", m_behavior.m_weightSeparation, "Weight of steering away from nearby swarmers", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "weightAlign", m_behavior.m_weightAlign, "This should be in newtons, based of average direction of swarmers", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "weightWander", m_behavior.m_weightWander, "Weight of wandering behavior", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "weightAnchor", m_behavior.m_weightAnchor, "Weight of the force steering swarmers to the world position", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "anchorRadius0", m_behavior.m_anchorRadius0, "Anchor force reaches zero here", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "anchorRadius1", m_behavior.m_anchorRadius1, "Anchor force is at maximum beyond this distance", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "weightDeceleration", m_behavior.m_weightDecelerate, "Weight of deceleration(multiplied with current velocity)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxDeceleration", m_behavior.m_maxDeceleration, "Maximum amount of deceleration in newtons", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "separationDistance", m_behavior.m_separationDistance, "Distance for separation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "formationDistance", m_behavior.m_formationDistance, "Distance formation", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "wanderFluctuation", m_behavior.m_wanderFluctuation, "Defines how fast wander target point on projected sphere fluctuates", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "wanderDistance", m_behavior.m_wanderDistance, "Distance of projected sphere in front of swarmers", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "wanderRadius", m_behavior.m_wanderRadius, "Radius of the projected sphere", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataGenericShader );
const Be::ClassInfo* EveSOFDataGenericShader::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataGenericShader, "" )
        MAP_INTERFACE( EveSOFDataGenericShader )

		MAP_ATTRIBUTE( "shader", m_shader, "The actual shader", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "transparencyTextureName", m_transparencyTextureName, "Some shaders have transparency maps.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "doGenerateDepthArea", m_doGenerateDepthArea, "Some shaders need an accompanying depth area.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "parameters", m_parameters, "Complete list of all parameters for this shader", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "defaultTextures", m_defaultTextures, "Default (global) textures for this shader", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "defaultParameters", m_defaultParameters, "Default (global) parameters for this shader", Be::READ | Be::PERSIST )
		EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataGenericDecalShader );
const Be::ClassInfo* EveSOFDataGenericDecalShader::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataGenericDecalShader, "" )
		MAP_INTERFACE( EveSOFDataGenericDecalShader )

		MAP_ATTRIBUTE( "shader", m_shader, "The actual shader", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "parameters", m_parameters, "Complete list of all parameters for this shader", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "defaultTextures", m_defaultTextures, "Default (global) textures for this shader", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "parentTextures", m_parentTextures, "Parent textures from the hull", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "additive", m_additive, "Render decals with this shader during the additive pass rather than the decal pass to make sure they are rendered on top of any other decals", Be::READ | Be::PERSIST )
	EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataGenericVariant );
const Be::ClassInfo* EveSOFDataGenericVariant::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataGenericVariant, "" )
		MAP_INTERFACE( EveSOFDataGenericVariant )

		MAP_ATTRIBUTE( "name", m_name, "Variant name", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isTransparent", m_isTransparent, "What area does it go into", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullArea", m_hullArea, "The actual hull area data", Be::READWRITE | Be::PERSIST )
		EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataGenericHullCategory );
const Be::ClassInfo* EveSOFDataGenericHullCategory::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataGenericHullCategory, "" )
		MAP_INTERFACE( EveSOFDataGenericHullCategory )

		MAP_ATTRIBUTE( "name", m_categoryName, "The hull category", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "reflectionMode", m_reflectionMode, "When this hull category is rendered in the cubemap", Be::READWRITE | Be::PERSIST | Be::ENUM, EntityComponents::ReflectionModeChooser )

		EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataGeneric );
const Be::ClassInfo* EveSOFDataGeneric::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataGeneric, "" )
        MAP_INTERFACE( EveSOFDataGeneric )

		MAP_ATTRIBUTE( "resPathDefaultAlliance", m_resPathDefaultAlliance, "The texture for the default alliance logo\n:jessica-group: DefaultTextures\n:jessica-widget: filepath\n:jessica-file-filter: texture", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "resPathDefaultCorp", m_resPathDefaultCorp, "The texture for the default corp logo\n:jessica-group: DefaultTextures\n:jessica-widget: filepath\n:jessica-file-filter: texture", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "resPathDefaultCeo", m_resPathDefaultCeo, "The texture for the default ceo logo\n:jessica-group: DefaultTextures\n:jessica-widget: filepath\n:jessica-file-filter: texture", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "areaShaderLocation", m_areaShaderLocation, "The location of all the area shaders\n:jessica-group: ShaderInfo", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "decalShaderLocation", m_decalShaderLocation, "The location of all the decal shaders\n:jessica-group: ShaderInfo", Be::READWRITE | Be::PERSIST )

#define MAP_SCREENSIZE( sc ) MAP_ATTRIBUTE( "decalMinScreenSize" # sc, m_decalMinScreenSizes[EveSOFDataHullDecalSetItem::USAGE_##sc], ":jessica-group: Decal Min Screen Size", Be::READWRITE | Be::PERSIST )

		MAP_SCREENSIZE( STANDARD )
		MAP_SCREENSIZE( KILLCOUNTER )
		MAP_SCREENSIZE( HOLE )
		MAP_SCREENSIZE( CYLINDRICAL )
		MAP_SCREENSIZE( GLOWCYLINDRICAL )
		MAP_SCREENSIZE( GLOWSTANDARD )
		MAP_SCREENSIZE( LOGO )

#undef MAP_SCREENSIZE

		MAP_ATTRIBUTE( "shaderPrefix", m_shaderPrefix, "A prefix for all shaders\n:jessica-group: ShaderInfo", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shaderPrefixAnimated", m_shaderPrefixAnimated, "A prefix for all skinned shaders\n:jessica-group: ShaderInfo", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "materialPrefixes", m_materialPrefixes, "List of all the support material prefixes", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "patternMaterialPrefixes", m_patternMaterialPrefixes, "List of all the supported pattern material prefixes", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "areaShaders", m_areaShaders, "List of all the area shaders and their generic data", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "decalShaders", m_decalShaders, "List of all the decal shaders and their generic data", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "damage", m_damage, "Global visual damage data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hullDamage", m_hullDamage, "Global visual hull damage data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "swarm", m_swarm, "Global swarm behavior preset data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "bannerShader", m_bannerShader, "Banner shader", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "genericWreckMaterial", m_genericWreckMaterial, "Global wreck area shader data", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "variants", m_variants, "All the hull  variants", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "visibilityGroups", m_visibilityGroups, "All the visibility groups", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "hullCategories", m_hullCategories, "All the hull categories", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "hullCategoriesData", m_hullCategoryData, "New hull categories that contain reflection settings", Be::READ | Be::PERSIST )

		EXPOSURE_END()
}



BLUE_DEFINE( EveSOFData );
const Be::ClassInfo* EveSOFData::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFData, "" )
        MAP_INTERFACE( EveSOFData )

		MAP_ATTRIBUTE( "generic", m_generic, "All the generic data we have in EVE", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hull", m_hull, "All the hull data we have in EVE", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "faction", m_faction, "All the factions data we have in EVE", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "race", m_race, "All the racial data we have in EVE", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "material", m_material, "All the material data we have in EVE", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "pattern", m_pattern, "All the pattern data we have in EVE", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "layout", m_layout, "All the layout data we have in EVE", Be::READ | Be::PERSIST )
	EXPOSURE_END()
}
