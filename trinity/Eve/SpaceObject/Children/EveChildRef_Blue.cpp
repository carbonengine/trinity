////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//
#include "StdAfx.h"
#include "EveChildRef.h"


BLUE_DEFINE( EveChildRef );

const Be::ClassInfo* EveChildRef::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildRef, "\n:jessica-icon: alicorn\n:jessica-icon-color: (123, 28, 212)\n:jessica-help-url: https://wiki.ccpgames.com/pages/viewpage.action?spaceKey=TTL&title=Plugs+and+Sockets \n" )
		MAP_INTERFACE( EveChildRef )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( ITr2CurveSetOwner )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IEveEffectChildrenOwner )
		MAP_INTERFACE( IShaderConfigurer )
		MAP_INTERFACE( ITr2SoundEmitterOwner )
		MAP_INTERFACE( EveEntity )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "resPath", m_resPath, "Path to a red file.", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY)
		MAP_ATTRIBUTE( "child", m_child, "Reference to the loaded child.\n:jessica-hidden: True\n", Be::READ )

		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "localTransform", m_localTransform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform, "", Be::READ )
		MAP_ATTRIBUTE( "useSRT", m_useSRT, "Should local transform be built from scaling, rotation and translation attributes.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "staticTransform", m_staticTransform, "Does local transform need to be rebuilt every frame.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "loadChildAutomatically", m_loadChildAutomatically, "Don't load child effect automatically \n:jessica-hidden: True\n", Be::READWRITE | Be::PERSIST )



		MAP_METHOD_AND_WRAP( "RebuildLocalTransform", RebuildLocalTransform, "Rebuilds local transform." )

		MAP_METHOD_AND_WRAP(
			"SetControllerVariable",
			SetControllerVariable,
			"Set variable for all applicable controllers\n"
			":jessica-hidden: True\n"
			":param name: variable name\n"
			":param value: new variable value\n"
		)

		MAP_METHOD_AND_WRAP(
			"HandleControllerEvent",
			HandleControllerEvent,
			"Pass an event to controllers\n:jessica-hidden: True\n"
			":param name: event name"
		)

		MAP_METHOD_AND_WRAP(
			"StartControllers",
			StartControllers,
			"Start all controllers\n"
			":jessica-hidden: True\n"
		)

        MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
            "Reload",
            Reload,
            1,
            "Reload the effect child.\n"
        )
    EXPOSURE_END()
}