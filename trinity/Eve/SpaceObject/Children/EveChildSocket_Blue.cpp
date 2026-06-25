// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "EveChildSocket.h"


BLUE_DEFINE( EveChildSocket );

const Be::ClassInfo* EveChildSocket::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildSocket, "\n:jessica-icon:blender-phone\n:jessica-icon-color: (123, 28, 212)\n:jessica-help-url: https://wiki.ccpgames.com/pages/viewpage.action?spaceKey=TTL&title=Plugs+and+Sockets \n" )
		MAP_INTERFACE( EveChildSocket )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( ITr2CurveSetOwner )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IEveEffectChildrenOwner )
		MAP_INTERFACE( IShaderConfigurer )
		MAP_INTERFACE( ITr2SoundEmitterOwner )
		MAP_INTERFACE( EveEntity )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "resPath", m_plugResPath, "Path to the plug (must be a red file with just an EveChildPlug).", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "plug", m_plug, "Reference to the loaded plus.\n:jessica-hidden: True\n", Be::READ )

		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "localTransform", m_localTransform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform, "", Be::READ )
		MAP_ATTRIBUTE( "useSRT", m_useSRT, "Should local transform be built from scaling, rotation and translation attributes.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "staticTransform", m_staticTransform, "Does local transform need to be rebuilt every frame.", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "parameters", m_parameters, "List of specialized parameters.", Be::READ | Be::PERSIST | Be::NOTIFY )

		MAP_METHOD_AND_WRAP( "RebuildLocalTransform", RebuildLocalTransform, "Rebuilds local transform.\n:jessica-hidden: True\n" )

		MAP_METHOD_AND_WRAP(
			"SetControllerVariable",
			SetControllerVariable,
			"Set variable for all applicable controllers\n"
			":jessica-hidden: True\n"
			":param name: variable name\n"
			":param value: new variable value\n" )

		MAP_METHOD_AND_WRAP(
			"HandleControllerEvent",
			HandleControllerEvent,
			"Pass an event to controllers\n"
			":jessica-hidden: True\n"
			":param name: event name" )

		MAP_METHOD_AND_WRAP(
			"StartControllers",
			StartControllers,
			"Start all controllers"
			":jessica-hidden: True\n" )

		MAP_METHOD_AND_WRAP(
			"Reload",
			Reload,
			"Reload the effect child.\n" )

		MAP_METHOD_AND_WRAP(
			"Rebind",
			BindParameters,
			"Rebind parameters."
			":jessica-placement:\n"
			":jessica-icon: far-sync\n" )

	EXPOSURE_END()
}