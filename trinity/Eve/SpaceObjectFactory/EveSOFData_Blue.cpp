////////////////////////////////////////////////////////////
//
//    Created:   August 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "EveSOFData.h"
#include "Lights/Tr2Light.h"


namespace SOFDataFactionColorChooser
{
const Be::VarChooser EveSOFDataFactionColorSetTypeChooser[] =
        {
                { "Primary", BeCast( TYPE_PRIMARY ), "Primary Color" },
                { "Secondary", BeCast( TYPE_SECONDARY ), "Secondary Color" },
                { "Tertiary", BeCast( TYPE_TERTIARY ), "Tertiary Color" },
                { "Black", BeCast( TYPE_BLACK ), "Black Color" },
                { "White", BeCast( TYPE_WHITE ), "White" },
                { "Yellow", BeCast( TYPE_YELLOW ), "Yellow" },
                { "Orange", BeCast( TYPE_ORANGE ), "Orange" },
                { "Red", BeCast( TYPE_RED ), "Red" },
                { "Blue", BeCast( TYPE_BLUE ), "Blue" },
                { "Green", BeCast( TYPE_GREEN ), "Green" },
                { "Cyan", BeCast( TYPE_CYAN ), "Cyan" },
                { "Fire", BeCast( TYPE_FIRE ), "Fire" },
                { "Hull", BeCast( TYPE_HULL ), "Material Hullarea Glow" },
                { "Glass", BeCast( TYPE_GLASS ), "Material Glassarea Glow" },
                { "Reactor", BeCast( TYPE_REACTOR ), "Material Reactorarea Glow" },
                { "Darkhull", BeCast( TYPE_DARKHULL ), "Material Darkhull Glow" },
                { "Booster", BeCast( TYPE_BOOSTER ), "Material Hullarea Heat Shimmer Glow" },
                { "Killmark", BeCast( TYPE_KILLMARK ), "Killmark glow color" },
                { "PrimaryLight", BeCast( TYPE_PRIMARY_LIGHT ), "Primary light color" },
                { "SecondaryLight", BeCast( TYPE_SECONDARY_LIGHT ), "Secondary light color" },
                { "TertiaryLight", BeCast( TYPE_TERTIARY_LIGHT ), "Tertiary light color" },
                { "WhiteLight", BeCast( TYPE_WHITE_LIGHT ), "White light color" },
                { "PrimaryHologram", BeCast( TYPE_PRIMARY_HOLOGRAM ), "Primary Hologram"},
                { "SecondaryHologram", BeCast( TYPE_SECONDARY_HOLOGRAM ), "Secondary Hologram color" },
                { "TertiaryHologram", BeCast( TYPE_TERTIARY_HOLOGRAM ), "Tertiary Hologram color" },
                { "State0", BeCast( TYPE_STATE_0 ), "State 0 color" },
                { "State1", BeCast( TYPE_STATE_1 ), "State 1 color" },
                { "State2", BeCast( TYPE_STATE_2 ), "State 2 color" },
                { "State3", BeCast( TYPE_STATE_3 ), "State 3 color" },
                { "StateVulnerable", BeCast( TYPE_STATE_VULNERABLE ), "State Vulnerable color" },
                { "StateInvulnerable", BeCast( TYPE_STATE_INVULNERABLE ), "State Invulnerable color" },
                { "PrimaryForcefield", BeCast( TYPE_PRIMARY_FORCEFIELD ), "Primary Forcefield color" },
                { "SecondaryForcefield", BeCast( TYPE_SECONDARY_FORCEFIELD ), "Secondary Forcefield color" },
                { "PrimaryBanner", BeCast( TYPE_PRIMARY_BANNER ), "Primary Banner color" },
                { "PrimaryFx", BeCast( TYPE_PRIMARY_FX ), "Primary Fx color" },
                { "SecondaryFx", BeCast( TYPE_SECONDARY_FX ), "Secondary Fx color" },
                { "PrimarySpotlight", BeCast( TYPE_PRIMARY_SPOTLIGHT ), "Primary spotlight color" },
                { "SecondarySpotlight", BeCast( TYPE_SECONDARY_SPOTLIGHT ), "Secondary spotlight color" },
                { "TertiarySpotlight", BeCast( TYPE_TERTIARY_SPOTLIGHT ), "Tertiary spotlight color" },
                { "PrimaryBillboard", BeCast( TYPE_PRIMARY_BILLBOARD ), "Primary Billboard color" },
                { "PrimaryWarpFx", BeCast( TYPE_PRIMARY_WARP_FX ), "Primary Warp FX color" },
                { "PrimaryAttackFX", BeCast( TYPE_PRIMARY_ATTACK_FX ), "Primary Attack FX color" },
                { "PrimarySiegeFX", BeCast( TYPE_PRIMARY_SIEGE_FX ), "Primary Siege FX color" },
                { "PrimaryDockedFX", BeCast( TYPE_PRIMARY_DOCKED_FX ), "Primary Docked FX color" },
                { 0 }
        };
BLUE_REGISTER_ENUM_EX( "EveSOFDataFactionColorSetType", ColorType, EveSOFDataFactionColorSetTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
}

Be::VarChooser EveSOFDataLogoSetTypeChooser[] =
{
	{ "Primary", BeCast( EveSOFDataLogoSet::TYPE_PRIMARY ), "Primary Logo" },
	{ "Secondary", BeCast( EveSOFDataLogoSet::TYPE_SECONDARY ), "Secondary Logo" },
	{ "Tertiary", BeCast( EveSOFDataLogoSet::TYPE_TERTIARY), "Tertiary Logo" },
	{ "Marking_01", BeCast( EveSOFDataLogoSet::TYPE_MARKING_01 ), "Marking 01 Logo" },
	{ "Marking_02", BeCast( EveSOFDataLogoSet::TYPE_MARKING_02 ), "Marking 02 Logo" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "EveSOFDataLogoSetType", EveSOFDataLogoSet::LogoType, EveSOFDataLogoSetTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

Be::VarChooser EveSOFDataPlaneSetBlinkTypeChooser [] =
{
	{ "Static", BeCast( EveSOFDataBlinkType::TYPE_STATIC ), "Static, no blinking" },
	{ "Blink", BeCast( EveSOFDataBlinkType::TYPE_BLINK ), "Regular blink" },
	{ "FadeIn", BeCast( EveSOFDataBlinkType::TYPE_FADE_IN ), "Fade in" },
	{ "FadeOut", BeCast( EveSOFDataBlinkType::TYPE_FADE_OUT ), "Fade out" },
	{ "Cycle", BeCast( EveSOFDataBlinkType::TYPE_CYCLE ), "Cycle (fade in/out)" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "EveSOFDataBlinkType", EveSOFDataBlinkType::BlinkType, EveSOFDataPlaneSetBlinkTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

BLUE_DEFINE( EveSOFDataAreaMaterial );
const Be::ClassInfo* EveSOFDataAreaMaterial::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataAreaMaterial, "" )
		MAP_INTERFACE( EveSOFDataAreaMaterial )

		MAP_ATTRIBUTE( "material1", m_material[MATERIAL1], ":jessica-widget: materialpicker", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "material2", m_material[MATERIAL2], ":jessica-widget: materialpicker", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "material3", m_material[MATERIAL3], ":jessica-widget: materialpicker", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "material4", m_material[MATERIAL4], ":jessica-widget: materialpicker", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "colorType", m_glowColorType, "", Be::READWRITE | Be::PERSIST | Be::ENUM, SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser )
		EXPOSURE_END()
}



Be::VarChooser EveSOFDataHazeTypeChooser[] =
{
	{ "Spherical", BeCast( EveSOFDataHullHazeSet::TYPE_SPHERICAL ), "Spherical Haze" },
	{ "HalfSpherical_DONOTUSE", BeCast( EveSOFDataHullHazeSet::TYPE_HALFSPHERICAL ), "HalfSpherical Haze" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "HazeType", EveSOFDataHullHazeSet::HazeType, EveSOFDataHazeTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );



Be::VarChooser EveSOFDataHullBuildFilterChooser[] = {
	{ "Standalone", BeCast( EveSOFDataHullBuildFilter::STANDALONE ), "Enabled for normal hulls" },
	{ "NonInstancedPlacement", BeCast( EveSOFDataHullBuildFilter::NON_INSTANCED_PLACEMENT ), "Enabled on placed hulls" },
	{ "InstancedPlacement", BeCast( EveSOFDataHullBuildFilter::INSTANCED_PLACEMENT ), "Enabled on placed instanced hulls" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "EveSOFDataHullBuildFilter", uint32_t, EveSOFDataHullBuildFilterChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );


Be::VarChooser EveSOFDataAreaTypeChooser[] =
{
	{ "Primary", BeCast( EveSOFDataArea::TYPE_PRIMARY ), "Primary Area Type" },
	{ "Glass", BeCast( EveSOFDataArea::TYPE_GLASS ), "Area Type Glass" },
	{ "Sails", BeCast( EveSOFDataArea::TYPE_SAILS ), "Area Type Sails" },
	{ "Reactor", BeCast( EveSOFDataArea::TYPE_REACTOR ), "Area Type Reactor" },
	{ "Darkhull", BeCast( EveSOFDataArea::TYPE_DARKHULL), "Area Type Dark Hull" },
	{ "Wreck", BeCast( EveSOFDataArea::TYPE_WRECK ), "Area Type Generic Wreck" },
	{ "Rock", BeCast( EveSOFDataArea::TYPE_ROCK ), "Area Type Rock" },
	{ "Monument", BeCast( EveSOFDataArea::TYPE_MONUMENT ), "Area Type Monument" },
	{ "Ornament", BeCast( EveSOFDataArea::TYPE_ORNAMENT ), "Area Type Ornament" },
	{ "SimplePrimary", BeCast( EveSOFDataArea::TYPE_SIMPLEPRIMARY ), "Simple Primary Area Type" },
	{ "NoOverwrite", BeCast( EveSOFDataArea::TYPE_NO_OVERWRITE ), "Area Type No Overwrite" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "EveSOFDataAreaType", EveSOFDataArea::AreaType, EveSOFDataAreaTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

BLUE_DEFINE( EveSOFDataArea );
const Be::ClassInfo* EveSOFDataArea::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataArea, "" )
		MAP_INTERFACE( EveSOFDataArea )

		MAP_ATTRIBUTE( EveSOFDataAreaTypeChooser[TYPE_PRIMARY].mKey, m_materials[TYPE_PRIMARY], "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataAreaTypeChooser[TYPE_GLASS].mKey, m_materials[TYPE_GLASS], "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataAreaTypeChooser[TYPE_SAILS].mKey, m_materials[TYPE_SAILS], "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataAreaTypeChooser[TYPE_REACTOR].mKey, m_materials[TYPE_REACTOR], "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataAreaTypeChooser[TYPE_DARKHULL].mKey, m_materials[TYPE_DARKHULL], "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataAreaTypeChooser[TYPE_ROCK].mKey, m_materials[TYPE_ROCK], "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataAreaTypeChooser[TYPE_MONUMENT].mKey, m_materials[TYPE_MONUMENT], "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataAreaTypeChooser[TYPE_ORNAMENT].mKey, m_materials[TYPE_ORNAMENT], "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataAreaTypeChooser[TYPE_SIMPLEPRIMARY].mKey, m_materials[TYPE_SIMPLEPRIMARY], "", Be::READWRITE | Be::PERSIST )
		EXPOSURE_END()
}





BLUE_DEFINE( EveSOFDataFactionColorSet );
const Be::ClassInfo* EveSOFDataFactionColorSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataFactionColorSet, "" )
		MAP_INTERFACE( EveSOFDataFactionColorSet )

		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_PRIMARY].mKey, m_colors[SOFDataFactionColorChooser::TYPE_PRIMARY], ":jessica-group:Glow Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_SECONDARY].mKey, m_colors[SOFDataFactionColorChooser::TYPE_SECONDARY], ":jessica-group:Glow Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_TERTIARY].mKey, m_colors[SOFDataFactionColorChooser::TYPE_TERTIARY], ":jessica-group:Glow Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_BLACK].mKey, m_colors[SOFDataFactionColorChooser::TYPE_BLACK], ":jessica-group:Glow Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_WHITE].mKey, m_colors[SOFDataFactionColorChooser::TYPE_WHITE], ":jessica-group:Glow Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_YELLOW].mKey, m_colors[SOFDataFactionColorChooser::TYPE_YELLOW], ":jessica-group:Glow Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_ORANGE].mKey, m_colors[SOFDataFactionColorChooser::TYPE_ORANGE], ":jessica-group:Glow Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_RED].mKey, m_colors[SOFDataFactionColorChooser::TYPE_RED], ":jessica-group:Glow Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_BLUE].mKey, m_colors[SOFDataFactionColorChooser::TYPE_BLUE], ":jessica-group:Glow Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_GREEN].mKey, m_colors[SOFDataFactionColorChooser::TYPE_GREEN], ":jessica-group:Glow Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_CYAN].mKey, m_colors[SOFDataFactionColorChooser::TYPE_CYAN], ":jessica-group:Glow Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_FIRE].mKey, m_colors[SOFDataFactionColorChooser::TYPE_FIRE], ":jessica-group:Glow Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_HULL].mKey, m_colors[SOFDataFactionColorChooser::TYPE_HULL], ":jessica-group:Material Glow Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_GLASS].mKey, m_colors[SOFDataFactionColorChooser::TYPE_GLASS], ":jessica-group:Material Glow Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_REACTOR].mKey, m_colors[SOFDataFactionColorChooser::TYPE_REACTOR], ":jessica-group:Material Glow Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_DARKHULL].mKey, m_colors[SOFDataFactionColorChooser::TYPE_DARKHULL], ":jessica-group:Material Glow Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_BOOSTER].mKey, m_colors[SOFDataFactionColorChooser::TYPE_BOOSTER], ":jessica-group:Material Glow Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_KILLMARK].mKey, m_colors[SOFDataFactionColorChooser::TYPE_KILLMARK], ":jessica-group:Decal Glow Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_PRIMARY_LIGHT].mKey, m_colors[SOFDataFactionColorChooser::TYPE_PRIMARY_LIGHT], ":jessica-group:Light Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_SECONDARY_LIGHT].mKey, m_colors[SOFDataFactionColorChooser::TYPE_SECONDARY_LIGHT], ":jessica-group:Light Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_TERTIARY_LIGHT].mKey, m_colors[SOFDataFactionColorChooser::TYPE_TERTIARY_LIGHT], ":jessica-group:Light Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_WHITE_LIGHT].mKey, m_colors[SOFDataFactionColorChooser::TYPE_WHITE_LIGHT], ":jessica-group:Light Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_PRIMARY_SPOTLIGHT].mKey, m_colors[SOFDataFactionColorChooser::TYPE_PRIMARY_SPOTLIGHT], ":jessica-group:Spotlight Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_SECONDARY_SPOTLIGHT].mKey, m_colors[SOFDataFactionColorChooser::TYPE_SECONDARY_SPOTLIGHT], ":jessica-group:Spotlight Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_TERTIARY_SPOTLIGHT].mKey, m_colors[SOFDataFactionColorChooser::TYPE_TERTIARY_SPOTLIGHT], ":jessica-group:Spotlight Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_PRIMARY_HOLOGRAM].mKey, m_colors[SOFDataFactionColorChooser::TYPE_PRIMARY_HOLOGRAM], ":jessica-group:Hologram Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_SECONDARY_HOLOGRAM].mKey, m_colors[SOFDataFactionColorChooser::TYPE_SECONDARY_HOLOGRAM], ":jessica-group:Hologram Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_TERTIARY_HOLOGRAM].mKey, m_colors[SOFDataFactionColorChooser::TYPE_TERTIARY_HOLOGRAM], ":jessica-group:Hologram Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_STATE_0].mKey, m_colors[SOFDataFactionColorChooser::TYPE_STATE_0], ":jessica-group:State Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_STATE_1].mKey, m_colors[SOFDataFactionColorChooser::TYPE_STATE_1], ":jessica-group:State Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_STATE_2].mKey, m_colors[SOFDataFactionColorChooser::TYPE_STATE_2], ":jessica-group:State Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_STATE_3].mKey, m_colors[SOFDataFactionColorChooser::TYPE_STATE_3], ":jessica-group:State Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_STATE_VULNERABLE].mKey, m_colors[SOFDataFactionColorChooser::TYPE_STATE_VULNERABLE], ":jessica-group:State Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_STATE_INVULNERABLE].mKey, m_colors[SOFDataFactionColorChooser::TYPE_STATE_INVULNERABLE], ":jessica-group:State Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_PRIMARY_FORCEFIELD].mKey, m_colors[SOFDataFactionColorChooser::TYPE_PRIMARY_FORCEFIELD], ":jessica-group:Forcefield Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_SECONDARY_FORCEFIELD].mKey, m_colors[SOFDataFactionColorChooser::TYPE_SECONDARY_FORCEFIELD], ":jessica-group:Forcefield Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_PRIMARY_BANNER].mKey, m_colors[SOFDataFactionColorChooser::TYPE_PRIMARY_BANNER], ":jessica-group:Planeset Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_PRIMARY_BILLBOARD].mKey, m_colors[SOFDataFactionColorChooser::TYPE_PRIMARY_BILLBOARD], ":jessica-group:Planeset Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_PRIMARY_FX].mKey, m_colors[SOFDataFactionColorChooser::TYPE_PRIMARY_FX], ":jessica-group:FX Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_SECONDARY_FX].mKey, m_colors[SOFDataFactionColorChooser::TYPE_SECONDARY_FX], ":jessica-group:FX Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_PRIMARY_WARP_FX].mKey, m_colors[SOFDataFactionColorChooser::TYPE_PRIMARY_WARP_FX], ":jessica-group:FX Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_PRIMARY_ATTACK_FX].mKey, m_colors[SOFDataFactionColorChooser::TYPE_PRIMARY_ATTACK_FX], ":jessica-group:FX Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_PRIMARY_SIEGE_FX].mKey, m_colors[SOFDataFactionColorChooser::TYPE_PRIMARY_SIEGE_FX], ":jessica-group:FX Colors", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser[SOFDataFactionColorChooser::TYPE_PRIMARY_DOCKED_FX].mKey, m_colors[SOFDataFactionColorChooser::TYPE_PRIMARY_DOCKED_FX], ":jessica-group:FX Colors", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataBoosterShape );
