// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveEllipseDefinition.h"

BLUE_DEFINE( EveEllipseDefinition );

const Be::ClassInfo* EveEllipseDefinition::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveEllipseDefinition, "" )
		MAP_INTERFACE( EveEllipseDefinition )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "center", m_center, "Center of the ellipse in local space", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "semiMajor", m_semiMajor, "Semi-major axis length", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "semiMinor", m_semiMinor, "Semi-minor axis length", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "planeNormal", m_planeNormal, "Plane normal of the orbit", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "rotationDegrees", m_rotationDegrees, "Rotation of the major axis within the plane, in degrees", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
	EXPOSURE_END()
}