// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2VoxelGeometry.h"

BLUE_DEFINE( Tr2VoxelGeometry );


const Be::ClassInfo* Tr2VoxelGeometry::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2VoxelGeometry, "" )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_INTERFACE( ITr2InstanceData )
		MAP_ICACHEABLE_METHODS()

		MAP_PROPERTY( "voxelData", GetVoxelData, SetVoxelData, "Voxel data of the geometry" )
		MAP_PROPERTY( "cellSize", GetCellSize, SetCellSize, "Size of each voxel cell" )

	EXPOSURE_CHAINTO( Tr2ProceduralGeometryProvider )
}
