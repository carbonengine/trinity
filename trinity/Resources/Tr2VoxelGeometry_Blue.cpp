// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2VoxelGeometry.h"

BLUE_DEFINE( Tr2VoxelGeometry );


namespace
{
Be::VarChooser Tr2VoxelGeometryMethodChooser[] = {
	{ "Simple", BeCast( Tr2VoxelGeometry::Method::Simple ), "" },
	{ "SurfaceNets", BeCast( Tr2VoxelGeometry::Method::SurfaceNets ), "" },
	{ 0 }
};
}

BLUE_REGISTER_ENUM_EX(
	"Tr2VoxelGeometryMethod",
	Tr2VoxelGeometry::Method,
	Tr2VoxelGeometryMethodChooser,
	ENUM_REG_ENUM_OBJECT_ON_MODULE );


const Be::ClassInfo* Tr2VoxelGeometry::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2VoxelGeometry, "" )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_INTERFACE( ITr2InstanceData )
		MAP_INTERFACE( IInitialize )
		MAP_ICACHEABLE_METHODS()

		MAP_ATTRIBUTE( "voxelData", m_voxelData, "", Be::PERSISTONLY )
		MAP_PROPERTY( "voxelData", GetVoxelData, SetVoxelData, "Voxel data of the geometry" )
		MAP_ATTRIBUTE( "occupancyChannelIndex", m_occupancyChannelIndex, "", Be::PERSISTONLY )
		MAP_PROPERTY( "occupancyChannelIndex", GetOccupancyChannelIndex, SetOccupancyChannelIndex, "Index of the occupancy channel in the voxel data" )
		MAP_ATTRIBUTE( "cellSize", m_cellSize, "", Be::PERSISTONLY )
		MAP_PROPERTY( "cellSize", GetCellSize, SetCellSize, "Size of each voxel cell" )
		MAP_ATTRIBUTE( "subdivision", m_subdivision, "", Be::PERSISTONLY )
		MAP_PROPERTY( "subdivision", GetSubdivision, SetSubdivision, "Subdivision level for the geometry" )
		MAP_ATTRIBUTE( "relaxIterations", m_relaxIterations, "", Be::PERSISTONLY )
		MAP_PROPERTY( "relaxIterations", GetRelaxIterations, SetRelaxIterations, "Number of relaxation iterations for smoothing" )
		MAP_ATTRIBUTE( "method", m_method, "", Be::PERSISTONLY )
		MAP_PROPERTY( "method", GetMethod, SetMethod, "Method used for generating the geometry (Simple or SurfaceNets)" )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			Create,
			3,
			"Initializes the voxel geometry\n\n"
			":param voxelData: Tr2VoxelData object containing the voxel data\n"
			":param occupancyChannelIndex: Index of the occupancy channel\n"
			":param cellSize: Size of each voxel cell\n" )
		MAP_METHOD_AND_WRAP(
			"TestBuildFile",
			TestBuildFile,
			"Tests building geometry. Builds and saves a CMF file from the voxel data\n\n"
			":param resPath: Path to save the CMF file\n"
			":raises RuntimeError: If building the CMF file fails (no data, empty data, etc.)\n"
			":raises OSError: If there is an OS-level error while writing the file\n"
			":raises IOError: If the file cannot be written to the specified path\n" )

	EXPOSURE_CHAINTO( Tr2ProceduralGeometryProvider )
}
