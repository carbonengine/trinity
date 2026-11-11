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
		MAP_PROPERTY( "voxelData", GetVoxelData, SetVoxelData, "Voxel data of the geometry\n\n:jessica-group: Input" )
		MAP_ATTRIBUTE( "shapeChannelIndex", m_shapeChannelIndex, "", Be::PERSISTONLY )
		MAP_PROPERTY( "shapeChannelIndex", GetShapeChannelIndex, SetShapeChannelIndex, "Index of the shape channel in the voxel data\n\n:jessica-group: Input" )

		MAP_ATTRIBUTE( "shapeTransformScale", m_shapeTransform.scale, "", Be::PERSISTONLY )
		MAP_PROPERTY( "shapeTransformScale", GetShapeChannelScale, SetShapeChannelScale, "Shape channel data transform linear scale (useful for normalized data)\n\n:jessica-group: Input" )
		MAP_ATTRIBUTE( "shapeTransformOffset", m_shapeTransform.offset, "", Be::PERSISTONLY )
		MAP_PROPERTY( "shapeTransformOffset", GetShapeChannelOffset, SetShapeChannelOffset, "Shape channel data transform linear offset (useful for normalized data)\n\n:jessica-group: Input" )

		MAP_ATTRIBUTE( "occupancyChannelIndex", m_occupancyChannelIndex, "", Be::PERSISTONLY )
		MAP_PROPERTY( "occupancyChannelIndex", GetOccupancyChannelIndex, SetOccupancyChannelIndex, "Index of the occupancy channel in the voxel data\n\n:jessica-group: Input" )
		MAP_ATTRIBUTE( "dataTagChannelIndex", m_dataTagChannelIndex, "", Be::PERSISTONLY )
		MAP_PROPERTY(
			"dataTagChannelIndex",
			GetDataTagChannelIndex,
			SetDataTagChannelIndex,
			"Index of the data tag channel in the voxel data; the tags of the solid and empty cells either side of the surface go to the DataTags vertex component (SurfaceNets and DualContouring)\n\n"
			":jessica-group: Input" )
		MAP_ATTRIBUTE_WITH_CHOOSER( "method", m_method, "Triangulation method used to create the geometry (see trinity.Tr2VoxelGeometryMethod)\n\n:jessica-group: Shape", Be::READWRITE | Be::NOTIFY | Be::PERSIST | Be::ENUM, Tr2VoxelGeometryMethodChooser )
		MAP_ATTRIBUTE( "subdivision", m_subdivision, "", Be::PERSISTONLY )
		MAP_PROPERTY( "subdivision", GetSubdivision, SetSubdivision, "Subdivision level for the geometry\n\n:jessica-group: Shape" )
		MAP_ATTRIBUTE( "relaxIterations", m_relaxIterations, "", Be::PERSISTONLY )
		MAP_PROPERTY( "relaxIterations", GetRelaxIterations, SetRelaxIterations, "Number of relaxation iterations for smoothing\n\n:jessica-group: Shape" )
		MAP_ATTRIBUTE_WITH_CHOOSER( "components", m_components.m_filter, "Filter for vertex components (see trinity.Tr2VoxelGeometryComponents)\n\n:jessica-group: Output", Be::READWRITE | Be::NOTIFY | Be::PERSIST, Tr2VoxelGeometryComponentsChooser )
		MAP_ATTRIBUTE( "lodLevels", m_lodLevels, "", Be::PERSISTONLY )
		MAP_PROPERTY( "lodLevels", GetLodLevels, SetLodLevels, "Maximum number of LOD levels generated\n\n:jessica-group: Output" )
		MAP_ATTRIBUTE( "packedTangents", m_packedTangents, "", Be::PERSISTONLY )
		MAP_PROPERTY( "packedTangents", GetPackedTangents, SetPackedTangents, "Whether to pack tangents\n\n:jessica-group: Output" )


		MAP_ATTRIBUTE( "cutSeed", m_cutSeed, "", Be::PERSISTONLY )
		MAP_PROPERTY( "cutSeed", GetCutSeed, SetCutSeed, "Seed of the facet and cleave placement (SurfaceNets and DualContouring)\n\n:jessica-group: Cuts" )
		MAP_ATTRIBUTE( "facetSpacing", m_facetSpacing, "", Be::PERSISTONLY )
		MAP_PROPERTY( "facetSpacing", GetFacetSpacing, SetFacetSpacing, "Distance between facet seeds, in cells\n\n:jessica-group: Cuts" )
		MAP_ATTRIBUTE( "facetDepth", m_facetDepth, "", Be::PERSISTONLY )
		MAP_PROPERTY( "facetDepth", GetFacetDepth, SetFacetDepth, "How far below the surface facet planes sink, in cells; 0 disables facets\n\n:jessica-group: Cuts" )
		MAP_ATTRIBUTE( "facetMaxCut", m_facetMaxCut, "", Be::PERSISTONLY )
		MAP_PROPERTY( "facetMaxCut", GetFacetMaxCut, SetFacetMaxCut, "Deepest cut a facet makes, in cells\n\n:jessica-group: Cuts" )
		MAP_ATTRIBUTE( "cleaveDepth", m_cleaveDepth, "", Be::PERSISTONLY )
		MAP_PROPERTY( "cleaveDepth", GetCleaveDepth, SetCleaveDepth, "Deepest cleave cap, in cells; 0 disables cleaves\n\n:jessica-group: Cuts" )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			Create,
			3,
			"Initializes the voxel geometry\n\n"
			":param voxelData: Tr2VoxelData object containing the voxel data\n"
			":param sdfChannelIndex: Index of the SDF channel\n"
			":param occupancyChannelIndex: Index of the occupancy channel\n" )
		MAP_METHOD_AND_WRAP(
			"LockBuild",
			LockBuild,
			"Pauses rebuilds on property changes until UnlockBuild is called. This is useful when multiple properties are being changed at once, to avoid unnecessary rebuilds.\n"
			"It is possible to call LockBuild multiple times, but UnlockBuild must be called the same number of times to unlock the build process." )
		MAP_METHOD_AND_WRAP(
			"UnlockBuild",
			UnlockBuild,
			"Unpauses geometry builds. If a rebuild was pending while locked, it will be triggered now." )
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