const Be::ClassInfo* EveSOFDataBoosterShape::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataBoosterShape, "" )
        MAP_INTERFACE( EveSOFDataBoosterShape )

		MAP_ATTRIBUTE( "noiseFunction", m_noiseFunction, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseSpeed", m_noiseSpeed, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseAmplitureStart", m_noiseAmplitureStart, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseAmplitureEnd", m_noiseAmplitureEnd, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseFrequency", m_noiseFrequency, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "color", m_color, "", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataBooster );
const Be::ClassInfo* EveSOFDataBooster::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataBooster, "" )
        MAP_INTERFACE( EveSOFDataBooster )

		MAP_ATTRIBUTE( "scale", m_scale, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "glowColor", m_glowColor, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "warpGlowColor", m_warpGlowColor, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "glowScale", m_glowScale, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "haloColor", m_haloColor, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "warpHalpColor", m_warpHaloColor, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "haloScaleX", m_haloScaleX, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "haloScaleY", m_haloScaleY, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "symHaloScale", m_symHaloScale, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "trailColor", m_trailColor, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "trailSize", m_trailSize, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shape0", m_shape0, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shape1", m_shape1, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "warpShape0", m_warpShape0, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "warpShape1", m_warpShape1, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shapeAtlasResPath", m_shapeAtlasResPath, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "gradient0ResPath", m_gradient0ResPath, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "gradient1ResPath", m_gradient1ResPath, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shapeAtlasHeight", m_shapeAtlasHeight, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shapeAtlasCount", m_shapeAtlasCount, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lightOffset", m_lightOffset, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lightRadius", m_lightRadius, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lightWarpRadius", m_lightWarpRadius, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lightFlickerAmplitude", m_lightFlickerAmplitude, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lightFlickerFrequency", m_lightFlickerFrequency, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lightColor", m_lightColor, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lightWarpColor", m_lightWarpColor, "", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataRaceDamage );
