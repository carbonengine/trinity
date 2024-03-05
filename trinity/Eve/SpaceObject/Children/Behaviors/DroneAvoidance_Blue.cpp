#include "StdAfx.h"
#include "DroneAvoidance.h"

BLUE_DEFINE( DroneAvoidance );

extern Be::VarChooser BehaviorPriorityChooser[];

const Be::ClassInfo* DroneAvoidance::ExposeToBlue()
{
	EXPOSURE_BEGIN( DroneAvoidance, "" )
		MAP_INTERFACE( DroneAvoidance )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE_WITH_CHOOSER( "behaviorPriority", m_priority, "control what priority this behavior should have", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, BehaviorPriorityChooser )

		MAP_ATTRIBUTE( "enabled", m_enabled, "Should this behavior be active", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "behaviorWeight", m_behaviorWeight, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "visionRange", m_visionRange, "How far the drone looks for it's buddies", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "framesBetweenUpdates", m_framesBetweenUpdates, "updateFrequency to lessen the load", Be::READWRITE | Be::PERSIST )

		EXPOSURE_END()
}