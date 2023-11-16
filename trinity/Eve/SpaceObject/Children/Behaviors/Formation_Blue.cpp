#include "StdAfx.h"
#include "Formation.h"

BLUE_DEFINE( Formation );

extern Be::VarChooser BehaviorPriorityChooser[];

const Be::ClassInfo* Formation::ExposeToBlue()
{
	EXPOSURE_BEGIN( Formation, "" )
		MAP_INTERFACE( Formation )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE_WITH_CHOOSER( "behaviorPriority", m_priority, "control what priority this behavior should have", 
			Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, BehaviorPriorityChooser )

		MAP_ATTRIBUTE( "enabled", m_enabled, "Should this behavior be active", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "behaviorWeight", m_behaviorWeight, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "framesBetweenUpdates", m_framesBetweenUpdates, "updateFrequency to lessen the load", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxFormationVelocityScaler", m_maxFormationVelocityScaler, "formation speed compared to ship speed", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "stubbornness", m_stubbornness, "How many times in a row it needs to pass a check to break or form", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "inFormation", m_inFormation, "a debug to see when group is attemting to form", Be::READ )

		EXPOSURE_END()
}