// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionSpawnerBurst.h"

BLUE_DEFINE_INTERFACE( IEveDistributionSpawner );

BLUE_DEFINE( EveDistributionSpawnerBurst );

const Be::ClassInfo* EveDistributionSpawnerBurst::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistributionSpawnerBurst, ":jessica-icon: explosion\n" )
		MAP_INTERFACE( EveDistributionSpawnerBurst )
		MAP_INTERFACE( IEveDistributionSpawner )

		MAP_ATTRIBUTE( "delayBeforeInitialBurst", m_delayBeforeInitialBurst, "in sec", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "completeness", m_completeness, "how much of available locators do we use per burst\n:jessica-numeric-range: (0.0,1.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "additionalTriggersPerBurst", m_additionalTriggersPerBurst, "if you want to have a static number of triggers", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
