// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Wander.h"

BLUE_DEFINE( Wander );

extern Be::VarChooser BehaviorPriorityChooser[];

const Be::ClassInfo* Wander::ExposeToBlue()
{
	EXPOSURE_BEGIN( Wander, "" )
		MAP_INTERFACE( Wander )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE_WITH_CHOOSER( "behaviorPriority", m_priority, "control what priority this behavior should have", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, BehaviorPriorityChooser )

		MAP_ATTRIBUTE( "enabled", m_enabled, "Should this behavior be active", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "weightWander", m_weightWander, "How much weight should this behavior have", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rand1", rand1, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rand2", rand2, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rand3", rand3, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "freq", m_freq, "How frequent should the agent change directions", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}