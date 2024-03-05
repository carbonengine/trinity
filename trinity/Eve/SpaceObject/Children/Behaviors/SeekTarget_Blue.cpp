#include "StdAfx.h"
#include "SeekTarget.h"

BLUE_DEFINE( SeekTarget );

extern Be::VarChooser BehaviorPriorityChooser[];

const Be::ClassInfo* SeekTarget::ExposeToBlue()
{
	EXPOSURE_BEGIN( SeekTarget, "" )
		MAP_INTERFACE( SeekTarget )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE_WITH_CHOOSER( "behaviorPriority", m_priority, "control what priority this behavior should have",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, BehaviorPriorityChooser )		
		
		MAP_ATTRIBUTE( "enabled", m_enabled, "Should this behavior be active", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "locatorSet", m_locatorSet, "Set of Blue structure lists of locators identified by a name", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "locatorSetName", m_locatorSetName, "Name of locatorSet", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "behaviorWeight", m_behaviorWeight, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "distFromOrigin", m_distFromOrigin, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "arrivedRadius", m_arrivedRadius, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "slowDownRadius", m_slowDownRadius, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "target", m_target, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "onFirstDroneArrivedCallback", m_onFirstDroneArrivedCallback, "", Be::READWRITE )
		MAP_ATTRIBUTE( "totalRepairTime", m_totalRepairTime, "", Be::READWRITE )
		MAP_ATTRIBUTE( "secondsToTurn", m_seconds, "How long should it take for the drone to turn, higher number = slower turn", Be::READWRITE )
		MAP_ATTRIBUTE( "firstSpawnAtRandomPlaces", m_firstSpawnAtRandomPlaces, "", Be::READWRITE | Be::PERSIST )

		// Debugging purposes
		MAP_ATTRIBUTE( "exit", m_exit, "set\n:jessica-group: Debugging", Be::READWRITE )
		MAP_ATTRIBUTE( "repair", m_repair, "set\n:jessica-group: Debugging", Be::READWRITE )

		MAP_METHOD_AND_WRAP(
			"SetTarget",
			SetTarget,
			"Assigns target.\n"
			":param transforms: target" )

		MAP_METHOD_AND_WRAP(
			"SetExit",
			SetExit,
			"Set exit value.\n"
			":param transforms: target" )

		MAP_METHOD_AND_WRAP(
			"SetBehaviorWeight",
			SetBehaviorWeight,
			"Set behavior weight.\n"
			":param transforms: target" )

		MAP_METHOD_AND_WRAP(
			"SetupShipRepair",
			SetupShipRepair,
			"Setup seek target and play fx behavior for when players repair ship again.\n"
			":param transforms: target" )

		MAP_METHOD_AND_WRAP(
			"ResetBehavior",
			ResetBehavior,
			"Reset seek target and play fx behavior.\n"
			":param transforms: target" )

		MAP_METHOD_AND_WRAP(
			"SplitBoundingBox",
			SplitBoundingBox,
			"Splits bounding box into sub boxes if ship is large enough.\n"
			":param transforms: target" )
		
		MAP_METHOD_AND_WRAP(
			"AddLocatorSet", 
			AddLocatorSet, 
			"Adds a locatorSet to the behavior \n:jessica-placement: TOOLBAR\n:jessica-icon: fa-map-pin\n" )

		MAP_METHOD_AND_WRAP(
			"SetTotalRepairTime",
			SetTotalRepairTime,
			"Set the totalRepairTime \n" )


		EXPOSURE_END()
}