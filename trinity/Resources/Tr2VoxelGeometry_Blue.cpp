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
		MAP_ATTRIBUTE( "voxelData", m_voxelData, "", Be::PERSISTONLY )
		MAP_PROPERTY( "occupancyChannelIndex", GetOccupancyChannelIndex, SetOccupancyChannelIndex, "Index of the occupancy channel in the voxel data" )
		MAP_ATTRIBUTE( "occupancyChannelIndex", m_occupancyChannelIndex, "", Be::PERSISTONLY )
		MAP_PROPERTY( "cellSize", GetCellSize, SetCellSize, "Size of each voxel cell" )
		MAP_ATTRIBUTE( "cellSize", m_cellSize, "", Be::PERSISTONLY )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS( 
			"__init__", 
			Create, 
			3,
			"Initializes the voxel geometry\n\n" 
			":param voxelData: Tr2VoxelData object containing the voxel data\n"
			":param occupancyChannelIndex: Index of the occupancy channel\n"
			":param cellSize: Size of each voxel cell\n"
		)
		MAP_METHOD_AND_WRAP( 
			"TestBuildFile", 
			TestBuildFile, 
			"Tests building geometry. Builds and saves a CMF file from the voxel data\n\n"
			":param resPath: Path to save the CMF file\n"
			":raises RuntimeError: If building the CMF file fails (no data, empty data, etc.)\n"
			":raises OSError: If there is an OS-level error while writing the file\n"
			":raises IOError: If the file cannot be written to the specified path\n"
		)

	EXPOSURE_CHAINTO( Tr2ProceduralGeometryProvider )
}
