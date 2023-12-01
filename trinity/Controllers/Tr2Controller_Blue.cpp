////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2Controller.h"


BLUE_DEFINE_INTERFACE( ITr2ControllerAction );
BLUE_DEFINE_INTERFACE( ITr2Controller );

BLUE_DEFINE( Tr2Controller );

const Be::ClassInfo* Tr2Controller::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Controller, "" )
		MAP_INTERFACE( Tr2Controller )
		MAP_INTERFACE( ITr2Controller )
		MAP_INTERFACE( IListNotify )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "isShared", m_isShared, "If the controller is shared between different models. Only used by tools, validation, etc.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "stateMachines", m_stateMachines, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "variables", m_variables, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "eventHandlers", m_eventHandlers, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "isPlaying", m_isActive, "", Be::READ )
		MAP_PROPERTY_READONLY( "callbackCount", GetCallbackCount, "Returns the number of callbacks that are attached to this controller" )

		MAP_METHOD_AND_WRAP( 
			"Start", 
			Start, 
			"Starts the controller\n"
			":jessica-favorite:\n"
			":jessica-icon: timeline/play.png"
		)
		MAP_METHOD_AND_WRAP( 
			"Stop", 
			Stop, 
			"Stops the controller\n"
			":jessica-favorite:\n"
			":jessica-icon: timeline/stop.png"
		)
		MAP_METHOD_AND_WRAP(
			"HandleEvent",
			HandleEvent,
			"Handle the specified event\n"
			":param name: event name"
		)
		MAP_METHOD_AND_WRAP(
			"GetOwner",
			GetOwner,
			"Returns controller owner"
		)		
		MAP_METHOD_AND_WRAP(
			"RegisterCallback",
			RegisterCallback,
			"Registers a callback under a specific name\n"
			":param callbackName: the name of the callback\n"
			":param callback: A python function that accepts no arguments"
		)
		MAP_METHOD_AND_WRAP(
			"ClearCallbacks",
			ClearCallbacks,
			"Clears all callbacks"
		)
		MAP_METHOD_AND_WRAP(
			"ReLink",
			ReLink,
			"Re-links the controller with the assigned owner. Used by tools only." )

	EXPOSURE_END()
}
