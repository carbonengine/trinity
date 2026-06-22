// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionSpawnModifierRandomRotation.h"

BLUE_DEFINE( EveDistributionSpawnModifierRandomRotation );

const Be::ClassInfo* EveDistributionSpawnModifierRandomRotation::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistributionSpawnModifierRandomRotation, ":jessica-icon: rotate-left\n" )
		MAP_INTERFACE( EveDistributionSpawnModifierRandomRotation )
		MAP_INTERFACE( IEveDistributionSpawnModifier )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "overrideRotation", m_overrideRotation, "on: sets placements rotaion to this, off: adds this rotation to the placements", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "consistentRandom", m_consistentRandom, "can be toggled off to add time as seed (on load). else it's fully procedural", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minRotation", m_minRotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxRotation", m_maxRotation, "", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
