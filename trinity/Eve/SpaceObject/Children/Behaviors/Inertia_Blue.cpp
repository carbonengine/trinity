#include "StdAfx.h"
#include "Inertia.h"

BLUE_DEFINE( Inertia );

extern Be::VarChooser BehaviorPriorityChooser[];

const Be::ClassInfo* Inertia::ExposeToBlue()
{
	EXPOSURE_BEGIN( Inertia, "" )
		MAP_INTERFACE( Inertia )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE_WITH_CHOOSER( "behaviorPriority", m_priority, "control what priority this behavior should have", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, BehaviorPriorityChooser )

		MAP_ATTRIBUTE( "enabled", m_enabled, "Should this behavior be active", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minInertiaWeight", m_minInertiaWeight, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxRotationSpeed", m_maxRotationSpeed, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxAcceleration", m_maxAcceleration, "", Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
