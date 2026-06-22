// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionSpawnerTriggerPlane.h"

BLUE_DEFINE( EveDistributionSpawnerTriggerPlane );

const Be::ClassInfo* EveDistributionSpawnerTriggerPlane::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistributionSpawnerTriggerPlane, ":jessica-icon: person-walking-dashed-line-arrow-right\n" )
		MAP_INTERFACE( EveDistributionSpawnerTriggerPlane )
		MAP_INTERFACE( IEveDistributionSpawner )

		MAP_ATTRIBUTE( "planeRotation", m_planeRotation, "direction of trigger sequence", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "playDuration", m_playDuration, "in sec", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "triggerChance", m_triggerChance, "chance of placement triggering when overlapped by the sphere\n:jessica-numeric-range: (0.0,1.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "delayBeforeActivation", m_delayBeforeActivation, "in sec", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "startSequenceAtFirstTrigger", m_startSequenceAtFirstTrigger, "if sequence should skip sphere animation up to the first trigger", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "reversePlaneAnimation", m_reversePlaneAnimation, "play with plane animating backwards", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
