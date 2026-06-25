// Copyright © 2021 CCP ehf.

#include "EveChildProceduralContainer.h"

BLUE_DEFINE( EveChildProceduralContainer );

const Be::ClassInfo* EveChildProceduralContainer::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildProceduralContainer, "" )
		MAP_INTERFACE( EveChildProceduralContainer )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( ITr2CurveSetOwner )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( IListNotify )
		MAP_INTERFACE( IShaderConfigurer )
		MAP_INTERFACE( ITr2SoundEmitterOwner )
		MAP_INTERFACE( IEveInheritPropertiesOwner )
		MAP_INTERFACE( EveEntity )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "localTransform", m_localTransform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform, "", Be::READ )
		MAP_ATTRIBUTE( "useSRT", m_useSRT, "Should local transform be built from scaling, rotation and translation attributes.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "staticTransform", m_staticTransform, "Does local transform need to be rebuilt every frame.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "useStaticRotation", m_useStaticRotation, "Should this container ignore the parent rotation.", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "selectionMethod", m_selectionMethod, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "selectedObject", m_selectedObject, ":jessica-icon: fa-cube", Be::READ )
		MAP_ATTRIBUTE( "transformModifiers", m_transformModifiers, ":jessica-hidden: True", Be::READ | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "RebuildLocalTransform", RebuildLocalTransform, "Rebuilds local transform." )
		MAP_METHOD_AND_WRAP( "SetControllerVariable", SetControllerVariable, "Set variable for all applicable controllers\n"
																			 ":param name: variable name\n"
																			 ":param value: new variable value\n" )
		MAP_METHOD_AND_WRAP( "HandleControllerEvent", HandleControllerEvent, "Pass an event to controllers\n"
																			 ":param name: event name" )
		MAP_METHOD_AND_WRAP( "StartControllers", StartControllers, "Start all controllers" )
		MAP_METHOD_AND_WRAP( "GetMethodVariableName", GetMethodVariableName, "Get the name of the seed variable being utilized" )
		MAP_METHOD_AND_WRAP( "SetProceduralContainerVariable", SetProceduralContainerVariable, "Set variable for all applicable ProceduralContainers\n:param name: variable name\n:param value: new variable value\n" )
	EXPOSURE_END()
}