const Be::ClassInfo* EveSOFDataRaceDamage::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataRaceDamage, "" )
		MAP_INTERFACE( EveSOFDataRaceDamage )

		MAP_ATTRIBUTE( "armorImpactParameters", m_armorImpactParameters, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "armorImpactTextures", m_armorImpactTextures, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "shieldImpactParameters", m_shieldImpactParameters, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "shieldImpactTextures", m_shieldImpactTextures, "", Be::READ | Be::PERSIST )
		EXPOSURE_END()
}

BLUE_DEFINE(EveSOFDataPointLightAttachment);
const Be::ClassInfo* EveSOFDataPointLightAttachment::ExposeToBlue()
{
	EXPOSURE_BEGIN(EveSOFDataPointLightAttachment, "")
		MAP_INTERFACE(EveSOFDataPointLightAttachment)

		MAP_ATTRIBUTE( "saturation", m_saturation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "intensity", m_intensity, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "innerScaleMultiplier", m_innerScaleMultiplier, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "outerScaleMultiplier", m_outerScaleMultiplier, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseAmplitude", m_noiseAmplitude, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseFrequency", m_noiseFrequency, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseOctaves", m_noiseOctaves, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lightProfilePath", m_lightProfilePath, "", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}

BLUE_DEFINE(EveSOFDataSpotLightAttachment);
const Be::ClassInfo* EveSOFDataSpotLightAttachment::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataSpotLightAttachment, "" )
		MAP_INTERFACE( EveSOFDataSpotLightAttachment )

		MAP_ATTRIBUTE( "saturation", m_saturation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "intensity", m_intensity, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "innerAngleMultiplier", m_innerAngleMultiplier, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "outerAngleMultiplier", m_outerAngleMultiplier, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "innerScaleMultiplier", m_innerScaleMultiplier, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "outerScaleMultiplier", m_outerScaleMultiplier, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseAmplitude", m_noiseAmplitude, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseFrequency", m_noiseFrequency, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseOctaves", m_noiseOctaves, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lightProfilePath", m_lightProfilePath, "", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataHullSpotlightSetItem );
const Be::ClassInfo* EveSOFDataHullSpotlightSetItem::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullSpotlightSetItem, "" )
        MAP_INTERFACE( EveSOFDataHullSpotlightSetItem )

		MAP_ATTRIBUTE( "transform", m_transform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_boneIndex, ":jessica-widget: boneindex", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "colorType", m_colorType, "Used with SOF-6", Be::READWRITE | Be::PERSIST | Be::ENUM, SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser )
		MAP_ATTRIBUTE( "boosterGainInfluence", m_boosterGainInfluence, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "groupIndex", m_groupIndex, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "spriteScale", m_spriteScale, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "coneIntensity", m_coneIntensity, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "flareIntensity", m_flareIntensity, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "spriteIntensity", m_spriteIntensity, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "saturation", m_saturation, "Used with SOF-6", Be::READWRITE | Be::PERSIST)
		MAP_ATTRIBUTE( "light", m_light, "Used with SOF-6", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHullSpotlightSet );
const Be::ClassInfo* EveSOFDataHullSpotlightSet::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullSpotlightSet, "" )
        MAP_INTERFACE( EveSOFDataHullSpotlightSet )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "visibilityGroup", m_visibilityGroup, "Name for visibility group to toggle visibility for the whole set.\nUsed with SOF-6 \n:jessica-widget: visibilitygroup", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "skinned", m_skinned, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "zOffset", m_zOffset, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "coneTextureResPath", m_coneTextureResPath, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "glowTextureResPath", m_glowTextureResPath, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "items", m_items, "The items in this spotlightset", Be::READ | Be::PERSIST )
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
		MAP_ATTRIBUTE_WITH_CHOOSER( "colorType", m_colorType, "Used with SOF-6", Be::READWRITE | Be::PERSIST | Be::ENUM, SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser )
		MAP_ATTRIBUTE( "intensity", m_intensity, "Used with SOF-6", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "saturation", m_saturation, "Used with SOF-6", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer1Transform", m_layer1Transform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer1Scroll", m_layer1Scroll, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer2Transform", m_layer2Transform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer2Scroll", m_layer2Scroll, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_boneIndex, ":jessica-widget: boneindex", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "groupIndex", m_groupIndex, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maskMapAtlasIndex", m_maskMapAtlasIndex, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "blinkRate", m_rate, "rate (Hz)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "blinkPhase", m_phase, "phase (0.-1.)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "blinkMode", m_blinkMode, "", Be::READWRITE | Be::PERSIST | Be::ENUM, EveSOFDataPlaneSetBlinkTypeChooser )
		MAP_ATTRIBUTE( "lights", m_lights, "Used with SOF-6", Be::READ | Be::PERSIST )

	EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataHullPlaneSet );

