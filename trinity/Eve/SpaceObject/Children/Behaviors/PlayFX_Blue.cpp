#include "StdAfx.h"
#include "PlayFX.h"

BLUE_DEFINE( PlayFX );

extern Be::VarChooser BehaviorPriorityChooser[];

const Be::ClassInfo* PlayFX::ExposeToBlue()
{
	EXPOSURE_BEGIN( PlayFX, "" )
		MAP_INTERFACE( PlayFX )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE_WITH_CHOOSER( "behaviorPriority", m_priority, "control what priority this behavior should have", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, BehaviorPriorityChooser )

		MAP_ATTRIBUTE( "enabled", m_enabled, "Should this behavior be active", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "behaviorWeight", m_behaviorWeight, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sec", m_sec, "How long should the fx play", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "firingEffect", m_firingEffect, "A stretch effect for this firing effect", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "generatedFiringEffects", m_firingEffects, "generated firing effects", Be::READ )
	
	EXPOSURE_END()
}