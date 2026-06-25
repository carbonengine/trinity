// Copyright © 2025 CCP ehf.

#include "EveDistributionSpawnerInterval.h"

BLUE_DEFINE( EveDistributionSpawnerInterval );

const Be::ClassInfo* EveDistributionSpawnerInterval::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistributionSpawnerInterval, ":jessica-icon: clock-rotate-left\n" )
		MAP_INTERFACE( EveDistributionSpawnerInterval )
		MAP_INTERFACE( IEveDistributionSpawner )

		MAP_ATTRIBUTE( "delayBeforeInitialSpawn", m_delayBeforeInitialSpawn, "to set up fancy sequences", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "numberOfRepeats", m_numberOfTriggers, "0: repeat forever", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "useRandomStartOffset", m_useRandomStartOffset, "to have multiple IntervalSpawners still start naturally (not simultaniously) (not optimal for long repeated interval)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "delayBetweenRepeats", m_delayBetweenRepeats, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxRandomizedIntervalDelta", m_maxRandomizedIntervalDelta, "to make the interval feel more natural, this adds a random value [-this:this] to the delay ", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