Be::VarChooser EveSOFDataHullPlaneSetUsageChooser[] =
{
	{ "Standard", BeCast( EveSOFDataHullPlaneSet::USAGE_STANDARD ), "Standard planeset" },
	{ "SpaceVideo", BeCast( EveSOFDataHullPlaneSet::USAGE_SPACE_VIDEO ), "Space Video planeset" },
	{ "HangarVideo", BeCast( EveSOFDataHullPlaneSet::USAGE_HANGAR_VIDEO ), "Hangar Video planeset" },
	{ "Haze", BeCast( EveSOFDataHullPlaneSet::USAGE_HAZE ), "Fake haze planeset" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "HullPlanesetUsage", EveSOFDataHullPlaneSet::Usage, EveSOFDataHullPlaneSetUsageChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* EveSOFDataHullPlaneSet::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullPlaneSet, "" )
        MAP_INTERFACE( EveSOFDataHullPlaneSet )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "visibilityGroup", m_visibilityGroup, "Name for visibility group to toggle visibility for the whole set.\nUsed with SOF-6 \n:jessica-widget: visibilitygroup", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer1MapResPath", m_layer1MapResPath, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "layer2MapResPath", m_layer2MapResPath, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maskMapResPath", m_maskMapResPath, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER("usage", m_usage, "Choose the usage of this planeSet", Be::READWRITE | Be::PERSIST | Be::ENUM, EveSOFDataHullPlaneSetUsageChooser )
		MAP_ATTRIBUTE( "atlasSize", m_atlasSize, "Specifies the uniform division of a texture atlas into a square grid of chunks.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "atlasAspectRatio", m_atlasAspectRatio, "Adjusts the chunk sizes within the atlas for non-uniform X and Y dimensions.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "skinned", m_skinned, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "items", m_items, "The items in this planeset", Be::READ | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataFactionVisibilityGroupSet );
const Be::ClassInfo* EveSOFDataFactionVisibilityGroupSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataFactionVisibilityGroupSet, "" )
		MAP_INTERFACE( EveSOFDataFactionVisibilityGroupSet )

		MAP_ATTRIBUTE( "visibilityGroups", m_visibilityGroups, "", Be::READ | Be::PERSIST )
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
		MAP_ATTRIBUTE( "intensity", m_intensity, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_boneIndex, ":jessica-widget: boneindex", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "colorType", m_colorType, "", Be::READWRITE | Be::PERSIST | Be::ENUM, SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser )
		MAP_ATTRIBUTE( "saturation", m_saturation, "Used with SOF-6", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "light", m_light, "Used with SOF-6", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHullSpriteSet );
const Be::ClassInfo* EveSOFDataHullSpriteSet::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullSpriteSet, "" )
        MAP_INTERFACE( EveSOFDataHullSpriteSet )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "skinned", m_skinned, "Is this spriteset bone-animated.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "visibilityGroup", m_visibilityGroup, "Name for visibility group to toggle visibility for the whole set.\n:jessica-widget: visibilitygroup", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "items", m_items, "The items in this spriteset", Be::READ | Be::PERSIST )
    EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHullSpriteLineSetItem );
const Be::ClassInfo* EveSOFDataHullSpriteLineSetItem::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataHullSpriteLineSetItem, "" )
		MAP_INTERFACE( EveSOFDataHullSpriteLineSetItem )

		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "spacing", m_spacing, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "blinkRate", m_blinkRate, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "blinkPhase", m_blinkPhase, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "blinkPhaseShift", m_blinkPhaseShift, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minScale", m_minScale, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxScale", m_maxScale, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "falloff", m_falloff, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "intensity", m_intensity, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_boneIndex, ":jessica-widget: boneindex", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isCircle", m_isCircle, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "colorType", m_colorType, "", Be::READWRITE | Be::PERSIST | Be::ENUM, SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser )
		MAP_ATTRIBUTE( "saturation", m_saturation, "Used with SOF-6", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "light", m_light, "Used with SOF-6", Be::READWRITE | Be::PERSIST)
		EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHullSpriteLineSet );
const Be::ClassInfo* EveSOFDataHullSpriteLineSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataHullSpriteLineSet, "" )
		MAP_INTERFACE( EveSOFDataHullSpriteLineSet )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "skinned", m_skinned, "Is this spriteset bone-animated.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "visibilityGroup", m_visibilityGroup, "Name for visibility group to toggle visibility for the whole set.\n:jessica-widget: visibilitygroup", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "items", m_items, "The items in this spritelineset", Be::READ | Be::PERSIST )
		EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHullHazeSetItem );
const Be::ClassInfo* EveSOFDataHullHazeSetItem::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataHullHazeSetItem, "" )
		MAP_INTERFACE( EveSOFDataHullHazeSetItem )

		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_boneIndex, ":jessica-widget: boneindex", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "colorType", m_colorType, "", Be::READWRITE | Be::PERSIST | Be::ENUM, SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser )
		MAP_ATTRIBUTE( "saturation", m_saturation, "Used with SOF-6", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hazeBrightness", m_hazeBrightness, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hazeFalloff", m_hazeFalloff, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sourceBrightness", m_sourceBrightness, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sourceSize", m_sourceSize, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boosterGainInfluence", m_boosterGainInfluence, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lights", m_lights, "Used with SOF-6", Be::READ | Be::PERSIST )

		EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHullHazeSet );
const Be::ClassInfo* EveSOFDataHullHazeSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataHullHazeSet, "" )
		MAP_INTERFACE( EveSOFDataHullHazeSet )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "hazeType", m_hazeType, "", Be::READWRITE | Be::PERSIST | Be::ENUM, EveSOFDataHazeTypeChooser )
		MAP_ATTRIBUTE( "visibilityGroup", m_visibilityGroup, "Name for visibility group to toggle visibility for the whole set.\n:jessica-widget: visibilitygroup", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "skinned", m_skinned, "Is this hazeset bone-animated.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "items", m_items, "The items in this hazeset", Be::READ | Be::PERSIST )
		EXPOSURE_END()
}

