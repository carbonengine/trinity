#include "StdAfx.h"
#include "SpawnDrones.h"

BLUE_DEFINE( SpawnDrones );

const Be::ClassInfo* SpawnDrones::ExposeToBlue()
{
	EXPOSURE_BEGIN( SpawnDrones, "" )
		MAP_INTERFACE( SpawnDrones )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE( "spawnPosition", m_spawnPosition, "SpawnDrones", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "seconds", m_seconds, "SpawnDrones", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "time", m_time, "SpawnDrones", Be::READ | Be::PERSIST )

	EXPOSURE_END()
}