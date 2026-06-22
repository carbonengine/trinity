// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveEllipseSet.h"

BLUE_DEFINE( EveEllipseSet );

const Be::ClassInfo* EveEllipseSet::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveEllipseSet, "" )
		MAP_INTERFACE( EveEllipseSet )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( ITr2Pickable )
		MAP_INTERFACE( IListNotify )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "enablePicking", m_enablePicking, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "translation", m_translation, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "rotation", m_rotation, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "scaling", m_scaling, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "depthOffset", m_depthOffset, "", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "ribbonSegmentCount", m_ribbonSegmentCount, "", Be::READWRITE | Be::NOTIFY | Be::PERSIST );
		MAP_ATTRIBUTE( "effect", m_effect, "Effect used to render the ribbon", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "ellipses", m_ellipses, "Collection of orbit ellipses", Be::READ | Be::PERSIST );


		MAP_METHOD_AND_WRAP( "ClearEllipses", ClearEllipses, "Removes all orbit ellipses" );
		MAP_METHOD_AND_WRAP(
			"AddEllipse",
			AddEllipse,
			"Adds a closed elliptical orbit in the given plane (center, semiMajor, semiMinor, planeNormal, rotationDegrees)" );
		MAP_METHOD_AND_WRAP( "__init__", py__init__, "Initializes the EveEllipseSet" );
	EXPOSURE_END()
}
