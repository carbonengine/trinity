// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionSpawnerTriggerSphere.h"

BLUE_DEFINE( EveDistributionSpawnerTriggerSphere );

const Be::ClassInfo* EveDistributionSpawnerTriggerSphere::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistributionSpawnerTriggerSphere, ":jessica-icon: globe\n" )
		MAP_INTERFACE( EveDistributionSpawnerTriggerSphere )
		MAP_INTERFACE( IEveDistributionSpawner )

		MAP_ATTRIBUTE( "playDuration", m_playDuration, "in sec", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "triggerChance", m_triggerChance, "chance of placement triggering when overlapped by the sphere\n:jessica-numeric-range: (0.0,1.0)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "delayBeforeActivation", m_delayBeforeActivation, "in sec", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "startSequenceAtFirstTrigger", m_startSequenceAtFirstTrigger, "if sequence should skip sphere animation up to the first trigger", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "reverseSphereAnimation", m_reverseSphereAnimation, "play with sphere animating inwards", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sphereOffset", m_sphereOffset, "if you want to start the triggering from the non-center", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
