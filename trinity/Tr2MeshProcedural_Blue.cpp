// Copyright © 2026 CCP ehf.

#include "StdAfx.h"

#include "Tr2MeshProcedural.h"

#include "Resources/Tr2GeometryProvider.h"
#include "TriConstants.h"


BLUE_DEFINE( Tr2MeshProcedural );

const Be::ClassInfo* Tr2MeshProcedural::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2MeshProcedural, "" )

		MAP_INTERFACE( Tr2MeshProcedural )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "geometry", m_geometryResource, "", Be::PERSISTONLY ) // Needed to make Graphite able to detect bindings inside of the geometry
		MAP_PROPERTY( "geometry", GetGeometry, SetGeometryResource, "Procedural geometry" )

	EXPOSURE_CHAINTO( Tr2MeshBase )
}
