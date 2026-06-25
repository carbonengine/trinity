// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightSpotLight.h"

BLUE_DEFINE( EveSmartLightSpotLight );

const Be::ClassInfo* EveSmartLightSpotLight::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSmartLightSpotLight, ":jessica-icon: lamp-desk\n" )
		MAP_INTERFACE( EveSmartLightSpotLight )

		MAP_ATTRIBUTE( "innerAngle", m_lightGroupData.innerAngle, "Inner angle of the spotlight (in degrees)\n:jessica-group: SpotLightOptions", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "outerAngle", m_lightGroupData.outerAngle, "Outer angle of the spotlight (in degrees)\n:jessica-group: SpotLightOptions", Be::READWRITE | Be::PERSIST )

	EXPOSURE_CHAINTO( EveSmartLightPointLight )
}
