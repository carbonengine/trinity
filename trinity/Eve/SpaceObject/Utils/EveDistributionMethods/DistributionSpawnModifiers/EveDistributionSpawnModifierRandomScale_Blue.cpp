// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveDistributionSpawnModifierRandomScale.h"

BLUE_DEFINE_INTERFACE( IEveDistributionSpawnModifier );

BLUE_DEFINE( EveDistributionSpawnModifierRandomScale );

const Be::ClassInfo* EveDistributionSpawnModifierRandomScale::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveDistributionSpawnModifierRandomScale, ":jessica-icon: fa-scale-balanced\n" )
		MAP_INTERFACE( EveDistributionSpawnModifierRandomScale )
		MAP_INTERFACE( IEveDistributionSpawnModifier )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "overrideScale", m_overrideScale, "on: sets placements scale to this, off: multiplies this with placement scale", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "consistentRandom", m_consistentRandom, "can be toggled off to add time as seed (on load). else it's fully procedural", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "uniformScale", m_uniformScale, "if the algorithm should roll simultaniously for all axis values per placement", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minScale", m_minScale, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxScale", m_maxScale, "", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
