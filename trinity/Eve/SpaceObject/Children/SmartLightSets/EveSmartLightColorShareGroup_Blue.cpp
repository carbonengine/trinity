#include "StdAfx.h"
#include "EveSmartLightColorShareGroup.h"

BLUE_DEFINE( EveSmartLightColorShareGroup );

const Be::ClassInfo* EveSmartLightColorShareGroup::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSmartLightColorShareGroup, "" )
		MAP_INTERFACE( EveSmartLightColorShareGroup )
		MAP_INTERFACE( EveSmartLightBaseGroup )
		MAP_INTERFACE( IListNotify )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "lightGroups", m_lightGroups, "list of lights and light-renderables", Be::READ | Be::PERSIST | Be::NOTIFY )

	EXPOSURE_CHAINTO( EveSmartLightBaseGroup )
}
