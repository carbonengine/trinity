// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "ApproachGroup.h"

BLUE_DEFINE( ApproachGroup );

extern Be::VarChooser BehaviorPriorityChooser[];

const Be::ClassInfo* ApproachGroup::ExposeToBlue()
{
	EXPOSURE_BEGIN( ApproachGroup, "" )
		MAP_INTERFACE( ApproachGroup )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE_WITH_CHOOSER( "behaviorPriority", m_priority, "control what priority this behavior should have", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, BehaviorPriorityChooser )

		MAP_ATTRIBUTE( "enabled", m_enabled, "Should this behavior be active", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "behaviorWeight", m_behaviorWeight, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "visionRange", m_visionRange, "How far the drone looks for it's buddies", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "framesBetweenUpdates", m_framesBetweenUpdates, "updateFrequency to lessen the load", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}