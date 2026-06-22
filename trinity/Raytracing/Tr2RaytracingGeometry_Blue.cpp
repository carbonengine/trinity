// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2RaytracingGeometry.h"


BLUE_DEFINE( Tr2RaytracingGeometry );

const Be::ClassInfo* Tr2RaytracingGeometry::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2RaytracingGeometry, "" )
		MAP_INTERFACE( Tr2RaytracingGeometry )
		MAP_INTERFACE( ITr2GpuBuffer )
	EXPOSURE_END()
}