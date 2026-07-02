// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Light.h"

BLUE_DEFINE_ABSTRACT( Tr2Light );

const Be::VarChooser PerLightShadowSettingChooser[] = {
	{ "Low", BeCast( PerLightShadowSetting::SHADOW_LOW ), "Light casts shadow when shadow quality is Low." },
	{ "High", BeCast( PerLightShadowSetting::SHADOW_HIGH ), "Light casts shadow when shadow quality is High." },
	{ "Raytraced", BeCast( PerLightShadowSetting::SHADOW_RAYTRACED ), "Light casts shadow when shadow quality is Raytraced." },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "PerLightShadowSetting", PerLightShadowSetting, PerLightShadowSettingChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::VarChooser Tr2LightFlagChooser[] = {
	{ "AFFECTS_SURFACES", BeCast( Tr2LightManager::FLAG_AFFECTS_SURFACES ), "Affects surfaces" },
	{ "AFFECTS_PARTICLES", BeCast( Tr2LightManager::FLAG_AFFECTS_PARTICLES ), "Affects lit particles" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "Tr2LightFlags", uint16_t, Tr2LightFlagChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::VarChooser LightFalloffTypeChooser[] = {
	{ "INVERSE", BeCast( LightFalloffType::INVERSE ), "Inverse distance falloff" },
	{ "INVERSE_SQUARE", BeCast( LightFalloffType::INVERSE_SQUARE ), "Inverse square distance falloff" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "LightFalloffType", LightFalloffType, LightFalloffTypeChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* Tr2Light::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Light, "" )
		MAP_INTERFACE( Tr2Light )

	EXPOSURE_END()
}