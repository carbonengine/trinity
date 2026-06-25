// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightPointLight.h"

BLUE_DEFINE_INTERFACE( IEveSmartLightGroup );

BLUE_DEFINE( EveSmartLightPointLight );

const Be::ClassInfo* EveSmartLightPointLight::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSmartLightPointLight, ":jessica-icon: brightness\n" )
		MAP_INTERFACE( EveSmartLightPointLight )
		MAP_INTERFACE( EveSmartLightBaseGroup )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IListNotify )
		MAP_INTERFACE( EveEntity )


		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE_WITH_CHOOSER( "flags", m_lightGroupData.flags, "Various light options", Be::READWRITE | Be::PERSIST, Tr2LightFlagChooser )

		MAP_ATTRIBUTE( "radius", m_lightGroupData.radius, "Light radius", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "innerRadius", m_lightGroupData.innerRadius, "Inner light radius (to mimick a glowing sphere)", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "brightness", m_lightGroupData.brightness, "Light brightness (modulates color) for easier animation", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "lightProfilePath", m_lightProfilePath, "Path to IES profile\n:jessica-widget: filepath\n:jessica-file-filter: ies", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "lightProfile", m_lightProfile, ":jessica-hidden: True", Be::READ )

		MAP_ATTRIBUTE( "staticOffsetTranslation", m_staticOffsetTranslation, "static per instance offset in local space before placement \n:jessica-group: StaticOffsetFromDistribution", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "staticOffsetRotation", m_staticOffsetRotation, "static per instance rotation in local space before placement \n:jessica-group: StaticOffsetFromDistribution", Be::READWRITE | Be::PERSIST )

	EXPOSURE_CHAINTO( EveSmartLightBaseGroup )
}
