#include "StdAfx.h"
#include "SpawnDrones.h"

BLUE_DEFINE( SpawnDrones );

const Be::ClassInfo* SpawnDrones::ExposeToBlue()
{
	EXPOSURE_BEGIN( SpawnDrones, "" )
		MAP_INTERFACE( SpawnDrones )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE( "enabled", m_enabled, "Should this behavior be active", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "addByCount", m_addByCount, "If enabled behavior will spawn count of drones instead of count drones every x seconds", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "spawnPosition", m_spawnPosition, "Where should the drones spawn", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "seconds", m_seconds, "How many seconds until the next one spawns, set to -1 if you don't want any drones added", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "count", m_count, "How many drones should spawn", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "time", m_time, "Time left until next agent spawns", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "addOnGrid", m_addOnGrid, "If enabled behavior will spawn count of drones (xCount, yCount, zCount, distance) to a grid \n:jessica-group: Grid", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "regenerateDrones", m_regenerateDrones, "bind to this if you want to trigger a grid regeneration \n:jessica-group: Grid", Be::READWRITE )
		MAP_ATTRIBUTE( "gridInfo", m_gridInfo, "x = count of drones in x row, y = count of drones in y row, z = count of drones in z row, w = distance between models \n:jessica-group: Grid", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "gridSpacing", m_gridSpacing, "(x,y,z) spacing in between models \n:jessica-group: Grid", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "gridFullnessFactor", m_gridFullnessFactor, "[0-1] how full should the grid be, 0 being no drones and 1 being fully populated, -1 will randomize the percentage factor \n:jessica-group: Grid", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		

		MAP_METHOD_AND_WRAP( "gridToggleReset", GridToggleReset, "toggle to 'reset' grid spawning \n:jessica-placement: TOOLBAR\n:jessica-icon: far-bomb\n" )
	
		EXPOSURE_END()
}