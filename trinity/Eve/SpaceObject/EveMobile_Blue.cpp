// Copyright © 2013 CCP ehf.

#include "StdAfx.h"
#include "EveMobile.h"

BLUE_DEFINE( EveMobile );

const Be::ClassInfo* EveMobile::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveMobile, "" )
		MAP_INTERFACE( EveMobile )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( IListNotify )

		MAP_ATTRIBUTE( "turretSets", m_turretSets, "a list of all the turret sets on this ship", Be::READ | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "ActiveTurretCount", m_activeTurretCount, "How many active turretSets are on the space object", Be::READ )

		MAP_METHOD_AND_WRAP(
			"GetTurretLocatorIndex",
			GetTurretLocatorIndex,
			"Get the index for the locator in the ship's locator-list used for a single turret\n"
			":param turretSetIdx: index of the turretSet in the ship's turretset list\n"
			":param slotIdx: index of the individual single turret in the turretset\n" )
		MAP_METHOD_AND_WRAP( "GetTurretLocatorCount", GetTurretLocatorCount, "Returns the turret locator count of locators and bones matching the correct naming scheme." )
		MAP_METHOD_AND_WRAP( "RebuildTurretPositions", RebuildTurretPositions, "Re-positions all the turrets on this ship" )



	EXPOSURE_CHAINTO( EveSpaceObject2 )
}
