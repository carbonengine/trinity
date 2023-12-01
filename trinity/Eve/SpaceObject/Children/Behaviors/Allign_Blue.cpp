#include "StdAfx.h"
#include "Allign.h"

BLUE_DEFINE( Allign );

Be::VarChooser BehaviorPriorityChooser[] =
	{
		{ "LEAST_PRIORITY", BeCast( IBehavior::LEAST_PRIORITY ), "Has the lowest priority" },
		{ "LESS_PRIORITY", BeCast( IBehavior::LESS_PRIORITY ), "Second to lowest priority" },
		{ "MORE_PRIORITY", BeCast( IBehavior::MORE_PRIORITY ), "second highest priority" },
		{ "MOST_PRIORITY", BeCast( IBehavior::MOST_PRIORITY ), "highest priority" },
		{ 0 }
	};


BLUE_REGISTER_ENUM_EX( "BehaviorPriority", IBehavior::ProcessPriority, BehaviorPriorityChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* Allign::ExposeToBlue()
{
	EXPOSURE_BEGIN( Allign, "" )
		MAP_INTERFACE( Allign )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE_WITH_CHOOSER( "behaviorPriority", m_priority, "control what priority this behavior should have", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, BehaviorPriorityChooser )

		MAP_ATTRIBUTE( "enabled", m_enabled, "Should this behavior be active", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "behaviorWeight", m_behaviorWeight, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "visionRange", m_visionRange, "How far the drone looks for it's buddies", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "framesBetweenUpdates", m_framesBetweenUpdates, "updateFrequency to lessen the load", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}