Be::VarChooser EveSOFDataHullBannerUsageChooser[] =
{
	{ "AllianceLogo", BeCast( EveSOFDataHullBanner::ALLIANCE_LOGO ), "Alliance logo" },
	{ "CorpLogo", BeCast( EveSOFDataHullBanner::CORP_LOGO ), "Corporation logo" },
	{ "CeoPortrait", BeCast( EveSOFDataHullBanner::CEO_PORTRAIT ), "Ceo portrait" },
	{ "VerticalBanner", BeCast( EveSOFDataHullBanner::VERTICAL_BANNER ), "Vertical banner" },
	{ "HorizontalBanner", BeCast( EveSOFDataHullBanner::HORIZONTAL_BANNER ), "Vertical banner" },

	{ "TargetSystemAllianceLogo", BeCast( EveSOFDataHullBanner::TARGET_SYSTEM_ALLIANCE_LOGO ), "Target system alliance logo (gates)" },
	{ "TargetSystemVerticalBanner", BeCast( EveSOFDataHullBanner::TARGET_SYSTEM_VERTICAL_BANNER ), "Target system vertical banner (gates)" },
	{ "TargetSystemHorizontalBanner", BeCast( EveSOFDataHullBanner::TARGET_SYSTEM_HORIZONTAL_BANNER ), "Target system horizontal banner (gates)" },
	{ "TargetSystemInfo0", BeCast( EveSOFDataHullBanner::TARGET_SYSTEM_INFO_0 ), "Target system information (gates)" },
	{ "TargetSystemInfo1", BeCast( EveSOFDataHullBanner::TARGET_SYSTEM_INFO_1 ), "Target system information (gates)" },
	{ "TargetSystemInfo2", BeCast( EveSOFDataHullBanner::TARGET_SYSTEM_INFO_2 ), "Target system information (gates)" },
	{ "TargetSystemInfo3", BeCast( EveSOFDataHullBanner::TARGET_SYSTEM_INFO_3 ), "Target system information (gates)" },
	{ "TargetSystemInfo4", BeCast( EveSOFDataHullBanner::TARGET_SYSTEM_INFO_4 ), "Target system information (gates)" },
	{ "TargetSystemStatus", BeCast( EveSOFDataHullBanner::TARGET_SYSTEM_STATUS ), "Target system status (gates)" },
	{ "CurrentSystemAllianceLogo", BeCast( EveSOFDataHullBanner::CURRENT_SYSTEM_ALLIANCE_LOGO ), "Current system alliance logo (gates)" },
	{ "CurrentSystemVerticalBanner", BeCast( EveSOFDataHullBanner::CURRENT_SYSTEM_VERTICAL_BANNER ), "Current system vertical banner (gates)" },
	{ "CurrentSystemHorizontalBanner", BeCast( EveSOFDataHullBanner::CURRENT_SYSTEM_HORIZONTAL_BANNER ), "Current system horizontal banner (gates)" },
	{ "PublicityPoster", BeCast( EveSOFDataHullBanner::PUBLICITY_POSTER ), "Publicity structure poster" },
	{ "PublicityPortrait", BeCast( EveSOFDataHullBanner::PUBLICITY_PORTRAIT ), "Publicity structure portrait" },
	{ "RecruitmentInformation0", BeCast( EveSOFDataHullBanner::RECRUITMENT_INFORMATION_0 ), "Publicity structure recruitment information" },
	{ "RecruitmentInformation1", BeCast( EveSOFDataHullBanner::RECRUITMENT_INFORMATION_1 ), "Publicity structure recruitment information" },
	{ "RecruitmentInformation2", BeCast( EveSOFDataHullBanner::RECRUITMENT_INFORMATION_2 ), "Publicity structure recruitment information" },
	{ "RecruitmentInformation3", BeCast( EveSOFDataHullBanner::RECRUITMENT_INFORMATION_3 ), "Publicity structure recruitment information" },
	{ "RecruitmentInformation4", BeCast( EveSOFDataHullBanner::RECRUITMENT_INFORMATION_4 ), "Publicity structure recruitment information" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "HullBannerUsage", EveSOFDataHullBanner::Usage, EveSOFDataHullBannerUsageChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

// USED FOR SOF PHASE-6, PLEASE KEEP UP TO DATE AND MAKE IT MATCH THE LIST ABOVE!
Be::VarChooser EveSOFDataHullBannerSetItemUsageChooser[] = {
	{ "AllianceLogo", BeCast( EveSOFDataHullBannerSetItem::ALLIANCE_LOGO ), "Alliance logo" },
	{ "CorpLogo", BeCast( EveSOFDataHullBannerSetItem::CORP_LOGO ), "Corporation logo" },
	{ "CeoPortrait", BeCast( EveSOFDataHullBannerSetItem::CEO_PORTRAIT ), "Ceo portrait" },
	{ "VerticalBanner", BeCast( EveSOFDataHullBannerSetItem::VERTICAL_BANNER ), "Vertical banner" },
	{ "HorizontalBanner", BeCast( EveSOFDataHullBannerSetItem::HORIZONTAL_BANNER ), "Vertical banner" },

	{ "TargetSystemAllianceLogo", BeCast( EveSOFDataHullBannerSetItem::TARGET_SYSTEM_ALLIANCE_LOGO ), "Target system alliance logo (gates)" },
	{ "TargetSystemVerticalBanner", BeCast( EveSOFDataHullBannerSetItem::TARGET_SYSTEM_VERTICAL_BANNER ), "Target system vertical banner (gates)" },
	{ "TargetSystemHorizontalBanner", BeCast( EveSOFDataHullBannerSetItem::TARGET_SYSTEM_HORIZONTAL_BANNER ), "Target system horizontal banner (gates)" },
	{ "TargetSystemInfo0", BeCast( EveSOFDataHullBannerSetItem::TARGET_SYSTEM_INFO_0 ), "Target system information (gates)" },
	{ "TargetSystemInfo1", BeCast( EveSOFDataHullBannerSetItem::TARGET_SYSTEM_INFO_1 ), "Target system information (gates)" },
	{ "TargetSystemInfo2", BeCast( EveSOFDataHullBannerSetItem::TARGET_SYSTEM_INFO_2 ), "Target system information (gates)" },
	{ "TargetSystemInfo3", BeCast( EveSOFDataHullBannerSetItem::TARGET_SYSTEM_INFO_3 ), "Target system information (gates)" },
	{ "TargetSystemInfo4", BeCast( EveSOFDataHullBannerSetItem::TARGET_SYSTEM_INFO_4 ), "Target system information (gates)" },
	{ "TargetSystemStatus", BeCast( EveSOFDataHullBannerSetItem::TARGET_SYSTEM_STATUS ), "Target system status (gates)" },
	{ "CurrentSystemAllianceLogo", BeCast( EveSOFDataHullBannerSetItem::CURRENT_SYSTEM_ALLIANCE_LOGO ), "Current system alliance logo (gates)" },
	{ "CurrentSystemVerticalBanner", BeCast( EveSOFDataHullBannerSetItem::CURRENT_SYSTEM_VERTICAL_BANNER ), "Current system vertical banner (gates)" },
	{ "CurrentSystemHorizontalBanner", BeCast( EveSOFDataHullBannerSetItem::CURRENT_SYSTEM_HORIZONTAL_BANNER ), "Current system horizontal banner (gates)" },
	{ "PublicityPoster", BeCast( EveSOFDataHullBannerSetItem::PUBLICITY_POSTER ), "Publicity structure poster" },
	{ "PublicityPortrait", BeCast( EveSOFDataHullBannerSetItem::PUBLICITY_PORTRAIT ), "Publicity structure portrait" },
	{ "RecruitmentInformation0", BeCast( EveSOFDataHullBannerSetItem::RECRUITMENT_INFORMATION_0 ), "Publicity structure recruitment information" },
	{ "RecruitmentInformation1", BeCast( EveSOFDataHullBannerSetItem::RECRUITMENT_INFORMATION_1 ), "Publicity structure recruitment information" },
	{ "RecruitmentInformation2", BeCast( EveSOFDataHullBannerSetItem::RECRUITMENT_INFORMATION_2 ), "Publicity structure recruitment information" },
	{ "RecruitmentInformation3", BeCast( EveSOFDataHullBannerSetItem::RECRUITMENT_INFORMATION_3 ), "Publicity structure recruitment information" },
	{ "RecruitmentInformation4", BeCast( EveSOFDataHullBannerSetItem::RECRUITMENT_INFORMATION_4 ), "Publicity structure recruitment information" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "HullBannerSetItemUsage", EveSOFDataHullBannerSetItem::Usage, EveSOFDataHullBannerSetItemUsageChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

BLUE_DEFINE( EveSOFDataHullBannerLight );
const Be::ClassInfo* EveSOFDataHullBannerLight::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataHullBannerLight, "" )
		MAP_ATTRIBUTE(
			"brightness",
			m_brightness,
			"A multiplier for the light's brightness\n"
			":jessica-group: Light",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"radiusMultiplier",
			m_radiusMultiplier,
			"Multiplier for the size of the light \n"
			":jessica-group: Light",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"innerRadiusMultiplier",
			m_innerRadiusMultiplier,
			"A multiplier for the light's inner radius based on it's scaling mod (default is 1/3 of the outer radius)\n"
			":jessica-group: Light",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"noiseAmplitude",
			m_noiseAmplitude,
			"Brightness noise amplitude\n"
			":jessica-group: Noise",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"noiseFrequency",
			m_noiseFrequency,
			"Brightness noise frequency\n"
			":jessica-group: Noise",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"noiceOctaves",
			m_noiseOctaves,
			"Brightness turbulance octives\n"
			":jessica-group: Noise",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"saturation",
			m_saturation,
			"color saturation for banner lights \n"
			":jessica-group: ColorMix",
			Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataHullBanner );
const Be::ClassInfo* EveSOFDataHullBanner::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataHullBanner, "" )
		MAP_INTERFACE( EveSOFDataHullBanner )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "usage", m_usage, "Banner usage", Be::READWRITE | Be::PERSIST | Be::ENUM, EveSOFDataHullBannerUsageChooser )
		MAP_ATTRIBUTE( "visibilityGroup", m_visibilityGroup, "Name for visibility group to toggle visibility\n:jessica-widget: visibilitygroup", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::PERSISTONLY )
		MAP_PROPERTY( "scaling", GetScaling, SetScaling, "" )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "angleX", m_angleX, "", Be::PERSISTONLY )
		MAP_PROPERTY( "angleX", GetAngleX, SetAngleX, "Horizontal curve angle" )
		MAP_ATTRIBUTE( "angleY", m_angleY, "", Be::PERSISTONLY )
		MAP_PROPERTY( "angleY", GetAngleY, SetAngleY, "Vertical curve angle" )

		MAP_ATTRIBUTE( "lightOverride", m_lightOverride, "", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "boneIndex", m_boneIndex, ":jessica-widget: boneindex", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "maintainAspectRatio", m_maintainAspectRatio, "Maintain UV aspect ratio when manipulating a banner", Be::READWRITE )
	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataHullBannerSet );
