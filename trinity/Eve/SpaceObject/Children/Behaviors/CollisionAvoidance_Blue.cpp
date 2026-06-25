// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "CollisionAvoidance.h"

BLUE_DEFINE( CollisionAvoidance );

extern Be::VarChooser BehaviorPriorityChooser[];

const Be::ClassInfo* CollisionAvoidance::ExposeToBlue()
{
	EXPOSURE_BEGIN( CollisionAvoidance, "" )
		MAP_INTERFACE( CollisionAvoidance )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE_WITH_CHOOSER( "behaviorPriority", m_priority, "control what priority this behavior should have", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, BehaviorPriorityChooser )
		MAP_ATTRIBUTE( "enabled", m_enabled, "Should this behavior be active", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "exclusionVolumes", m_exclusionVolumes, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "avoidanceScalar", m_collisionAvoidanceScalar, "", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}