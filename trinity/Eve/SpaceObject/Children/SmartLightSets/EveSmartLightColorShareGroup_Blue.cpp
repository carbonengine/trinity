// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightColorShareGroup.h"

BLUE_DEFINE( EveSmartLightColorShareGroup );

const Be::ClassInfo* EveSmartLightColorShareGroup::ExposeToBlue()
{

	EXPOSURE_BEGIN( EveSmartLightColorShareGroup, ":jessica-icon: folder-gear\n:jessica-icon-color: (118, 205, 255)\n" )
		MAP_INTERFACE( EveSmartLightColorShareGroup )
		MAP_INTERFACE( EveSmartLightBaseGroup )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IListNotify )
		MAP_INTERFACE( EveEntity )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "lightGroups", m_lightGroups, "list of lights and light-renderables", Be::READ | Be::PERSIST | Be::NOTIFY )

	EXPOSURE_CHAINTO( EveSmartLightBaseGroup )
}