const Be::ClassInfo* EveSOFDataHullBannerSet::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataHullBannerSet, "" )
		MAP_INTERFACE( EveSOFDataHullBannerSet )
		MAP_PROPERTY_READONLY( "name", GetName, "")
		MAP_ATTRIBUTE( "visibilityGroup", m_visibilityGroup, ":jessica-widget: visibilitygroup", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "banners", m_banners, "", Be::READ | Be::PERSIST )
	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataHullBannerSetItem );
const Be::ClassInfo* EveSOFDataHullBannerSetItem::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataHullBannerSetItem, "" )
		MAP_INTERFACE( EveSOFDataHullBannerSetItem )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "usage", m_usage, "Banner usage", Be::READWRITE | Be::PERSIST | Be::ENUM, EveSOFDataHullBannerSetItemUsageChooser )
		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::PERSISTONLY )
		MAP_PROPERTY( "scaling", GetScaling, SetScaling, "" )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "angleX", m_angleX, "", Be::PERSISTONLY )
		MAP_PROPERTY( "angleX", GetAngleX, SetAngleX, "Horizontal curve angle" )
		MAP_ATTRIBUTE( "angleY", m_angleY, "", Be::PERSISTONLY )
		MAP_PROPERTY( "angleY", GetAngleY, SetAngleY, "Vertical curve angle" )
		MAP_ATTRIBUTE( "light", m_light, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "boneIndex", m_boneIndex, ":jessica-widget: boneindex", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "maintainAspectRatio", m_maintainAspectRatio, "Maintain UV aspect ratio when manipulating a banner", Be::READWRITE )
	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataHullBooster );
const Be::ClassInfo* EveSOFDataHullBooster::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullBooster, "" )
        MAP_INTERFACE( EveSOFDataHullBooster )

		MAP_ATTRIBUTE( "alwaysOn", m_alwaysOn, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hasTrails", m_hasTrails, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "items", m_items, "", Be::READ | Be::PERSIST )
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
		MAP_ATTRIBUTE( "atlasIndex0", m_atlasIndex0, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "atlasIndex1", m_atlasIndex1, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lightScale", m_lightScale, "", Be::READWRITE | Be::PERSIST )
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
		MAP_ATTRIBUTE( "blockedMaterials", m_blockedMaterials, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "areaType", m_areaType, "na", Be::READWRITE | Be::PERSIST | Be::ENUM, EveSOFDataAreaTypeChooser )
		MAP_ATTRIBUTE( "textures", m_textures, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "parameters", m_parameters, "", Be::READ | Be::PERSIST )
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

BLUE_DEFINE_INTERFACE( IEveSOFDataHullLocatorSet );

BLUE_DEFINE( EveSOFDataHullLocatorSet );
const Be::ClassInfo* EveSOFDataHullLocatorSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataHullLocatorSet, "" )
		MAP_INTERFACE( EveSOFDataHullLocatorSet )
		MAP_INTERFACE( IEveSOFDataHullLocatorSet )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "locators", m_locators, "", Be::READ | Be::PERSIST )
	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataHullLocatorSetGroup );
const Be::ClassInfo* EveSOFDataHullLocatorSetGroup::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataHullLocatorSetGroup, ":jessica-icon: far-folder-tree" )
		MAP_INTERFACE( EveSOFDataHullLocatorSetGroup )
		MAP_INTERFACE( IEveSOFDataHullLocatorSet )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "locatorSets", m_locatorSets, "", Be::READ | Be::PERSIST )
	EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataTransform );
const Be::ClassInfo* EveSOFDataTransform::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataTransform, "" )
        MAP_INTERFACE( EveSOFDataTransform )

		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_boneIndex, ":jessica-widget: boneindex", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataHullChildSet );
const Be::ClassInfo* EveSOFDataHullChildSet::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataHullChildSet, "" )
		MAP_INTERFACE( EveSOFDataHullChildSet )

		MAP_PROPERTY_READONLY( "name", GetName, "" )
		MAP_ATTRIBUTE( "visibilityGroup", m_visibilityGroup, ":jessica-widget: visibilitygroup", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "items", m_items, "", Be::READ | Be::PERSIST )
	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataHullChildSetItem );
const Be::ClassInfo* EveSOFDataHullChildSetItem::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataHullChildSetItem, "" )
		MAP_INTERFACE( EveSOFDataHullChildSetItem )
		MAP_PROPERTY_READONLY( "name", GetName, "" )
		MAP_ATTRIBUTE( "redFilePath", m_redFilePath, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lowestLodVisible", m_lowestLodVisible, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "buildFilter", m_buildFilter, "", Be::READWRITE | Be::PERSIST, EveSOFDataHullBuildFilterChooser )
	EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataHullChild );
const Be::ClassInfo* EveSOFDataHullChild::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHullChild, "" )
        MAP_INTERFACE( EveSOFDataHullChild )

		MAP_PROPERTY_READONLY( "name", GetName, "" )
		MAP_ATTRIBUTE( "redFilePath", m_redFilePath, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lowestLodVisible", m_lowestLodVisible, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "id", m_id, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "groupIndex", m_groupIndex, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "buildFilter", m_buildFilter, "", Be::READWRITE | Be::PERSIST, EveSOFDataHullBuildFilterChooser )
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



BLUE_DEFINE( EveSOFDataHullController );
const Be::ClassInfo* EveSOFDataHullController::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataHullController, "" )
		MAP_INTERFACE( EveSOFDataHullController )

		MAP_PROPERTY_READONLY( "name", GetName, "" )
		MAP_ATTRIBUTE(
			"path",
			m_path,
			"Path to the red file for the controller\n"
			":jessica-widget: filepath\n"
			":jessica-file-filter : redfile",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "buildFilter", m_buildFilter, "", Be::READWRITE | Be::PERSIST, EveSOFDataHullBuildFilterChooser )
	EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataHullSoundEmitter );
const Be::ClassInfo* EveSOFDataHullSoundEmitter::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataHullSoundEmitter, "" )
		MAP_INTERFACE( EveSOFDataHullSoundEmitter )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "prefix", m_prefix, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( 
			"attenuationScalingFactor", 
			m_attenuationScalingFactor, 
			"The attenuation scaling factor when this audio emitter is initially spawned.", 
			Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}



BLUE_DEFINE( EveSOFDataHull );

