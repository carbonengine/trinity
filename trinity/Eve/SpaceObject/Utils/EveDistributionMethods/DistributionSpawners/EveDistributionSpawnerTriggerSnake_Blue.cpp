// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionSpawnerTriggerSnake.h"

BLUE_DEFINE( EveDistributionSpawnerTriggerSnake );

const Be::ClassInfo* EveDistributionSpawnerTriggerSnake::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistributionSpawnerTriggerSnake, ":jessica-icon: snake\n" )
		MAP_INTERFACE( EveDistributionSpawnerTriggerSnake )
		MAP_INTERFACE( IEveDistributionSpawner )

		MAP_ATTRIBUTE( "totalDestinations", m_totalDestinations, "how many travelPoints?", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "destinationsReached", m_numDestinationsReached, "how many have been triggered", Be::READ )
		MAP_ATTRIBUTE( "minBaseTimeBetweenTriggers", m_minTimeBetweenTriggers, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxBaseTimeBetweenTriggers", m_maxTimeBetweenTriggers, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "distanceToTravelTimeMultiplier", m_distanceToTravelTimeMultiplier, "sec per 100 meters \n:jessica-numeric-range: (0.0,10.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "travelProgress", m_travelProgress, "currentTravel progress [0:1]", Be::READ )

	EXPOSURE_END()
}
