////////////////////////////////////////////////////////////
//
//    Created:   August 2025
//    Copyright: CCP 2025
//

#include "StdAfx.h"
#include "Tr2TimelineController.h"


BLUE_DEFINE( Tr2TimelineContoller );


const Be::ClassInfo* Tr2TimelineContoller::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TimelineContoller, "" )
		MAP_INTERFACE( Tr2TimelineContoller )
		MAP_INTERFACE( ITr2Controller )
		MAP_INTERFACE( ITr2ActionController )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "variables", m_variables, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "eventHandlers", m_eventHandlers, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "isPlaying", m_isActive, "", Be::READ )
		MAP_PROPERTY( "time", GetTime, SetTime, "" )
		MAP_ATTRIBUTE( "timeScale", m_timeScale, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "actions", m_actions, "", Be::PERSISTONLY )
		MAP_ATTRIBUTE( "entries", m_entries, "", Be::PERSISTONLY )

		MAP_METHOD_AND_WRAP(
			"Start",
			Start,
			"Starts the controller\n"
			":jessica-favorite:\n"
			":jessica-icon: timeline/play.png" )
		MAP_METHOD_AND_WRAP(
			"Stop",
			Stop,
			"Stops the controller\n"
			":jessica-favorite:\n"
			":jessica-icon: timeline/stop.png" )
		MAP_METHOD_AND_WRAP(
			"HandleEvent",
			HandleEvent,
			"Handle the specified event\n"
			":param name: event name" )
		MAP_METHOD_AND_WRAP(
			"GetOwner",
			GetOwner,
			"Returns controller owner" )
		MAP_METHOD_AND_WRAP(
			"RegisterCallback",
			RegisterCallback,
			"Registers a callback under a specific name\n"
			":param callbackName: the name of the callback\n"
			":param callback: A python function that accepts no arguments" )
		MAP_METHOD_AND_WRAP(
			"ClearCallbacks",
			ClearCallbacks,
			"Clears all callbacks" )
		MAP_METHOD_AND_WRAP(
			"ReLink",
			ReLink,
			"Re-links the controller with the assigned owner. Used by tools only." )

		MAP_METHOD_AND_WRAP(
			"GetActionCount",
			GetActionCount,
			"" )
		MAP_METHOD_AND_WRAP(
			"GetAction",
			GetAction,
			"" )
		MAP_METHOD_AND_WRAP(
			"GetActionStartTime",
			GetActionStartTime,
			"" )
		MAP_METHOD_AND_WRAP(
			"GetActionEndTime",
			GetActionEndTime,
			"" )
		MAP_METHOD_AND_WRAP(
			"SetActionStartTime",
			SetActionStartTime,
			"" )
		MAP_METHOD_AND_WRAP(
			"SetActionEndTime",
			SetActionEndTime,
			"" )
		MAP_METHOD_AND_WRAP(
			"AddAction",
			AddAction,
			"" )
		MAP_METHOD_AND_WRAP(
			"RemoveAction",
			RemoveAction,
			"" )

	EXPOSURE_END()
}
