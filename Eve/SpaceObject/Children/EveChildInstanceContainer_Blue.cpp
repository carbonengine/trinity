////////////////////////////////////////////////////////////
//
//    Created:   March 2020
//    Copyright: CCP 2020
//
#include "StdAfx.h"
#include "EveChildInstanceContainer.h"

BLUE_DEFINE( EveChildInstanceContainer );
BLUE_DECLARE_INTERFACE( ITr2DebugRenderable );

const Be::ClassInfo* EveChildInstanceContainer::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildInstanceContainer, "" )
		MAP_INTERFACE( EveChildInstanceContainer )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( ITr2CurveSetOwner )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IEveEffectChildrenOwner )
		MAP_INTERFACE( IShaderConfigurer )
		MAP_INTERFACE( ITr2ControllerOwner )
		MAP_INTERFACE( IListNotify )
		MAP_INTERFACE( EveEntity )
		
		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "alwaysOn", m_isAlwaysOn, "If false this will be hidden if a spaceobjects activation strength < 0.5. If True then it is always on.", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "inheritProperties", m_inheritProperties, "Properties inherited from the parent ship when loaded through SOF", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "origin", m_origin, "Where did this effect originate from", Be::READ )

		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "localTransform", m_localTransform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform, "", Be::READ )

		MAP_ATTRIBUTE( "staticTransform", m_staticTransform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "useSRT", m_useSRT, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "useStaticRotation", m_useStaticRotation, "", Be::READWRITE | Be::PERSIST )

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

		MAP_ATTRIBUTE( "source", m_source, "The sourceObject to instance", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "locatorSet", m_locatorSetName, "The name of the locatorset to distribute across", Be::READWRITE | Be::PERSIST |  Be::NOTIFY )
		MAP_ATTRIBUTE( "reset", m_reset, "Redistributes the source", Be::READWRITE )
		MAP_ATTRIBUTE( "instances", m_instances, "The generated instances\n:jessica-skip-validation:", Be::READ )
		MAP_ATTRIBUTE( "transforms", m_transforms, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "transformModifiers", m_transformModifiers, "", Be::READ | Be::PERSIST | Be::NOTIFY )

		EXPOSURE_END()
};
