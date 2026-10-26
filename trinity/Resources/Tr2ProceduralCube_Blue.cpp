// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2ProceduralCube.h"

BLUE_DEFINE( Tr2ProceduralCube );


const Be::ClassInfo* Tr2ProceduralCube::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ProceduralCube, "" )
		MAP_INTERFACE( Tr2ProceduralCube )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_INTERFACE( ITr2InstanceData )
		MAP_ICACHEABLE_METHODS()

		MAP_PROPERTY( "extent", GetExtent, SetExtent, "Extent of the cube" )

	EXPOSURE_CHAINTO( Tr2ProceduralGeometryProvider )
}