Be::VarChooser EveSOFBuildClassChooser[] =
{
	{	"EveShip2", BeCast( EveSOFDataHull::BUILDCLASS_SHIP ), "Build an EveShip2" },
	{	"EveMobile", BeCast( EveSOFDataHull::BUILDCLASS_MOBILE ), "Build an EveMobile" },
	{	"EveStation2", BeCast( EveSOFDataHull::BUILDCLASS_STATIONARY ), "Build an EveStation2" },
	{	"EveSwarm", BeCast( EveSOFDataHull::BUILDCLASS_SWARM ), "Build an EveSwarm" },
	{	"Extension", BeCast( EveSOFDataHull::BUILDCLASS_EXTENSION), "Build an EveEffectRoot with a child" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "BuildClass", EveSOFDataHull::BuildClass, EveSOFBuildClassChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

Be::VarChooser EveSOFImpactEffectTypeChooser[] =
{
	{ "Nothing", BeCast( EveSOFDataHull::IMPACTEFFECT_NONE ), "No impact effects" },
	{ "Ellipsoid", BeCast( EveSOFDataHull::IMPACTEFFECT_ELLIPSOID ), "Use ellipsoid for shield" },
	{ "Hull", BeCast( EveSOFDataHull::IMPACTEFFECT_HULL), "Use ellipsoid for hull" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "ImpactEffectType", EveSOFDataHull::ImpactEffectType, EveSOFImpactEffectTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* EveSOFDataHull::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveSOFDataHull, "" )
        MAP_INTERFACE( EveSOFDataHull )

		MAP_ATTRIBUTE( "name", m_name, "The hull name, eg cb2_t1. This functions as an ID.", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "sof6", m_sof6, "Use SOF-6 features", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "description", m_description, "A description string. NOT used by the SOF, it's just for debugging purposes.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "category", m_category, "A category string. NOT used by the SOF, it's for tool validation.\n:jessica-widget: hullcategory", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE_WITH_CHOOSER("buildClass", m_buildClass, "Choose the output trinity class", Be::READWRITE | Be::PERSIST | Be::ENUM, EveSOFBuildClassChooser )

		MAP_ATTRIBUTE( "geometryResFilePath", m_geometryResFilePath, "The res file path to the gr2 file", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boundingSphere", m_boundingSphere, "The actual size of the geometry", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shapeEllipsoidCenter", m_shapeEllipsoidCenter, "The geometry's shape ellipsoid: center", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "shapeEllipsoidRadius", m_shapeEllipsoidRadius, "The geometry's shape ellipsoid: center", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isSkinned", m_isSkinned, "Does this hull need skinned shaders?", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "enableDynamicBoundingSphere", m_enableDynamicBoundingSphere, "Used to toggle dynamic bounding sphere.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "castShadow", m_castShadow, "Used to toggle shadow casting.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "opaqueAreas", m_opaqueAreas, "The opaque areas on this mesh", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "decalAreas", m_decalAreas, "The decal aSOFDatareas on this mesh", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "transparentAreas", m_transparentAreas, "The transparent areas on this mesh", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "additiveAreas", m_additiveAreas, "The additive areas on this mesh", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "distortionAreas", m_distortionAreas, "The distortion areas on this mesh", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "defaultPattern", m_defaultPattern, "The default pattern projection data for this hull", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "spriteSets", m_spriteSets, "The spritesets", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "spotlightSets", m_spotlightSets, "The spotlightsets", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "planeSets", m_planeSets, "The planesets", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "spriteLineSets", m_spriteLineSets, "The spritelinesets", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "hazeSets", m_hazeSets, "The hazesets", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "banners", m_banners, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "bannerSets", m_bannerSets, "The bannerSets\nUsed with SOF-6", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "decalSets", m_decalSets, "The decalsets", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "lightSets", m_lightSets, "The lightSets", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "impactEffectType", m_impactEffectType, "Type of impact effect on this hull", Be::READWRITE | Be::PERSIST | Be::ENUM, EveSOFImpactEffectTypeChooser )

		MAP_ATTRIBUTE( "booster", m_booster, "The booster", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "audioPosition", m_audioPosition, "The audio position", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "children", m_children, "List of children", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "childSets", m_childSets, "Set of child effects\nUsed with SOF-6", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "soundEmitters", m_soundEmitters, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "controllers", m_controllers, "List of controller references", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "animations", m_animations, "List of animations", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "instancedMeshes", m_instancedMeshes, "List of instanced meshes", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "locatorTurrets", m_locatorTurrets, "Turret locators", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "locatorSets", m_locatorSets, "Damage locators", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "modelRotationCurvePath", m_modelRotationCurvePath, "Model rotation curve path", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "modelTranslationCurvePath", m_modelTranslationCurvePath, "Model translation curve path", Be::READWRITE | Be::PERSIST )
    EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataHullDecalSetItem );

Be::VarChooser EveSOFDecalUsageChooser[] =
	{
		{ "Standard", BeCast( EveSOFDataHullDecalSetItem::USAGE_STANDARD ), "Standard decal" },
		{ "KillCounter", BeCast( EveSOFDataHullDecalSetItem::USAGE_KILLCOUNTER ), "The killcounter decal" },
		{ "Hole", BeCast( EveSOFDataHullDecalSetItem::USAGE_HOLE ), "Hole decal" },
		{ "Cylindrical", BeCast( EveSOFDataHullDecalSetItem::USAGE_CYLINDRICAL ), "Cylindrical decal" },
		{ "GlowCylindrical", BeCast( EveSOFDataHullDecalSetItem::USAGE_GLOWCYLINDRICAL ), "Glow cylindrical decal" },
		{ "Glow", BeCast( EveSOFDataHullDecalSetItem::USAGE_GLOWSTANDARD ), "Glow decal" },
		{ "Logo", BeCast( EveSOFDataHullDecalSetItem::USAGE_LOGO), "Logo decal" },
		{ 0 }
	};
BLUE_REGISTER_ENUM_EX( "DecalUsage", EveSOFDataHullDecalSetItem::Usage, EveSOFDecalUsageChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* EveSOFDataHullDecalSetItem::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataHullDecalSetItem, "" )
		MAP_INTERFACE( EveSOFDataHullDecalSetItem )
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "usage", m_usage, "Choose the usage of this decal", Be::READWRITE | Be::PERSIST | Be::ENUM, EveSOFDecalUsageChooser )
		MAP_ATTRIBUTE( "position", m_position, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_boneIndex, ":jessica-widget: boneindex", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "meshIndex", m_meshIndex, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "glowColorType", m_glowColorType, "", Be::READWRITE | Be::PERSIST | Be::ENUM, SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "logoType", m_logoType, "", Be::READWRITE | Be::PERSIST | Be::ENUM, EveSOFDataLogoSetTypeChooser )
		MAP_ATTRIBUTE( "parameters", m_parameters, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "textures", m_textures, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "indexBuffers", m_indexBuffers, "", Be::READ | Be::PERSIST )
	EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataHullLightSetItem );
const Be::ClassInfo* EveSOFDataHullLightSetItem::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataHullLightSetItem, "" )
		MAP_INTERFACE( EveSOFDataHullLightSetItem )
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "flags", m_data.flags, "Various light options", Be::READWRITE | Be::PERSIST, Tr2LightFlagChooser )
		MAP_ATTRIBUTE( "position", m_data.position, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_data.boneIndex, ":jessica-widget: boneindex", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "lightColor", m_data.lightColor, "", Be::READWRITE | Be::PERSIST | Be::ENUM, SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser )
		MAP_ATTRIBUTE( "radius", m_data.radius, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "innerRadius", m_data.innerRadius, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "brightness", m_data.brightness, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseAmplitude", m_data.noiseAmplitude, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseFrequency", m_data.noiseFrequency, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseOctaves", m_data.noiseOctaves, "", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}

BLUE_DEFINE( EveSOFDataHullLightSetTexturedPointLight );
const Be::ClassInfo* EveSOFDataHullLightSetTexturedPointLight::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataHullLightSetTexturedPointLight, "" )
		MAP_INTERFACE( EveSOFDataHullLightSetTexturedPointLight )
		MAP_INTERFACE( EveSOFDataHullLightSetItem )
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "flags", m_data.flags, "Various light options", Be::READWRITE | Be::PERSIST, Tr2LightFlagChooser )
		MAP_ATTRIBUTE( "position", m_data.position, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_data.boneIndex, ":jessica-widget: boneindex", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "radius", m_data.radius, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "innerRadius", m_data.innerRadius, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "brightness", m_data.brightness, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseAmplitude", m_data.noiseAmplitude, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseFrequency", m_data.noiseFrequency, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseOctaves", m_data.noiseOctaves, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "texturePath", m_data.texturePath, "", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataHullLightSetSpotLight );
const Be::ClassInfo* EveSOFDataHullLightSetSpotLight::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataHullLightSetSpotLight, "" )
		MAP_INTERFACE( EveSOFDataHullLightSetSpotLight )
		MAP_INTERFACE( EveSOFDataHullLightSetItem )
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "flags", m_data.flags, "Various light options", Be::READWRITE | Be::PERSIST, Tr2LightFlagChooser )
		MAP_ATTRIBUTE( "position", m_data.position, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_data.boneIndex, ":jessica-widget: boneindex", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_data.rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "lightColor", m_data.lightColor, "", Be::READWRITE | Be::PERSIST | Be::ENUM, SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser )
		MAP_ATTRIBUTE( "radius", m_data.radius, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "innerRadius", m_data.innerRadius, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "innerAngle", m_data.innerAngle, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "outerAngle", m_data.outerAngle, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "brightness", m_data.brightness, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseAmplitude", m_data.noiseAmplitude, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseFrequency", m_data.noiseFrequency, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseOctaves", m_data.noiseOctaves, "", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataLogoSet );
const Be::ClassInfo* EveSOFDataLogoSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataLogoSet, "" )
		MAP_INTERFACE( EveSOFDataLogoSet )

		MAP_ATTRIBUTE( EveSOFDataLogoSetTypeChooser[TYPE_PRIMARY].mKey, m_logos[TYPE_PRIMARY], ":jessica-group:Logos", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataLogoSetTypeChooser[TYPE_SECONDARY].mKey, m_logos[TYPE_SECONDARY], ":jessica-group:Logos", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataLogoSetTypeChooser[TYPE_TERTIARY].mKey, m_logos[TYPE_TERTIARY], ":jessica-group:Logos", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataLogoSetTypeChooser[TYPE_MARKING_01].mKey, m_logos[TYPE_MARKING_01], ":jessica-group:Logos", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataLogoSetTypeChooser[TYPE_MARKING_02].mKey, m_logos[TYPE_MARKING_02], ":jessica-group:Logos", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataBlinkType );
