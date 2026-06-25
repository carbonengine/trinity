// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2ControllerReference.h"

BLUE_DEFINE( Tr2ControllerReference );

const Be::ClassInfo* Tr2ControllerReference::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ControllerReference, "" )
		MAP_INTERFACE( Tr2ControllerReference )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( ITr2Controller )

		MAP_ATTRIBUTE(
			"path",
			m_path,
			"Path to a controller\n"
			":jessica-widget: filepath\n"
			":jessica-file-filter: redfile",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "controller", m_controller, "Loaded controller", Be::READ )

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
	EXPOSURE_END()
}
