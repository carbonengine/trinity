////////////////////////////////////////////////////////////
//
//    Created:   2015
//    Copyright: CCP 2015
//
#include "StdAfx.h"
#include "EveChildContainer.h"

BLUE_DEFINE( EveChildContainer );

const Be::ClassInfo* EveChildContainer::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveChildContainer, "" )
        MAP_INTERFACE( EveChildContainer )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( ITr2CurveSetOwner )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( IListNotify )
		MAP_INTERFACE( IEveEffectChildrenOwner )
		MAP_INTERFACE ( IShaderConfigurer )
		MAP_INTERFACE( ITr2SoundEmitterOwner )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "objects", m_objects, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "transformModifiers", m_transformModifiers, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "curveSets", m_curveSets, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "hideOnLowQuality", m_hideOnLowQuality, "Disables all childs in this container on low quality mode.", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "scaling", m_scaling,"", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "localTransform", m_localTransform, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "worldTransform", m_worldTransform, "", Be::READ )
		MAP_ATTRIBUTE( "useSRT", m_useSRT, "Should local transform be built from scaling, rotation and translation attributes.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "staticTransform", m_staticTransform, "Does local transform need to be rebuilt every frame.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "observers", m_observers, "List of audio observers", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "lights", m_lights, "List of dynamic lights", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "controllers", m_controllers, "List of object controllers", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "inheritProperties", m_inheritProperties, "Properties inherited from the parent ship when loaded through SOF", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "RebuildLocalTransform", RebuildLocalTransform, "Rebuilds local transform." )

		MAP_METHOD_AND_WRAP(
			"SetControllerVariable",
			SetControllerVariable,
			"Set variable for all applicable controllers\n"
			":param name: variable name\n"
			":param value: new variable value\n"
		)

		MAP_METHOD_AND_WRAP(
			"StartControllers",
			StartControllers,
			"Start all controllers"
		)

    EXPOSURE_END()
}