const Be::ClassInfo* EveSOFDataBlinkType::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataBlinkType, "" )
		MAP_INTERFACE( EveSOFDataBlinkType )

		MAP_ATTRIBUTE( EveSOFDataPlaneSetBlinkTypeChooser[TYPE_BLINK].mKey, m_blinkType[TYPE_BLINK], ":jessica-group:Blink", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataPlaneSetBlinkTypeChooser[TYPE_FADE_IN].mKey, m_blinkType[TYPE_FADE_IN], ":jessica-group:Blink", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataPlaneSetBlinkTypeChooser[TYPE_FADE_OUT].mKey, m_blinkType[TYPE_FADE_OUT], ":jessica-group:Blink", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataPlaneSetBlinkTypeChooser[TYPE_CYCLE].mKey, m_blinkType[TYPE_CYCLE], ":jessica-group:Blink", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}


BLUE_DEFINE( EveSOFDataPatternLayer );
Be::VarChooser EveSOFDataPatternLayerProjectionTypeChooser[] =
	{
		{	"Repeat", BeCast( EveSOFDataPatternLayer::PROJECTION_REPEAT ), "Repeat pattern texture projection" },
		{	"Clamp", BeCast( EveSOFDataPatternLayer::PROJECTION_CLAMP ), "Clamp the projection" },
		{ 	"Border", BeCast( EveSOFDataPatternLayer::PROJECTION_BORDER ), "Border the projection" },
		{ 0 }
	};
BLUE_REGISTER_ENUM_EX( "EveSOFDataPatternLayerProjectionType", EveSOFDataPatternLayer::ProjectionType, EveSOFDataPatternLayerProjectionTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );


BLUE_DEFINE( EveSOFDataPatternLayerProperties );
const Be::ClassInfo* EveSOFDataPatternLayerProperties::ExposeToBlue(){
	EXPOSURE_BEGIN( EveSOFDataPatternLayerProperties, "" )
		MAP_INTERFACE( EveSOFDataPatternLayerProperties )

		MAP_ATTRIBUTE_WITH_CHOOSER( "projectionTypeU", m_projectionTypeU, "Choose the type of texture projection in u direction\n :jessica-group: Projection Direction", Be::READWRITE | Be::PERSIST | Be::ENUM, EveSOFDataPatternLayerProjectionTypeChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "projectionTypeV", m_projectionTypeV, "Choose the type of texture projection in v direction\n :jessica-group: Projection Direction", Be::READWRITE | Be::PERSIST | Be::ENUM, EveSOFDataPatternLayerProjectionTypeChooser )
		MAP_ATTRIBUTE( "isTargetMtl1", m_isTargetMtl1, "This pattern goes onto material 1\n :jessica-group: Materials", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isTargetMtl2", m_isTargetMtl2, "This pattern goes onto material 2\n :jessica-group: Materials", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isTargetMtl3", m_isTargetMtl3, "This pattern goes onto material 3\n :jessica-group: Materials", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isTargetMtl4", m_isTargetMtl4, "This pattern goes onto material 4\n :jessica-group: Materials", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataAreaTypeChooser[EveSOFDataArea::AreaType::TYPE_PRIMARY].mKey, m_applicableAreas[EveSOFDataArea::AreaType::TYPE_PRIMARY], ":jessica-group: Applicable Areas", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataAreaTypeChooser[EveSOFDataArea::AreaType::TYPE_GLASS].mKey, m_applicableAreas[EveSOFDataArea::AreaType::TYPE_GLASS], ":jessica-group: Applicable Areas", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataAreaTypeChooser[EveSOFDataArea::AreaType::TYPE_SAILS].mKey, m_applicableAreas[EveSOFDataArea::AreaType::TYPE_SAILS], ":jessica-group: Applicable Areas", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataAreaTypeChooser[EveSOFDataArea::AreaType::TYPE_REACTOR].mKey, m_applicableAreas[EveSOFDataArea::AreaType::TYPE_REACTOR], ":jessica-group: Applicable Areas", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataAreaTypeChooser[EveSOFDataArea::AreaType::TYPE_DARKHULL].mKey, m_applicableAreas[EveSOFDataArea::AreaType::TYPE_DARKHULL], ":jessica-group: Applicable Areas", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataAreaTypeChooser[EveSOFDataArea::AreaType::TYPE_ROCK].mKey, m_applicableAreas[EveSOFDataArea::AreaType::TYPE_ROCK], ":jessica-group: Applicable Areas", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataAreaTypeChooser[EveSOFDataArea::AreaType::TYPE_MONUMENT].mKey, m_applicableAreas[EveSOFDataArea::AreaType::TYPE_MONUMENT], ":jessica-group: Applicable Areas", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataAreaTypeChooser[EveSOFDataArea::AreaType::TYPE_ORNAMENT].mKey, m_applicableAreas[EveSOFDataArea::AreaType::TYPE_ORNAMENT], ":jessica-group: Applicable Areas", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( EveSOFDataAreaTypeChooser[EveSOFDataArea::AreaType::TYPE_SIMPLEPRIMARY].mKey, m_applicableAreas[EveSOFDataArea::AreaType::TYPE_SIMPLEPRIMARY], ":jessica-group: Applicable Areas", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}

Be::VarChooser EveSOFDataPatternLayerMaterialSourceChooser[] =
	{
		{	"Material1", BeCast( EveSOFDataPatternLayer::SOURCE_MATERIAL1 ), "Base material #1" },
		{	"Material2", BeCast( EveSOFDataPatternLayer::SOURCE_MATERIAL2 ), "Base material #2" },
		{	"Material3", BeCast( EveSOFDataPatternLayer::SOURCE_MATERIAL3 ), "Base material #3" },
		{	"Material4", BeCast( EveSOFDataPatternLayer::SOURCE_MATERIAL4 ), "Base material #4" },
		{	"PatternMaterial1", BeCast( EveSOFDataPatternLayer::SOURCE_PATTERN1 ), "Pattern material 1" },
		{	"PatternMaterial2", BeCast( EveSOFDataPatternLayer::SOURCE_PATTERN2 ), "Pattern material 2" },
		{ 0 }
	};
BLUE_REGISTER_ENUM_EX( "EveSOFDataPatternLayerMaterialSource", EveSOFDataPatternLayer::MaterialSource, EveSOFDataPatternLayerMaterialSourceChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );


const Be::ClassInfo* EveSOFDataPatternLayer::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSOFDataPatternLayer, "" )
		MAP_INTERFACE( EveSOFDataPatternLayer )

		MAP_ATTRIBUTE( "textureName", m_textureName, "Texture name", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "textureResFilePath", m_textureResFilePath, "Texture resfile", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "projectionTypeU", m_projectionTypeU, "Choose the type of texture projection in u direction", Be::READWRITE | Be::PERSIST | Be::ENUM, EveSOFDataPatternLayerProjectionTypeChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "projectionTypeV", m_projectionTypeV, "Choose the type of texture projection in v direction", Be::READWRITE | Be::PERSIST | Be::ENUM, EveSOFDataPatternLayerProjectionTypeChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "materialSource", m_materialSource, "Choose the material source for the pattern", Be::READWRITE | Be::PERSIST | Be::ENUM, EveSOFDataPatternLayerMaterialSourceChooser )
		MAP_ATTRIBUTE( "isTargetMtl1", m_isTargetMtl1, "This pattern goes onto material 1", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isTargetMtl2", m_isTargetMtl2, "This pattern goes onto material 2", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isTargetMtl3", m_isTargetMtl3, "This pattern goes onto material 3", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isTargetMtl4", m_isTargetMtl4, "This pattern goes onto material 4", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
