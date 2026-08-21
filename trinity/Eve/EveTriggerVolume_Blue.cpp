// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveTriggerVolume.h"

BLUE_DEFINE( EveTriggerVolume );

const Be::ClassInfo* EveTriggerVolume::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveTriggerVolume, "A standalone spatial trigger that fires a Python callback when a tracked position enters or exits its volumes" )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( IWorldPosition )
		MAP_INTERFACE( ITr2DebugRenderable )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"Name identifier, passed to the callback so one handler can serve many trigger volumes",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"translation",
			m_translation,
			"Local translation of the trigger volume",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"rotation",
			m_rotation,
			"Local rotation of the trigger volume",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"volumes",
			m_volumes,
			"The volumes defining the trigger region",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"exclusionVolumes",
			m_exclusionVolumes,
			"Volumes subtracted from the trigger region",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"enterThreshold",
			m_enterThreshold,
			"Volume intensity (0..1) at which the tracked position counts as inside",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"externalParameters",
			m_externalParameters,
			"List of external parameters exposing per-placement values, e.g. for dungeon asset manipulations",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"translationCurve",
			m_ballPosition,
			"Function for animated position updates, e.g. the object's own destiny ball in the client",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"trackedPositionCurve",
			m_trackedPosition,
			"Vector function slot for attaching a destiny ball as the tracked position",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"isInside",
			m_isInside,
			"Whether the tracked position is currently inside the trigger region",
			Be::READ )

		MAP_ATTRIBUTE(
			"intensity",
			m_currentIntensity,
			"Most recent evaluated volume intensity of the tracked position",
			Be::READ )

#if BLUE_WITH_PYTHON
		MAP_METHOD_AND_WRAP(
			"SetCallback",
			SetCallback,
			"Sets the callable invoked on enter/exit transitions.\n"
			"The callable is invoked as callback( name, entered ) where entered is\n"
			"True on entry and False on exit. Pass None to clear the callback.\n"
			":param callback: callable or None" )
#endif

	EXPOSURE_END()
}
