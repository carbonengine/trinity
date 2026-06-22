// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "EveChildSpherePin.h"

BLUE_DEFINE( EveChildSpherePin );

const Be::ClassInfo* EveChildSpherePin::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildSpherePin, "" )
		MAP_INTERFACE( EveChildSpherePin )

		MAP_ATTRIBUTE( "centerNormal", m_centerNormal, "Normal in cartesian coords to the center of the pin on the sphere surface", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "pinRadius", m_pinRadius, "radius of the pin on the sphere surface in radians", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "pinMaxRadius", m_pinMaxRadius, "radius of the pin on the sphere surface in radians, this is the size used by the geometry and should be set as rarely as possible", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "pinRotation", m_pinRotation, "rotation of the pin on the sphere surface in radians", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "pinColor", m_pinColor, "color modulation", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "color", m_pinColor, "color modulation", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "pinAlphaThreshold", m_pinAlphaThreshold, "special alpha value that can be used to show a progress bar", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "curveSets", m_curveSets, "Curvesets for animating things", Be::READ | Be::PERSIST )

	EXPOSURE_CHAINTO( EveChildMesh )
}
