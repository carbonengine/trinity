// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveChildSmartLightSet.h"

BLUE_DEFINE( EveChildSmartLightSet );

const Be::ClassInfo* EveChildSmartLightSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildSmartLightSet, "\n:jessica-icon: lightbulb-gear\n:jessica-icon-color: (255, 207, 50)\n:jessica-help-url: https://ccpgames.atlassian.net/wiki/spaces/TTL/pages/1308393505/SmartLightSets\n" )
		MAP_INTERFACE( EveChildSmartLightSet )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( ITr2DebugRenderable )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IListNotify )
		MAP_INTERFACE( IEveInheritPropertiesOwner )
		MAP_INTERFACE( EveEntity )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "distribution", m_distribution, "distributionMethod for entities\n:jessica-icon: map-location-dot\n:jessica-help-url: https://ccpgames.atlassian.net/wiki/spaces/TTL/pages/1311441160/Distributions\n", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "lightGroups", m_lightGroups, "list of lights and light-renderables", Be::READ | Be::PERSIST )

	EXPOSURE_END()
}
