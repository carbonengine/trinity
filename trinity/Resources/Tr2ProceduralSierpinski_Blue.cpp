// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2ProceduralSierpinski.h"

BLUE_DEFINE( Tr2ProceduralSierpinski );


const Be::ClassInfo* Tr2ProceduralSierpinski::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ProceduralSierpinski, "" )
		MAP_INTERFACE( Tr2ProceduralSierpinski )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_INTERFACE( ITr2InstanceData )
		MAP_ICACHEABLE_METHODS()

		MAP_PROPERTY( "size", GetSize, SetSize, "Size of the tetrahedron." )
		MAP_PROPERTY( "depth", GetDepth, SetDepth, "Iteration depth of fractal.\n:jessica-numeric-range: (0, 5)" )

	EXPOSURE_CHAINTO( Tr2ProceduralGeometryProvider )
}
