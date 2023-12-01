#include "StdAfx.h"
#include "BackAndForth.h"

BLUE_DEFINE( BackAndForth );

extern Be::VarChooser BehaviorPriorityChooser[];

Be::VarChooser LocatorChooser[] =
	{
		{ "localLocators", BeCast( BackAndForth::LOCAL_LOCATORS ), "Place locators manually, this is for back and forth locators." },
		{ "parentLocatorSet", BeCast( BackAndForth::PARENT_LOCATORS ), "use the parent's locatorSet for drones to seek. Remember to set the locatorSetName" },
		{ "targetLocatorSet", BeCast( BackAndForth::TARGET_LOCATORS ), "use the target's locatorSet for drones to seek. Remember to set the locatorSetName" },
		{ 0 }
	};
BLUE_REGISTER_ENUM_EX( "LocatorType", BackAndForth::LocatorType, LocatorChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* BackAndForth::ExposeToBlue()
{
	EXPOSURE_BEGIN( BackAndForth, "" )
		MAP_INTERFACE( BackAndForth )
		MAP_INTERFACE( IBehavior )

		MAP_ATTRIBUTE_WITH_CHOOSER( "enabled", m_enabled, "Should this behavior be active", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, BehaviorPriorityChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "behaviorPriority", m_priority, "control what priority this behavior should have", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, BehaviorPriorityChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "locatorType", m_locatorType, "Control from where to get locators for drones to seek", Be::READWRITE | Be::PERSIST | Be::ENUM | Be::NOTIFY, LocatorChooser )
		MAP_ATTRIBUTE( "locatorSetName", m_locatorSetName, "Name of locatorSet", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "arrivedRadius", m_arrivedRadius, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "distFromOrigin", m_distFromOrigin, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "slowDownRadius", m_slowDownRadius, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "backAndForthWeight", m_backAndForthWeight, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "fxBehavior", m_fxBehavior, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "locatorSet", m_locatorSets, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "target", m_target, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "parent", m_parent, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "secondsToTurn", m_seconds, "How long should it take for the drone to turn, higher number = slower turn", Be::READWRITE )

		MAP_METHOD_AND_WRAP( "AddLocatorSet", AddLocatorSet, "Adds a locatorSet to the behavior \n:jessica-placement: TOOLBAR\n:jessica-icon: fa-map-pin\n" )

	EXPOSURE_END()
}