// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionSpawnModifierRandomOffset.h"

BLUE_DEFINE( EveDistributionSpawnModifierRandomOffset );

const Be::ClassInfo* EveDistributionSpawnModifierRandomOffset::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistributionSpawnModifierRandomOffset, ":jessica-icon: fa-arrows-up-down-left-right\n" )
		MAP_INTERFACE( EveDistributionSpawnModifierRandomOffset )
		MAP_INTERFACE( IEveDistributionSpawnModifier )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "consistentRandom", m_consistentRandom, "can be toggled off to add time as seed (on load). else it's fully procedural", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "uniformOffset", m_uniformOffset, "if the algorithm should roll simultaniously for all axis values per placement", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minOffset", m_minOffset, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxOffset", m_maxOffset, "", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
