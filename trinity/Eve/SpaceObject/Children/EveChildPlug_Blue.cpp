////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//
#include "StdAfx.h"
#include "EveChildPlug.h"


BLUE_DEFINE( EveChildPlug );

const Be::ClassInfo* EveChildPlug::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildPlug, "\n:jessica-icon: plug\n:jessica-icon-color: (123, 28, 212)\n:jessica-help-url: https://wiki.ccpgames.com/pages/viewpage.action?spaceKey=TTL&title=Plugs+and+Sockets \n" )
		MAP_INTERFACE( EveChildPlug )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( ITr2CurveSetOwner )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IEveEffectChildrenOwner )
		MAP_INTERFACE( IShaderConfigurer )
		MAP_INTERFACE( ITr2SoundEmitterOwner )
		MAP_INTERFACE( EveEntity )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "objects", m_objects, "", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE( "externalParameters", m_externalParameters, "List of external parameters to bind to object elements", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "controllers", m_controllers, "List of object controllers", Be::READ | Be::PERSIST )

		MAP_METHOD_AND_WRAP(
			"SetControllerVariable",
			SetControllerVariable,
			"Set variable for all applicable controllers\n"
			":param name: variable name\n"
			":param value: new variable value\n"
		)

		MAP_METHOD_AND_WRAP(
			"HandleControllerEvent",
			HandleControllerEvent,
			"Pass an event to controllers\n"
			":param name: event name"
		)

		MAP_METHOD_AND_WRAP(
			"StartControllers",
			StartControllers,
			"Start all controllers"
		)

	EXPOSURE_END()
}