#include "StdAfx.h"
#include "EveChildBehaviorSystem.h"
#include "Behaviors/IBehavior.h"

BLUE_DEFINE( EveChildBehaviorSystem );
BLUE_DEFINE_INTERFACE( IBehavior );
const Be::ClassInfo* EveChildBehaviorSystem::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildBehaviorSystem, "" )
		MAP_INTERFACE( EveChildBehaviorSystem )
		MAP_INTERFACE( EveChildMesh )
		MAP_INTERFACE( ITr2InstanceData )

		MAP_ATTRIBUTE( "count", m_count, ":jessica-group: Agent", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "maxVelocity", m_maxVelocity, ":jessica-group: Agent", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxForce", m_maxForce, ":jessica-group: Agent", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "behaviors", m_behaviors, ":jessica-group: Agent", Be::READ | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "AddAgent", AddAgent, "" )
		MAP_METHOD_AND_WRAP( "SetCount", SetCount, "" )

	EXPOSURE_CHAINTO( EveChildMesh )
}