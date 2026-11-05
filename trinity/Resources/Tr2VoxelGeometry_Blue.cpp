// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2VoxelGeometry.h"

BLUE_DEFINE( Tr2VoxelGeometry );


namespace
{
Be::VarChooser Tr2VoxelGeometryMethodChooser[] = {
	{ "Simple", BeCast( Tr2VoxelGeometry::Method::Simple ), "" },
	{ "SurfaceNets", BeCast( Tr2VoxelGeometry::Method::SurfaceNets ), "" },
	{ "DualContouring", BeCast( Tr2VoxelGeometry::Method::DualContouring ), "" },
	{ 0 }
};

Be::VarChooser Tr2VoxelGeometryComponentsChooser[] = {
	{ "SmoothNormals", BeCast( 1 << static_cast<uint32_t>( Tr2VoxelGeometryComponents::SmoothNormals ) ), "" },
	{ "SplitNormals", BeCast( 1 << static_cast<uint32_t>( Tr2VoxelGeometryComponents::SplitNormals ) ), "" },
	{ "Curvature", BeCast( 1 << static_cast<uint32_t>( Tr2VoxelGeometryComponents::Curvature ) ), "" },
	{ "SmoothedCurvature", BeCast( 1 << static_cast<uint32_t>( Tr2VoxelGeometryComponents::SmoothedCurvature ) ), "" },
	{ "DataTags", BeCast( 1 << static_cast<uint32_t>( Tr2VoxelGeometryComponents::DataTags ) ), "" },
	{ 0 }
};
}

BLUE_REGISTER_ENUM_EX(
	"Tr2VoxelGeometryMethod",
	Tr2VoxelGeometry::Method,
	Tr2VoxelGeometryMethodChooser,
	ENUM_REG_ENUM_OBJECT_ON_MODULE );

BLUE_REGISTER_ENUM_EX(
	"Tr2VoxelGeometryComponents",
	Tr2VoxelGeometryComponents,
	Tr2VoxelGeometryComponentsChooser,
	ENUM_REG_ENUM_OBJECT_ON_MODULE );


const Be::ClassInfo* Tr2VoxelGeometry::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2VoxelGeometry, "" )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_INTERFACE( ITr2InstanceData )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_ICACHEABLE_METHODS()

		MAP_ATTRIBUTE( "voxelData", m_voxelData, "", Be::PERSISTONLY )
		MAP_PROPERTY( "voxelData", GetVoxelData, SetVoxelData, "Voxel data of the geometry" )
		MAP_ATTRIBUTE( "shapeChannelIndex", m_shapeChannelIndex, "", Be::PERSISTONLY )
		MAP_PROPERTY( "shapeChannelIndex", GetShapeChannelIndex, SetShapeChannelIndex, "Index of the shape channel in the voxel data" )
		MAP_ATTRIBUTE( "occupancyChannelIndex", m_occupancyChannelIndex, "", Be::PERSISTONLY )
		MAP_PROPERTY( "occupancyChannelIndex", GetOccupancyChannelIndex, SetOccupancyChannelIndex, "Index of the occupancy channel in the voxel data" )
		MAP_ATTRIBUTE( "dataTagChannelIndex", m_dataTagChannelIndex, "", Be::PERSISTONLY )
		MAP_PROPERTY( "dataTagChannelIndex", GetDataTagChannelIndex, SetDataTagChannelIndex, "Index of the data tag channel (one-component float) in the voxel data; the tags of the solid and empty cells either side of the surface go to the DataTags vertex component (SurfaceNets and DualContouring)" )
		MAP_ATTRIBUTE( "subdivision", m_subdivision, "", Be::PERSISTONLY )
		MAP_PROPERTY( "subdivision", GetSubdivision, SetSubdivision, "Subdivision level for the geometry" )
		MAP_ATTRIBUTE( "relaxIterations", m_relaxIterations, "", Be::PERSISTONLY )
		MAP_PROPERTY( "relaxIterations", GetRelaxIterations, SetRelaxIterations, "Number of relaxation iterations for smoothing" )
		MAP_ATTRIBUTE_WITH_CHOOSER( "method", m_method, "", Be::READWRITE | Be::NOTIFY | Be::PERSIST | Be::ENUM, Tr2VoxelGeometryMethodChooser )
		MAP_ATTRIBUTE( "cutSeed", m_cutSeed, "", Be::PERSISTONLY )
		MAP_PROPERTY( "cutSeed", GetCutSeed, SetCutSeed, "Seed of the facet and cleave placement (SurfaceNets and DualContouring)" )
		MAP_ATTRIBUTE( "facetSpacing", m_facetSpacing, "", Be::PERSISTONLY )
		MAP_PROPERTY( "facetSpacing", GetFacetSpacing, SetFacetSpacing, "Distance between facet seeds, in cells" )
		MAP_ATTRIBUTE( "facetDepth", m_facetDepth, "", Be::PERSISTONLY )
		MAP_PROPERTY( "facetDepth", GetFacetDepth, SetFacetDepth, "How far below the surface facet planes sink, in cells; 0 disables facets" )
		MAP_ATTRIBUTE( "facetMaxCut", m_facetMaxCut, "", Be::PERSISTONLY )
		MAP_PROPERTY( "facetMaxCut", GetFacetMaxCut, SetFacetMaxCut, "Deepest cut a facet makes, in cells" )
		MAP_ATTRIBUTE( "cleaveDepth", m_cleaveDepth, "", Be::PERSISTONLY )
		MAP_PROPERTY( "cleaveDepth", GetCleaveDepth, SetCleaveDepth, "Deepest cleave cap, in cells; 0 disables cleaves" )
		MAP_ATTRIBUTE_WITH_CHOOSER( "components", m_components.m_filter, "", Be::READWRITE | Be::NOTIFY | Be::PERSIST, Tr2VoxelGeometryComponentsChooser )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			Create,
			3,
			"Initializes the voxel geometry\n\n"
			":param voxelData: Tr2VoxelData object containing the voxel data\n"
			":param sdfChannelIndex: Index of the SDF channel\n"
			":param occupancyChannelIndex: Index of the occupancy channel\n" )
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
