// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Light.h"

BLUE_DEFINE_ABSTRACT( Tr2Light );

const Be::VarChooser PerLightShadowSettingChooser[] = {
	{ "Disabled", BeCast( PerLightShadowSetting::DISABLED ), "Light does not cast shadow." },
	{ "Enabled Only On High/Raytraced Shadow Quality Settings", BeCast( PerLightShadowSetting::ENABLED_ONLY_ON_HIGH_QUALITY ), "Light only casts shadow when Shadow Quality is set to High or Raytraced." },
	{ "Enabled", BeCast( PerLightShadowSetting::ALWAYS_ENABLED ), "Light casts shadow regardless of Shadow Quality Setting" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "PerLightShadowSetting", PerLightShadowSetting, PerLightShadowSettingChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::VarChooser Tr2LightFlagChooser[] = {
	{ "AFFECTS_SURFACES", BeCast( Tr2LightManager::FLAG_AFFECTS_SURFACES ), "Affects surfaces" },
	{ "AFFECTS_PARTICLES", BeCast( Tr2LightManager::FLAG_AFFECTS_PARTICLES ), "Affects lit particles" },
	{ 0 }
};
BLUE_REGISTER_ENUM_EX( "Tr2LightFlags", uint16_t, Tr2LightFlagChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* Tr2Light::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Light, "" )
		MAP_INTERFACE( Tr2Light )

	EXPOSURE_END()
}