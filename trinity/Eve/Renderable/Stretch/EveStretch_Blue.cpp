// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveStretch.h"
#include "TriFloat.h"

BLUE_DEFINE( EveStretch );
BLUE_DEFINE_INTERFACE( ITr2DebugRenderable );
BLUE_DEFINE_INTERFACE( IStrechAudio );

const Be::ClassInfo* EveStretch::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveStretch, "" )
		MAP_INTERFACE( EveStretch )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IEveTransform )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( IEveFiringEffectElement )
		MAP_INTERFACE( ITr2DebugRenderable )
		MAP_INTERFACE( ITr2LightOwner )
		MAP_INTERFACE( EveEntity )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"display",
			m_display,
			"If set, this transform hierarchy is displayed.\n"
			"Note that turning off display does not automatically turn\n"
			"off update.",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_METHOD_AND_WRAP(
			"Start",
			Start,
			"Trigger the movement if moveObject and curves exist." )

		MAP_ATTRIBUTE(
			"update",
			m_update,
			"If set, this transform hierarchy is updated every frame.",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"source",
			m_source,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"dest",
			m_dest,
			"",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"sourceObject",
			m_sourceObject,
			"Object to be rendered at the source",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"destObject",
			m_destObject,
			"Object to be rendered at the destination",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"stretchObject",
			m_stretchObject,
			"Object to be stretched from source to destination",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"moveObject",
			m_moveObject,
			"Unstretched object to be translated from source to destination",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"sourceLights",
			m_sourceLights,
			"Lights at the source object",
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE(
			"destLights",
			m_destLights,
			"Lights at the destination object",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"progressCurve",
			m_progressCurve,
			"Curve that determines the interpolated position of the move object",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"startTime",
			m_startTime,
			"Start time",
			Be::READ )

		MAP_ATTRIBUTE(
			"moveCompletion",
			m_moveCompletion,
			"Curveset that is triggered when the progress reaches 1.0",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"moveCompleted",
			m_moveCompleted,
			"Gets set to true when the move object reached destination",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"moving",
			m_moving,
			"Flag that is set when Play is called. Tells you that the move has started.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"curveSets",
			m_curveSets,
			"Curvesets for animating things",
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE(
			"length",
			m_length,
			"Distance between the source and the destination",
			Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE(
			"lodLevel",
			m_lodLevel,
			"Current LOD level, 1(high) to 3(low)",
			Be::READ )
		MAP_ATTRIBUTE(
			"useCurveLod",
			m_useCurveLod,
			"Should curves be LOD'd",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"audio",
			m_audio,
			"The type of audio to be used for this asset\n:jessica-deprecated: True\n:jessica-hidden: True",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"stretchAudio",
			m_stretchAudio,
			"An audio object specifically for stretch effects.",
			Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
