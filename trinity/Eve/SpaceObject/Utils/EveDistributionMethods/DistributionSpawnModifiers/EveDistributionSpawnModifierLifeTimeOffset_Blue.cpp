// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionSpawnModifierLifeTimeOffset.h"

BLUE_DEFINE( EveDistributionSpawnModifierLifeTimeOffset );

const Be::ClassInfo* EveDistributionSpawnModifierLifeTimeOffset::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistributionSpawnModifierLifeTimeOffset, ":jessica-icon: fa-timeline-arrow\n" )
		MAP_INTERFACE( EveDistributionSpawnModifierLifeTimeOffset )
		MAP_INTERFACE( IEveDistributionSpawnModifier )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "consistentRandom", m_consistentRandom, "can be toggled off to add time as seed (on load). else it's fully procedural", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "normalizeOffsets", m_normalizeOffsets, "instead set all offsets in initalSets equilly distributed between min and max offset (cascadingLifetimeOffset will be updated to reflect that)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minOffset", m_minOffset, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxOffset", m_maxOffset, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "cascadingLifetimeOffset", m_cascadingLifetimeOffset, "for each proceding element add to the value \n:jessica-numeric-range: (0.0,5.0)", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
