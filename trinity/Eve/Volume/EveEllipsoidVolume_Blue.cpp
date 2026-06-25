// Copyright © 2020 CCP ehf.

#include "StdAfx.h"
#include "EveEllipsoidVolume.h"

BLUE_DEFINE( EveEllipsoidVolume );
const Be::ClassInfo* EveEllipsoidVolume::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveEllipsoidVolume, "" )
		MAP_INTERFACE( EveEllipsoidVolume )
		MAP_INTERFACE( IEveVolume )
		MAP_INTERFACE( INotify )


		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_position, "The position of the volume", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "shape", m_shape, "The shape of the outer ellipsoid", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "innerShape", m_innerShape, "The shape of the inner ellipsoid", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "rotation", m_rotation, "The rotation of the ellipsoid", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "debugShowIntersection", m_debugShowIntersection, "When volume debugging is on, you the intersection points can be shown", Be::READWRITE )

	EXPOSURE_END()
}