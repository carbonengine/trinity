// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2ProceduralGeometryProvider.h"

BLUE_DEFINE_ABSTRACT( Tr2ProceduralGeometryProvider );


const Be::ClassInfo* Tr2ProceduralGeometryProvider::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ProceduralGeometryProvider, "" )
		MAP_INTERFACE( Tr2ProceduralGeometryProvider )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_INTERFACE( ITr2InstanceData )
		MAP_ICACHEABLE_METHODS()

		MAP_METHOD_AND_WRAP(
			"SetSynchronous",
			SetSynchronous,
			"Build on main thread instead of worker thread." )

		MAP_METHOD_AND_WRAP(
			"Rebuild",
			Rebuild,
			"Rebuild geometry from current parameters" )

	EXPOSURE_CHAINTO( Tr2GeometryProvider )
}
