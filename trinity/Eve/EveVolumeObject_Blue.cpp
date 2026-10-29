// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveVolumeObject.h"

BLUE_DEFINE( EveVolumeObject );
BLUE_DEFINE_INTERFACE( ITr2BoxVolume );

const Be::ClassInfo* EveVolumeObject::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveVolumeObject, "A standalone scene object that places a volume in the world and publishes its shape to the attached consumer" )
		MAP_INTERFACE( IEveSpaceObject2 )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( IWorldPosition )
		MAP_INTERFACE( ITr2DebugRenderable )
		MAP_INTERFACE( EveEntity )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"Name identifier of the volume object",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"boxVolume",
			m_boxVolume,
			"The box volume defining the shape",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"consumer",
			m_consumer,
			"",
			Be::PERSISTONLY )
		MAP_PROPERTY(
			"consumer",
			GetConsumer,
			SetConsumer,
			"The object that receives the shape of this volume, attached by the scene file" )

		MAP_ATTRIBUTE(
			"enabled",
			m_enabled,
			"Whether the volume is active for its consumer",
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
