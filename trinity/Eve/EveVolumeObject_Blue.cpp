// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveVolumeObject.h"

BLUE_DEFINE( EveVolumeObject );

const Be::ClassInfo* EveVolumeObject::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveVolumeObject, "A standalone scene object that places a volume in the world" )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( IWorldPosition )
		MAP_INTERFACE( ITr2DebugRenderable )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"Name identifier of the volume object",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"volumes",
			m_volumes,
			"The volumes defining the shape.",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"enabled",
			m_enabled,
			"Whether the volume is active",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"translationCurve",
			m_ballPosition,
			"Vector function slot for attaching a destiny ball to set the position of the volume object",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"rotationCurve",
			m_ballRotation,
			"Quaternion function slot for attaching a destiny ball to set the rotation of the volume object",
			Be::READWRITE | Be::PERSIST )

	EXPOSURE_END()
}
