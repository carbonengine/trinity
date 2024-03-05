////////////////////////////////////////////////////////////
//
//    Created:   March 2020
//    Copyright: CCP 2020
//

#include "StdAfx.h"
#include "EveStretch3.h"

BLUE_DEFINE( EveStretch3 );

const Be::ClassInfo* EveStretch3::ExposeToBlue()
{
    EXPOSURE_BEGIN( EveStretch3, "" )
        MAP_INTERFACE( EveStretch3 )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IListNotify )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( IEveFiringEffectElement )
		MAP_INTERFACE( ITr2ControllerOwner )
		MAP_INTERFACE( ITr2CurveSetOwner )
		MAP_INTERFACE( ITr2DynamicBindingOwner )
		MAP_INTERFACE( ITr2SoundEmitterOwner )
		MAP_INTERFACE( ITr2LightOwner )

		MAP_ATTRIBUTE
		(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"display",
			m_display,
			"If set, this transform hierarchy is displayed.\n"
			"Note that turning off display does not automatically turn\n"
			"off update.",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"update",
			m_update,
			"If set, this transform hierarchy is updated every frame.",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"source",
			m_source,
			"",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"dest",
			m_dest,
			"",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY
		)

		MAP_ATTRIBUTE
		(
			"sourcePosition",
			m_sourcePosition,
			"",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"destinationPosition",
			m_destinationPosition,
			"",
			Be::READ
		)
		
		MAP_ATTRIBUTE
		(
			"length",
			m_length,
			"Distance between the source and the destination",
			Be::READ | Be::PERSIST
		)

		MAP_PROPERTY
		(
			"sourceSpaceObject",
			GetSourceSpaceObject,
			SetSourceSpaceObject,
			""
		)

		MAP_PROPERTY
		(
			"destSpaceObject",
			GetDestSpaceObject,
			SetDestSpaceObject,
			""
		)

		MAP_ATTRIBUTE
		(
			"sourceObject",
			m_sourceObject,
			"Object to be rendered at the source",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"destObject",
			m_destObject,
			"Object to be rendered at the destination",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		(
			"stretchObject",
			m_stretchObject,
			"Object to be stretched from source to destination",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY
		)


		MAP_ATTRIBUTE
		(
			"moveObject",
			m_moveObject,
			"Unstretched object to be translated from source to destination",
			Be::READWRITE | Be::PERSIST
		)


		MAP_ATTRIBUTE
		(
			"moveProgression",
			m_moveProgression,
			"A value between 0 and 1 to determine the position of the move object between source and dest",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"startTime",
			m_startTime,
			"Start time",
			Be::READ
		)

		MAP_ATTRIBUTE
		(
			"controllers",
			m_controllers,
			"Controllers for awesomeness",
			Be::READ | Be::PERSIST
		)


		MAP_ATTRIBUTE
		(
			"curveSets",
			m_curveSets,
			"Curvesets for animating things",
			Be::READ | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"dynamicBindings",
			m_dynamicBindings,
			"Dynamic bindings",
			Be::READ | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"audio",
			m_audio,
			"The type of audio to be used for this asset",
			Be::READWRITE | Be::PERSIST
		)

		MAP_METHOD_AND_WRAP(
			"StartFiring",
			StartFiring,
			"Starts the firing effect\n "
			":param delay: The delay in seconds until it start firing (sets the firingDelay variable on the controllers)"
			""
		)

		MAP_METHOD_AND_WRAP(
			"StopFiring",
			StopFiring,
			"Stops the firing effect"
		)

		MAP_METHOD_AND_WRAP(
			"SetControllerVariable",
			SetControllerVariable,
			"Set variable for all applicable controllers\n"
			":param name: variable name\n"
			":param value: new variable value\n"
		)

    EXPOSURE_END()
}
