// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2VoxelGeometry.h"
#include "../Utilities/Vector3d.h"
#include "../Tr2SignedDistanceField.h"
#include "../Tr2SurfaceCuts.h"
#include <cmf/writer.h>
#include <cmf/bounds.h>
#include <cmf/bufferstreams.h>
#include <numeric>
#include <queue>


namespace
{

bool SameChannelIndex( int32_t a, int32_t b )
{
	return a == b || ( a < 0 && b < 0 );
}

bool IsValidDataTagChannelType( const Tr2VoxelData::PayloadType& type )
{
	return type.elementCount == 1 && ( type.type == Tr2VoxelDataType::Int8 || type.type == Tr2VoxelDataType::Boolean );
}

/**
 * @brief EmptyBuilder is a procedural geometry builder that produces no geometry. It needs to be used when the voxel data is invalid or missing, 
 * as the Tr2ProceduralGeometryProvider needs to always provide a builder, even if it produces no geometry. 
 */
class EmptyBuilder : public Tr2ProceduralGeometryBuilder
{
public:
	bool BuildCmf( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel, cmf::Data& data ) override
	{
		return false;
	}
};

/**
 * @brief Input settings for geometry builders.
 */
struct InputSettings
{
	Tr2VoxelData::PayloadSnapshot shape;
	Tr2SignedDistanceField::LinearTransform shapeTransform;
	Tr2VoxelData::PayloadSnapshot occupancy;
	Tr2VoxelData::PayloadSnapshot tags;

	[[nodiscard]] bool HasShapeChannel() const
	{
		return shape && Tr2SignedDistanceField::IsValidSdfChannelType( shape.m_type );
	}

	[[nodiscard]] bool HasOccupancyChannel() const
	{
		return occupancy && Tr2SignedDistanceField::IsValidOccupancyChannelType( occupancy.m_type );
	}

	[[nodiscard]] bool HasDataTagChannel() const
	{
		return tags && IsValidDataTagChannelType( tags.m_type );
	}

	[[nodiscard]] Tr2VoxelData::Dimensions GetDimensions() const
	{
		if( HasShapeChannel() )
		{
			return shape.m_voxelData->GetDimensions();
		}
		if( HasOccupancyChannel() )
		{
			return occupancy.m_voxelData->GetDimensions();
		}
		return {};
	}
};

/**
 * @brief Represents output configuration options for voxel geometry export.
 */
struct OutputSettings
{
	EnumFilter<Tr2VoxelGeometryComponents> components;
	uint32_t lodLevels = 1;
	bool packedTangents = false;
};

/**
 * @brief VoxelBuilder is a base class for procedural geometry builders that generates geometry from voxel data.
 */
class VoxelBuilder : public Tr2ProceduralGeometryBuilder
{
public:
	VoxelBuilder( InputSettings inputSettings, OutputSettings outputSettings ) :
		m_inputSettings( std::move( inputSettings ) ),
		m_outputSettings( std::move( outputSettings ) ),
		m_dimensions( m_inputSettings.GetDimensions() )
	{
	}

	bool BuildCmf( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel, cmf::Data& data ) override
	{
		if( cancel )
		{
			return false;
		}

		auto [lods, decl] = CreateBuffers( allocator, buffers, cancel );
		if( cancel || lods.empty() )
		{
			return false;
		}

		cmf::MeshArea area = {};
		area.name = allocator.AllocateString( "VoxelDataArea" );

		cmf::Mesh mesh = {};
		mesh.name = allocator.AllocateString( "VoxelData" );
		mesh.decl = decl;
		mesh.lods = lods;
		cmf::Modify( mesh.areas, allocator ).emplace_back( area );
		mesh.topology = cmf::MeshTopology::TriangleList;

		mesh.bounds = cmf::CalculateBounds( mesh, buffers );
		mesh.areas[0].bounds = mesh.bounds;

		data = {};
		cmf::Modify( data.meshes, allocator ).emplace_back( mesh );

		return true;
	}

protected:
	/**
	 * @brief Data extracted from the tag voxel channel, containing voxel values for solid and empty neighboring voxels.
	 */
	struct DataTag
	{
		float solid = {};
		float empty = {};
	};

	/**
	 * @brief Returns the cached signed distance field, creating it on first use from available shape and/or occupancy channels.
	 * The signed distance field is created from the explicit shape channel, or from the occupancy channel, or from a combination of both, 
	 * depending on which channels are available. If both channels are available, the resulting signed distance field is an intersection of the two.
	 * @param cancel A cancellation flag checked by signed-distance-field construction routines to allow early termination.
	 * @return A reference to the internally stored `Tr2SignedDistanceField` instance.
	 */
	Tr2SignedDistanceField& GetSignedDistanceField( const std::atomic<bool>& cancel )
	{
		if( m_signedDistanceField )
		{
			return *m_signedDistanceField;
		}
		if( m_inputSettings.HasShapeChannel() )
		{
			m_signedDistanceField = std::make_unique<Tr2SignedDistanceField>( m_inputSettings.shape, m_inputSettings.shapeTransform, cancel );

			if( m_inputSettings.HasOccupancyChannel() )
			{
				Tr2SignedDistanceField occupancySdf( m_inputSettings.occupancy, cancel );
				m_signedDistanceField->Max( occupancySdf );
			}
		}
		else
		{
			m_signedDistanceField = std::make_unique<Tr2SignedDistanceField>( m_inputSettings.occupancy, cancel );
		}
		return *m_signedDistanceField;
	}

	/**
	 * @brief Result type of the `CreateBuffers` method, containing vertex and index buffer views, vertex declaration, and the bounds of the generated vertices.
	 */
	struct GeneratedMesh
	{
		cmf::Span<cmf::MeshLod> lods;
		cmf::Span<cmf::VertexElement> decl;
	};

	/**
	 * @brief Creates mesh data for the generated geometry, including vertex and index buffers, vertex declaration, and the bounds of the generated vertices.
	 * @param allocator The memory allocator to use for buffer allocations.
	 * @param buffers The buffer manager to use for managing buffer lifetimes.
	 * @param cancel A cancellation flag checked by buffer creation routines to allow early termination.
	 * @return A `GeneratedMesh` containing the created vertex and index buffer views, vertex declaration, and the bounds of the generated vertices.
	 */
	virtual GeneratedMesh CreateBuffers( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel ) = 0;

	cmf::Span<cmf::VertexElement> CreateVertexDeclaration( cmf::MemoryAllocator& allocator ) const
	{
		cmf::Span<cmf::VertexElement> decl;
		uint32_t offset = 0;
		// Appends a float element and returns its index in decl
		auto addElement = [&]( cmf::Usage usage, uint8_t usageIndex, cmf::ElementType type, uint8_t componentCount ) {
			cmf::Modify( decl, allocator ).emplace_back( usage, usageIndex, type, componentCount, offset );
			offset += cmf::GetVertexElementSize( decl[decl.size() - 1] );
			return decl.size() - 1;
		};
		addElement( cmf::Usage::Position, 0, cmf::ElementType::Float32, 3 );
		if( m_outputSettings.components.HasBit( Tr2VoxelGeometryComponents::SmoothNormals ) )
		{
			addElement( m_outputSettings.packedTangents ? cmf::Usage::PackedTangent : cmf::Usage::Normal, 0, cmf::ElementType::Float16, 4 );
		}
		if( m_outputSettings.components.HasBit( Tr2VoxelGeometryComponents::SplitNormals ) )
		{
			addElement( m_outputSettings.packedTangents ? cmf::Usage::PackedTangent : cmf::Usage::Normal, 1, cmf::ElementType::Float16, 4 );
		}

		const auto hasExtraData = m_outputSettings.components.HasBit( Tr2VoxelGeometryComponents::Curvature ) ||
			m_outputSettings.components.HasBit( Tr2VoxelGeometryComponents::SmoothedCurvature ) ||
			m_outputSettings.components.HasBit( Tr2VoxelGeometryComponents::DataTags );
		if( hasExtraData )
		{
			addElement( cmf::Usage::TexCoord, 0, cmf::ElementType::Int8Norm, 4 );
		}
		return decl;
	}
	/**
	 * @brief Helper function that assembles the generated mesh based on enabled output components and available inputs. 
	 * @param allocator Allocator used to build and store the vertex declaration metadata.
	 * @param buffers Buffer manager used to allocate and access vertex and index buffers.
	 * @param positions Per-vertex position data used to populate the position stream (with origin/center offset applied).
	 * @param normals Per-vertex smooth normals, written when smooth normals are enabled.
	 * @param shadingNormals Per-vertex split/shading normals, written when split normals are enabled.
	 * @param curvature Per-vertex curvature values, written to extra packed data when curvature output is enabled.
	 * @param smoothedCurvature Per-vertex smoothed curvature values, written to extra packed data when enabled.
	 * @param dataTags Per-vertex tag pair values (solid/empty), written to extra packed data when enabled.
	 * @param indices Triangle index data copied into the allocated index buffer.
	 * @return A GeneratedMesh containing the allocated vertex buffer view, index buffer view, and the constructed vertex declaration.
	 */
	cmf::MeshLod AllocateBuffers(
		cmf::MemoryAllocator& allocator,
		cmf::BufferManager& buffers,
		float scale,
		const Vector3& offset,
		const cmf::Span<cmf::VertexElement>& decl,
		const std::vector<Vector3>& positions,
		const std::vector<Vector3>& normals,
		const std::vector<Vector3>& shadingNormals,
		const std::vector<float>& curvature,
		const std::vector<float>& smoothedCurvature,
		const std::vector<DataTag>& dataTags,
		const std::vector<uint32_t>& indices ) const
	{
		const auto vertexStride = std::accumulate( decl.begin(), decl.end(), 0u, []( uint32_t size, const cmf::VertexElement& element ) {
			return std::max( size, element.offset + cmf::GetVertexElementSize( element ) );
		} );
		const auto positionElement = cmf::FindElement( decl, cmf::Usage::Position, 0 );
		const auto normalElement = cmf::FindElement( decl, m_outputSettings.packedTangents ? cmf::Usage::PackedTangent : cmf::Usage::Normal, 0 );
		const auto shadingNormalElement = cmf::FindElement( decl, m_outputSettings.packedTangents ? cmf::Usage::PackedTangent : cmf::Usage::Normal, 1 );
		const auto extraElement = cmf::FindElement( decl, cmf::Usage::TexCoord, 0 );

		// Allocate vertex buffer
		cmf::BufferView vbView = buffers.AllocateBuffer(
			nullptr,
			uint32_t( positions.size() * vertexStride ),
			vertexStride );
		cmf::BufferElementStream<Vector3> positionStream( *positionElement, vbView, buffers );
		for( uint32_t i = 0; i < uint32_t( positions.size() ); ++i )
		{
			positionStream.set( i, positions[i] * scale + offset );
		}

		auto PopulateNormals = [&]( const std::vector<Vector3>& normalsToPopulate, const cmf::VertexElement* elementIndex ) {
			if( m_outputSettings.packedTangents )
			{
				cmf::BufferElementStream<Vector4> normalStream( *elementIndex, vbView, buffers );
				for( uint32_t i = 0; i < uint32_t( normalsToPopulate.size() ); ++i )
				{
					const Vector3& n = normalsToPopulate[i];
					auto tangent = std::abs( n.x ) < 0.999f ? Vector3( 1.0f, 0.0f, 0.0f ) : Vector3( 0.0f, 1.0f, 0.0f );
					const auto bitangent = Normalize( Cross( tangent, n ) );
					tangent = Normalize( Cross( n, bitangent ) );
					const Vector4 packedTangent = cmf::PackTangents( cmf::TangentCompression::PackedTangent, n, tangent, bitangent );
					normalStream.set( i, packedTangent );
				}
			}
			else
			{
				cmf::BufferElementStream<Vector3> normalStream( *elementIndex, vbView, buffers );
				for( uint32_t i = 0; i < uint32_t( normalsToPopulate.size() ); ++i )
				{
					normalStream.set( i, normalsToPopulate[i] );
				}
			}
		};

		if( normalElement )
		{
			PopulateNormals( normals, normalElement );
		}
		if( shadingNormalElement )
		{
			PopulateNormals( shadingNormals, shadingNormalElement );
		}
		if( extraElement )
		{
			cmf::BufferElementStream<Vector4> extraDataStream( *extraElement, vbView, buffers );

			for( uint32_t i = 0; i < extraDataStream.size(); ++i )
			{
				const float curvatureValue = m_outputSettings.components.HasBit( Tr2VoxelGeometryComponents::Curvature ) && !curvature.empty() ? curvature[i] : 0.0f;
				const float smoothedCurvatureValue = m_outputSettings.components.HasBit( Tr2VoxelGeometryComponents::SmoothedCurvature ) && !smoothedCurvature.empty() ? smoothedCurvature[i] : 0.0f;
				const Vector2 dataTagValue = m_outputSettings.components.HasBit( Tr2VoxelGeometryComponents::DataTags ) && !dataTags.empty() ? Vector2( dataTags[i].solid, dataTags[i].empty ) : Vector2( 0.0f, 0.0f );
				extraDataStream.set( i, Vector4( curvatureValue, smoothedCurvatureValue, dataTagValue.x, dataTagValue.y ) );
			}
		}

		// Allocate index buffer
		const uint32_t indexStride = sizeof( uint32_t );
		cmf::BufferView ibView = buffers.AllocateBuffer(
			indices.data(),
			uint32_t( indices.size() * indexStride ),
			indexStride );

		cmf::MeshLod lod = {};
		lod.ib = ibView;
		lod.vb = vbView;
		cmf::Modify( lod.areas, allocator ).emplace_back( cmf::LodMeshArea{ 0, uint32_t( indices.size() / 3 ) } );
		return lod;
	}

	const InputSettings m_inputSettings;
	const OutputSettings m_outputSettings;
	const Tr2VoxelData::Dimensions m_dimensions;
	std::unique_ptr<Tr2SignedDistanceField> m_signedDistanceField;
};

/**
 * @brief SimpleBuilder generates geometry from voxel data using a straightforward approach, treating each occupied voxel as a cube.
 * Can be useful for debugging or for simple voxel representations, but does not produce smooth surfaces.
 */
class SimpleBuilder : public VoxelBuilder
{
public:
	SimpleBuilder( InputSettings inputSettings, OutputSettings outputSettings ) :
		VoxelBuilder( std::move( inputSettings ), outputSettings )
	{
	}

protected:
	GeneratedMesh CreateBuffers( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel ) override
	{
		struct Face
		{
			Tr2VoxelData::Coord neighbor;
			Vector3 normal;
			// Corner offsets within the cell, wound clockwise when viewed from outside (same as Tr2ProceduralCube)
			std::array<Tr2VoxelData::Coord, 4> corners;
		};

		static const std::array<Face, 6> s_faces = { {
			{ { -1, 0, 0 }, { -1.f, 0.f, 0.f }, { { { 0, 0, 0 }, { 0, 1, 0 }, { 0, 1, 1 }, { 0, 0, 1 } } } },
			{ { 1, 0, 0 }, { 1.f, 0.f, 0.f }, { { { 1, 0, 0 }, { 1, 0, 1 }, { 1, 1, 1 }, { 1, 1, 0 } } } },
			{ { 0, -1, 0 }, { 0.f, -1.f, 0.f }, { { { 0, 0, 0 }, { 0, 0, 1 }, { 1, 0, 1 }, { 1, 0, 0 } } } },
			{ { 0, 1, 0 }, { 0.f, 1.f, 0.f }, { { { 0, 1, 0 }, { 1, 1, 0 }, { 1, 1, 1 }, { 0, 1, 1 } } } },
			{ { 0, 0, -1 }, { 0.f, 0.f, -1.f }, { { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 } } } },
			{ { 0, 0, 1 }, { 0.f, 0.f, 1.f }, { { { 0, 0, 1 }, { 0, 1, 1 }, { 1, 1, 1 }, { 1, 0, 1 } } } },
		} };

		const auto width = Tr2VoxelData::CoordType( m_dimensions.width );
		const auto height = Tr2VoxelData::CoordType( m_dimensions.height );
		const auto depth = Tr2VoxelData::CoordType( m_dimensions.depth );

		auto& sdf = GetSignedDistanceField( cancel );

		auto isOccupied = [&]( const Tr2VoxelData::Coord& coord ) {
			return sdf.GetValue( coord.x, coord.y, coord.z ) <= 0.f;
		};

		// Vertices are shared between faces only if they have the same grid corner and the same normal (hard edges)
		std::unordered_map<uint64_t, uint32_t> indicesMap;
		std::vector<uint32_t> indices;
		std::vector<Vector3> vertices;
		std::vector<Vector3> normals;

		auto addVertex = [&]( const Tr2VoxelData::Coord& corner, size_t faceIndex ) {
			uint64_t cornerIndex = uint64_t( corner.x ) + uint64_t( width + 1 ) * ( uint64_t( corner.y ) + uint64_t( height + 1 ) * uint64_t( corner.z ) );
			auto [iter, inserted] = indicesMap.try_emplace( cornerIndex * s_faces.size() + faceIndex, uint32_t( vertices.size() ) );
			if( inserted )
			{
				Vector3 position = Vector3( float( corner.x ), float( corner.y ), float( corner.z ) );
				vertices.push_back( position );
				normals.push_back( s_faces[faceIndex].normal );
			}
			return iter->second;
		};

		for( Tr2VoxelData::CoordType z = 0; z < depth; ++z )
		{
			if( cancel )
			{
				return {};
			}
			for( Tr2VoxelData::CoordType y = 0; y < height; ++y )
			{
				for( Tr2VoxelData::CoordType x = 0; x < width; ++x )
				{
					if( !isOccupied( { x, y, z } ) )
					{
						continue;
					}
					for( size_t faceIndex = 0; faceIndex < s_faces.size(); ++faceIndex )
					{
						const Face& face = s_faces[faceIndex];
						if( isOccupied( { x + face.neighbor.x, y + face.neighbor.y, z + face.neighbor.z } ) )
						{
							continue;
						}
						std::array<uint32_t, 4> quad;
						for( size_t i = 0; i < 4; ++i )
						{
							quad[i] = addVertex( { x + face.corners[i].x, y + face.corners[i].y, z + face.corners[i].z }, faceIndex );
						}
						indices.insert( indices.end(), { quad[0], quad[2], quad[1], quad[0], quad[3], quad[2] } );
					}
				}
			}
		}

		cmf::Span<cmf::VertexElement> decl = CreateVertexDeclaration( allocator );
		cmf::Span<cmf::MeshLod> lods;

		const Vector3 offset = Vector3( float( m_dimensions.width ), float( m_dimensions.height ), float( m_dimensions.depth ) ) * -0.5f;

		cmf::Modify( lods, allocator ).emplace_back( AllocateBuffers( allocator, buffers, 1.0f, offset, decl, vertices, normals, normals, {}, {}, {}, indices ) );

		return { lods, decl };
	}
};


/**
 * @brief Common ground of the dual methods. Cell centers sit at integer lattice coordinates, a dual vertex lives in every 2x2x2
 * block of cells with mixed occupancy, and every occupancy sign-change edge emits the quad joining the four blocks sharing
 * it. Cells outside the grid read as empty, so the surface closes at the grid boundary. m_subdivision > 1 runs the
 * construction on a lattice subdivided k-fold whose field is the trilinear interpolation of the SDF. Derived builders
 * decide where the dual vertex goes.
 */
class DualGridBuilder : public VoxelBuilder
{
public:
	DualGridBuilder( InputSettings inputSettings, OutputSettings outputSettings, uint32_t subdivision, uint32_t relaxIterations, const Tr2SurfaceCutParams& cutParams ) :
		VoxelBuilder( std::move( inputSettings ), outputSettings ),
		m_subdivision( subdivision ),
		m_cutParams( cutParams ),
		m_relaxIterations( relaxIterations )
	{
	}

protected:
	using Int3 = std::array<int32_t, 3>;

	static constexpr float s_roamMargin = 0.015; // dual vertex clamp cube inset, keeps neighboring vertices apart

	// (u, v) per axis, right-handed: u x v = +axis
	static constexpr std::array<std::array<int, 2>, 3> s_axisUV = { { { 1, 2 }, { 2, 0 }, { 0, 1 } } };

	/**
	 * @brief Occupancy and field over the lattice plus a one cell ring of empty cells (lattice coordinates -1..lattice)
	 */
	struct Window
	{
		explicit Window( const Int3& lattice_ ) :
			lattice( lattice_ ),
			strides( { 1, size_t( lattice_[0] + 2 ), size_t( lattice_[0] + 2 ) * size_t( lattice_[1] + 2 ) } ),
			occupancy( strides[2] * size_t( lattice_[2] + 2 ), 0 ),
			field( occupancy.size(), std::numeric_limits<float>::quiet_NaN() )
		{
		}

		size_t Index( int32_t x, int32_t y, int32_t z ) const
		{
			return size_t( x + 1 ) + strides[1] * size_t( y + 1 ) + strides[2] * size_t( z + 1 );
		}

		const Int3 lattice;
		const std::array<size_t, 3> strides;
		std::vector<uint8_t> occupancy;
		std::vector<float> field; // NaN where the cell has no field value
	};

	static Int3 DimensionsToInt3( const Tr2VoxelData::Dimensions& dims )
	{
		return { int32_t( dims.width ), int32_t( dims.height ), int32_t( dims.depth ) };
	}

	Window CreateLatticeWindow( const Tr2SignedDistanceField& field, int32_t scale, const std::atomic<bool>& cancel ) const
	{
		return scale == 1 ? CreateGridWindow( field ) : CreateSubdividedWindow( field, scale, cancel );
	}

	/**
	 * @brief Emits a quad for every occupancy sign change, as 4 indices into shared dual vertices (one per block, placed by
	 * getDualVertex), wound counter-clockwise seen from the empty side.
	 * @tparam GetDualVertex The callable type used to compute a vertex position from a block coordinate.
	 * @tparam AddVertexData The callable type used to record per-vertex data of a newly created vertex.
	 * @param window The sampling window containing lattice dimensions, occupancy data, indexing, and strides used to detect surface crossings.
	 * @param cancel An atomic cancellation flag; if set during processing, the function stops early and returns.
	 * @param getDualVertex A callable that takes a block coordinate and returns the corresponding dual vertex position.
	 * @param addVertexData A callable invoked once per new dual vertex with the solid and empty lattice cells of the sign change edge that created it.
	 * @param positions Output vertex-position array. New dual vertices are appended as needed and indexed by generated quads.
	 * @param quads Output index array for quads. For each emitted face, four vertex indices are appended in winding order.
	 */
	template <typename GetDualVertex, typename AddVertexData>
	static void Sweep( const Window& window, const std::atomic<bool>& cancel, GetDualVertex getDualVertex, AddVertexData addVertexData, std::vector<Vector3>& positions, std::vector<uint32_t>& quads )
	{
		const Int3& lattice = window.lattice;
		// Blocks are keyed by their minimum cell, -1..lattice-1 per axis
		const std::array<size_t, 3> blockStrides = { 1, size_t( lattice[0] + 1 ), size_t( lattice[0] + 1 ) * size_t( lattice[1] + 1 ) };
		std::vector<uint32_t> blockVertices( blockStrides[2] * size_t( lattice[2] + 1 ), std::numeric_limits<uint32_t>::max() );

		auto getVertex = [&]( const Int3& block, const Int3& solidCell, const Int3& emptyCell ) {
			auto& vertex = blockVertices[size_t( block[0] + 1 ) + blockStrides[1] * size_t( block[1] + 1 ) + blockStrides[2] * size_t( block[2] + 1 )];
			if( vertex == std::numeric_limits<uint32_t>::max() )
			{
				vertex = uint32_t( positions.size() );
				positions.push_back( getDualVertex( block ) );
				addVertexData( solidCell, emptyCell );
			}
			return vertex;
		};

		// Block fan around the edge, counter-clockwise seen from +axis
		static const std::array<std::array<int32_t, 2>, 4> s_quadUV = { { { -1, -1 }, { 0, -1 }, { 0, 0 }, { -1, 0 } } };

		for( int axis = 0; axis < 3; ++axis )
		{
			const int u = s_axisUV[axis][0];
			const int v = s_axisUV[axis][1];
			// Edges between a cell and its +axis neighbor; the axis coordinate runs -1..lattice-1 so boundary faces are emitted
			Int3 lo = { 0, 0, 0 };
			lo[axis] = -1;
			for( int32_t z = lo[2]; z < lattice[2]; ++z )
			{
				if( cancel )
				{
					return;
				}
				for( int32_t y = lo[1]; y < lattice[1]; ++y )
				{
					for( int32_t x = lo[0]; x < lattice[0]; ++x )
					{
						const size_t index = window.Index( x, y, z );
						if( window.occupancy[index] == window.occupancy[index + window.strides[axis]] )
						{
							continue;
						}
						const Int3 lowCell = { x, y, z };
						Int3 highCell = lowCell;
						++highCell[axis];
						const bool lowSolid = window.occupancy[index] != 0;
						const Int3& solidCell = lowSolid ? lowCell : highCell;
						const Int3& emptyCell = lowSolid ? highCell : lowCell;
						std::array<uint32_t, 4> quad;
						for( size_t i = 0; i < 4; ++i )
						{
							Int3 block = lowCell;
							block[u] += s_quadUV[i][0];
							block[v] += s_quadUV[i][1];
							quad[i] = getVertex( block, solidCell, emptyCell );
						}
						if( !lowSolid )
						{
							// Low cell is the empty side, so the normal points -axis
							std::reverse( quad.begin(), quad.end() );
						}
						quads.insert( quads.end(), quad.begin(), quad.end() );
					}
				}
			}
		}
	}

	/**
	 * @brief Applies signed-distance-based surface cut offsets to mesh vertex positions, limits cuts near geometric features, and iteratively reduces cuts to avoid quad flips.
	 * Only smooth vertices are cut: a vertex whose quads spread wider than s_featureCos sits on a real edge and stays, and the cut ramps in from such
	 * vertices at s_cutRampSlope so no edge stands proud as a fin. A quad the cuts turn over has its most cut corner
	 * lowered a step down s_cutFractions, for up to s_unfoldRounds rounds, after which its corners go uncut.
	 * @param field The signed distance field used to compute per-vertex cut vectors.
	 * @param positions The mesh vertex positions. Input positions are analyzed and then updated in place with the computed cuts.
	 * @param quads Quad index buffer (4 indices per quad) defining mesh connectivity used for normals, feature detection, and neighborhood traversal.
	 * @param cancel Cancellation flag checked at key stages; if set, the function exits early without further processing.
	 */
	void ApplySurfaceCuts( const Tr2SignedDistanceField& field, std::vector<Vector3>& positions, const std::vector<uint32_t>& quads, const std::atomic<bool>& cancel ) const
	{
		if( !m_cutParams.IsEnabled() || quads.empty() )
		{
			return;
		}
		const size_t vertexCount = positions.size();
		const size_t quadCount = quads.size() / 4;

		std::vector<Vector3> quadNormals( quadCount );
		for( size_t q = 0; q < quadCount; ++q )
		{
			quadNormals[q] = GetQuadNormal( positions, quads, q );
		}

		// Quads around each vertex, as corner indices into quads
		std::vector<uint32_t> firstCorner( vertexCount + 1, 0 );
		for( auto vertex : quads )
		{
			++firstCorner[vertex + 1];
		}
		for( size_t v = 0; v < vertexCount; ++v )
		{
			firstCorner[v + 1] += firstCorner[v];
		}
		std::vector<uint32_t> vertexCorners( quads.size() );
		{
			auto next = firstCorner;
			for( size_t corner = 0; corner < quads.size(); ++corner )
			{
				vertexCorners[next[quads[corner]]++] = uint32_t( corner );
			}
		}

		// Facing is the area-weighted mean of the quad normals; a vertex is a feature where two of its quads part too far
		std::vector<Vector3> facing( vertexCount, Vector3( 0.f, 0.f, 0.f ) );
		std::vector<uint8_t> feature( vertexCount, 0 );
		for( size_t v = 0; v < vertexCount; ++v )
		{
			Vector3 sum( 0.f, 0.f, 0.f );
			float spread = 1;
			for( uint32_t i = firstCorner[v]; i < firstCorner[v + 1]; ++i )
			{
				const Vector3& a = quadNormals[vertexCorners[i] / 4];
				sum += a;
				for( uint32_t j = i + 1; j < firstCorner[v + 1]; ++j )
				{
					const Vector3& b = quadNormals[vertexCorners[j] / 4];
					const float lengths = Length( a ) * Length( b );
					if( lengths > 1e-12 )
					{
						spread = std::min( spread, Dot( a, b ) / lengths );
					}
				}
			}
			const float length = Length( sum );
			feature[v] = spread < s_featureCos || length <= 1e-12 ? 1 : 0;
			if( length > 1e-12 )
			{
				facing[v] = sum / length;
			}
		}

		// Mesh distance to the nearest feature vertex, out to where the ramp no longer limits the deepest cut
		const float rampReach = m_cutParams.GetMaxCut() / s_cutRampSlope;
		std::vector<float> featureDistance( vertexCount, std::numeric_limits<float>::infinity() );
		using QueueEntry = std::pair<float, uint32_t>;
		std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> queue;
		for( size_t v = 0; v < vertexCount; ++v )
		{
			if( feature[v] )
			{
				featureDistance[v] = 0;
				queue.push( { float( 0 ), uint32_t( v ) } );
			}
		}
		while( !queue.empty() )
		{
			const auto [distance, v] = queue.top();
			queue.pop();
			if( distance > featureDistance[v] )
			{
				continue;
			}
			for( uint32_t i = firstCorner[v]; i < firstCorner[v + 1]; ++i )
			{
				const uint32_t corner = vertexCorners[i];
				const size_t base = corner & ~uint32_t( 3 );
				for( uint32_t side : { 1u, 3u } )
				{
					const uint32_t neighbor = quads[base + ( ( corner + side ) & 3 )];
					const float d = distance + Length( positions[neighbor] - positions[v] );
					if( d < featureDistance[neighbor] && d < rampReach )
					{
						featureDistance[neighbor] = d;
						queue.push( { d, neighbor } );
					}
				}
			}
		}
		if( cancel )
		{
			return;
		}

		Tr2SurfaceCuts surfaceCuts( field, m_dimensions, m_cutParams );
		std::vector<Vector3> cuts( vertexCount, Vector3( 0.f, 0.f, 0.f ) );
		for( size_t v = 0; v < vertexCount; ++v )
		{
			if( feature[v] )
			{
				continue;
			}
			Vector3 cut = surfaceCuts.GetCut( positions[v], facing[v] );
			const float length = Length( cut );
			const float limit = s_cutRampSlope * featureDistance[v];
			if( length > limit )
			{
				cut *= limit / length;
			}
			cuts[v] = cut;
		}
		if( cancel )
		{
			return;
		}

		const auto uncut = positions;
		std::vector<uint8_t> steps( vertexCount, 0 );
		const uint8_t lastStep = uint8_t( s_cutFractions.size() - 1 );
		auto place = [&]( size_t v ) {
			positions[v] = uncut[v] + cuts[v] * s_cutFractions[steps[v]];
		};
		for( size_t v = 0; v < vertexCount; ++v )
		{
			place( v );
		}
		std::vector<uint8_t> lower( vertexCount, 0 );
		for( int round = 0; round <= s_unfoldRounds; ++round )
		{
			// Each round reads the previous round's placement only
			bool changed = false;
			for( size_t q = 0; q < quadCount; ++q )
			{
				const Vector3& before = quadNormals[q];
				if( Dot( before, before ) <= 1e-24 || Dot( GetQuadNormal( positions, quads, q ), before ) > 0 )
				{
					continue;
				}
				// The corner keeping the largest fraction of its cut steps down; past the last round every corner goes uncut
				int32_t pick = -1;
				for( size_t i = 0; i < 4; ++i )
				{
					const uint32_t v = quads[q * 4 + i];
					if( steps[v] == lastStep || Dot( cuts[v], cuts[v] ) == 0 )
					{
						continue;
					}
					if( round == s_unfoldRounds )
					{
						lower[v] = lastStep;
						changed = true;
					}
					else if( pick < 0 || steps[v] < steps[pick] )
					{
						pick = int32_t( v );
					}
				}
				if( pick >= 0 )
				{
					lower[pick] = uint8_t( steps[pick] + 1 );
					changed = true;
				}
			}
			if( !changed )
			{
				break;
			}
			for( size_t v = 0; v < vertexCount; ++v )
			{
				if( lower[v] > steps[v] )
				{
					steps[v] = lower[v];
					place( v );
				}
				lower[v] = 0;
			}
		}
	}

	/**
	 * @brief Splits each quad along the diagonal whose two triangles are closer to coplanar.
	 * @param positions The positions of the vertices.
	 * @param quads The indices of the quads.
	 * @return A vector of triangle indices.
	 */
	static std::vector<uint32_t> Triangulate( const std::vector<Vector3>& positions, const std::vector<uint32_t>& quads )
	{
		std::vector<uint32_t> indices;
		indices.reserve( quads.size() / 4 * 6 );
		for( size_t q = 0; q < quads.size(); q += 4 )
		{
			const uint32_t i0 = quads[q];
			const uint32_t i1 = quads[q + 1];
			const uint32_t i2 = quads[q + 2];
			const uint32_t i3 = quads[q + 3];
			const auto& p0 = positions[i0];
			const auto& p1 = positions[i1];
			const auto& p2 = positions[i2];
			const auto& p3 = positions[i3];
			const float fold02 = GetFoldCosine( GetTriangleNormal( p0, p1, p2 ), GetTriangleNormal( p0, p2, p3 ) );
			const float fold13 = GetFoldCosine( GetTriangleNormal( p0, p1, p3 ), GetTriangleNormal( p1, p2, p3 ) );
			if( fold02 >= fold13 )
			{
				indices.insert( indices.end(), { i0, i1, i2, i0, i2, i3 } );
			}
			else
			{
				indices.insert( indices.end(), { i0, i1, i3, i1, i2, i3 } );
			}
		}
		return indices;
	}

	/**
	 * @brief Computes area-weighted vertex normals.
	 * @param positions The positions of the vertices.
	 * @param indices The indices of the triangles.
	 * @return A vector of vertex normals.
	 */
	static std::vector<Vector3> ComputeNormals( const std::vector<Vector3>& positions, const std::vector<uint32_t>& indices )
	{
		std::vector<Vector3> normals( positions.size(), Vector3{ 0.0, 0.0, 0.0 } );
		for( size_t t = 0; t < indices.size(); t += 3 )
		{
			const Vector3 n = GetTriangleNormal( positions[indices[t]], positions[indices[t + 1]], positions[indices[t + 2]] );
			for( size_t i = 0; i < 3; ++i )
			{
				auto& normal = normals[indices[t + i]];
				for( int j = 0; j < 3; ++j )
				{
					normal[j] += n[j];
				}
			}
		}
		for( auto& normal : normals )
		{
			const float length = std::sqrt( normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2] );
			if( length > 1e-12 )
			{
				normal = { normal[0] / length, normal[1] / length, normal[2] / length };
			}
			else
			{
				// Degenerate 1-ring; arbitrary but deterministic
				normal = { 0.0, 0.0, 1.0 };
			}
		}
		return normals;
	}

	/**
	 * @brief Shading normals with hard edges. The triangles around a vertex fall into fans, joined across the edges they share
	 * where the two fold less than s_shadingSplitDegrees, and every fan gets its own copy of the vertex shaded with the
	 * fan's area-weighted normal; every copy keeps the smooth normal for displacement. Triangles under s_minShadingArea
	 * (grid cell units) have unreliable normals, so they never split: they join every fan they share an edge with, and a
	 * vertex they join into a single fan shades with its smooth normal. Returns the shading normals, with positions,
	 * normals, curvatures, data tags and indices rewritten to the split vertices. Curvatures and data tags that were not
	 * computed are empty and stay so.
	 */
	static std::vector<Vector3> SplitNormals( std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<float>& curvature, std::vector<float>& smoothedCurvature, std::vector<DataTag>& dataTags, std::vector<uint32_t>& indices )
	{
		const size_t vertexCount = positions.size();
		const size_t triangleCount = indices.size() / 3;

		std::vector<Vector3> triangleNormals( triangleCount );
		std::vector<uint8_t> tiny( triangleCount, 0 );
		for( size_t t = 0; t < triangleCount; ++t )
		{
			triangleNormals[t] = GetTriangleNormal( positions[indices[t * 3]], positions[indices[t * 3 + 1]], positions[indices[t * 3 + 2]] );
			tiny[t] = Length( triangleNormals[t] ) * 0.5f < s_minShadingArea ? 1 : 0;
		}

		// Triangles around each vertex, as corner indices into indices
		std::vector<uint32_t> firstCorner( vertexCount + 1, 0 );
		for( auto vertex : indices )
		{
			++firstCorner[vertex + 1];
		}
		for( size_t v = 0; v < vertexCount; ++v )
		{
			firstCorner[v + 1] += firstCorner[v];
		}
		std::vector<uint32_t> vertexCorners( indices.size() );
		{
			auto next = firstCorner;
			for( size_t corner = 0; corner < indices.size(); ++corner )
			{
				vertexCorners[next[indices[corner]]++] = uint32_t( corner );
			}
		}

		// Two corners on the same vertex share an edge when their triangles have another vertex in common
		auto sharesEdge = [&]( uint32_t a, uint32_t b ) {
			for( uint32_t i : { 1u, 2u } )
			{
				const uint32_t vertex = indices[a - a % 3 + ( a % 3 + i ) % 3];
				for( uint32_t j : { 1u, 2u } )
				{
					if( vertex == indices[b - b % 3 + ( b % 3 + j ) % 3] )
					{
						return true;
					}
				}
			}
			return false;
		};

		std::vector<Vector3> splitPositions;
		std::vector<Vector3> splitNormals;
		std::vector<Vector3> shadingNormals;
		std::vector<float> splitCurvature;
		std::vector<float> splitSmoothedCurvature;
		std::vector<DataTag> splitDataTags;
		splitPositions.reserve( vertexCount );
		splitNormals.reserve( vertexCount );
		shadingNormals.reserve( vertexCount );
		splitCurvature.reserve( curvature.size() );
		splitSmoothedCurvature.reserve( smoothedCurvature.size() );
		splitDataTags.reserve( dataTags.size() );
		std::vector<uint32_t> splitIndices( indices.size() );
		const auto shadingSplitCos = cosf( s_shadingSplitDegrees * 3.14159265358979323846 / 180.0 );

		// Union-find over the corners of one vertex
		std::vector<uint32_t> parent;
		std::vector<Vector3> sums;
		std::vector<uint32_t> copies;
		auto find = [&]( uint32_t i ) {
			while( parent[i] != i )
			{
				parent[i] = parent[parent[i]];
				i = parent[i];
			}
			return i;
		};
		for( size_t v = 0; v < vertexCount; ++v )
		{
			const uint32_t first = firstCorner[v];
			const uint32_t count = firstCorner[v + 1] - first;
			parent.resize( count );
			std::iota( parent.begin(), parent.end(), 0u );
			for( uint32_t i = 0; i < count; ++i )
			{
				const uint32_t a = vertexCorners[first + i];
				const Vector3& na = triangleNormals[a / 3];
				for( uint32_t j = 0; j < i; ++j )
				{
					const uint32_t b = vertexCorners[first + j];
					if( !sharesEdge( a, b ) )
					{
						continue;
					}
					const Vector3& nb = triangleNormals[b / 3];
					if( tiny[a / 3] || tiny[b / 3] || Dot( na, nb ) >= shadingSplitCos * Length( na ) * Length( nb ) )
					{
						parent[find( i )] = find( j );
					}
				}
			}

			sums.assign( count, Vector3( 0.f, 0.f, 0.f ) );
			for( uint32_t i = 0; i < count; ++i )
			{
				sums[find( i )] += triangleNormals[vertexCorners[first + i] / 3];
			}
			copies.assign( count, std::numeric_limits<uint32_t>::max() );
			for( uint32_t i = 0; i < count; ++i )
			{
				const uint32_t root = find( i );
				if( copies[root] == std::numeric_limits<uint32_t>::max() )
				{
					// A degenerate fan shades with the smooth normal
					Vector3 shading = normals[v];
					const float length = Length( sums[root] );
					if( length > 1e-12 )
					{
						shading = sums[root] / length;
					}
					copies[root] = uint32_t( splitPositions.size() );
					splitPositions.push_back( positions[v] );
					splitNormals.push_back( normals[v] );
					shadingNormals.push_back( shading );
					if( !curvature.empty() )
					{
						splitCurvature.push_back( curvature[v] );
					}
					if( !smoothedCurvature.empty() )
					{
						splitSmoothedCurvature.push_back( smoothedCurvature[v] );
					}
					if( !dataTags.empty() )
					{
						splitDataTags.push_back( dataTags[v] );
					}
				}
				splitIndices[vertexCorners[first + i]] = copies[root];
			}
		}

		positions.swap( splitPositions );
		normals.swap( splitNormals );
		curvature.swap( splitCurvature );
		smoothedCurvature.swap( splitSmoothedCurvature );
		dataTags.swap( splitDataTags );
		indices.swap( splitIndices );
		return shadingNormals;
	}

	/**
	 * @brief Computes signed curvature per vertex: how far the vertex stands out of its quad-perimeter ring along its normal, over the ring's
	 * mean edge length, so positive on ridges and negative in crevices. Each output is that raw value after its own number
	 * of smoothing passes (half the vertex plus half its ring's mean), centered on the mesh's median so a gently convex
	 * body reads neutral.
	 * @param positions The positions of the vertices.
	 * @param normals The normals of the vertices.
	 * @param quads The indices of the quads.
	 * @param outputComponents The components to compute; curvature and/or smoothed curvature.
	 * @param curvature The output vector of raw curvature values.
	 * @param smoothedCurvature The output vector of smoothed curvature values.
	 */
	static void ComputeCurvature( const std::vector<Vector3>& positions, const std::vector<Vector3>& normals, const std::vector<uint32_t>& quads, EnumFilter<Tr2VoxelGeometryComponents> outputComponents, std::vector<float>& curvature, std::vector<float>& smoothedCurvature )
	{
		if( !outputComponents.HasBit( Tr2VoxelGeometryComponents::Curvature ) && !outputComponents.HasBit( Tr2VoxelGeometryComponents::SmoothedCurvature ) )
		{
			return;
		}

		const size_t vertexCount = positions.size();
		const auto neighbors = GetQuadNeighbors( vertexCount, quads );

		std::vector<float> level( vertexCount, 0.f );
		for( size_t i = 0; i < vertexCount; ++i )
		{
			const auto& ring = neighbors[i];
			if( ring.empty() )
			{
				continue;
			}
			const Vector3& p = positions[i];
			Vector3 centroid = { 0.0, 0.0, 0.0 };
			float edge = 0;
			for( auto neighbor : ring )
			{
				centroid += positions[neighbor];
				edge += Length( positions[neighbor] - p );
			}
			const float count = float( ring.size() );
			edge /= count;
			if( edge > 1e-9f )
			{
				level[i] = Dot( p - centroid / count, normals[i] ) / edge;
			}
		}

		std::vector<float> below( vertexCount );
		for( uint32_t pass = 1; pass <= s_smoothedCurvaturePasses; ++pass )
		{
			level.swap( below );
			for( size_t i = 0; i < vertexCount; ++i )
			{
				const auto& ring = neighbors[i];
				if( ring.empty() )
				{
					level[i] = below[i];
					continue;
				}
				float sum = 0;
				for( auto neighbor : ring )
				{
					sum += below[neighbor];
				}
				level[i] = 0.5f * below[i] + 0.5f * sum / float( ring.size() );
			}
			if( pass == s_curvaturePasses && outputComponents.HasBit( Tr2VoxelGeometryComponents::Curvature ) )
			{
				curvature = CentreOnMedian( level );
				if( !outputComponents.HasBit( Tr2VoxelGeometryComponents::SmoothedCurvature ) )
				{
					return;
				}
			}
		}
		smoothedCurvature = CentreOnMedian( level );
	}

	/**
	 * @brief Jacobi Laplacian smoothing: each vertex moves a fraction s_relaxAlpha toward the mean of its quad-perimeter neighbours
	 * @param positions The positions of the vertices.
	 * @param quads The indices of the quads.
	 * @param cancel A flag to cancel the operation.
	 */
	void Relax( std::vector<Vector3>& positions, const std::vector<uint32_t>& quads, const std::atomic<bool>& cancel ) const
	{
		if( m_relaxIterations == 0 || quads.empty() )
		{
			return;
		}
		const auto neighbors = GetQuadNeighbors( positions.size(), quads );

		std::vector<Vector3> moved( positions.size() );
		for( uint32_t iteration = 0; iteration < m_relaxIterations; ++iteration )
		{
			if( cancel )
			{
				return;
			}
			for( size_t i = 0; i < positions.size(); ++i )
			{
				const auto& p = positions[i];
				if( neighbors[i].empty() )
				{
					moved[i] = p;
					continue;
				}
				Vector3 sum = { 0.0, 0.0, 0.0 };
				for( auto neighbor : neighbors[i] )
				{
					sum += positions[neighbor];
				}
				const float count = float( neighbors[i].size() );
				for( int j = 0; j < 3; ++j )
				{
					moved[i][j] = p[j] + s_relaxAlpha * ( sum[j] / count - p[j] );
				}
			}
			positions.swap( moved );
		}
	}

	template <typename GetDualVertex>
	GeneratedMesh CreateBuffersImpl( GetDualVertex getDualVertex, cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel )
	{
		if( m_dimensions.width == 0 || m_dimensions.height == 0 || m_dimensions.depth == 0 )
		{
			return {};
		}

		int32_t scale = int32_t( std::max( m_subdivision, 1u ) );

		auto& field = GetSignedDistanceField( cancel );
		if( cancel )
		{
			return {};
		}

		cmf::Span<cmf::VertexElement> decl = CreateVertexDeclaration( allocator );
		cmf::Span<cmf::MeshLod> lods;
		float positionScale = 1.0f;

		for( uint32_t lod = 0; lod < m_outputSettings.lodLevels; ++lod )
		{
			auto window = CreateLatticeWindow( field, scale, cancel );
			if( cancel )
			{
				return {};
			}

			std::vector<Vector3> positions;
			std::vector<DataTag> dataTags;
			std::vector<uint32_t> quads;
			// Without a data tag channel the requested element reads zero
			const bool sampleDataTags = m_outputSettings.components.HasBit( Tr2VoxelGeometryComponents::DataTags ) && m_inputSettings.HasDataTagChannel();
			Sweep(
				window,
				cancel,
				[&]( const Int3& block ) { return getDualVertex( window, field, block, scale ); },
				[&]( const Int3& solidCell, const Int3& emptyCell ) {
					if( sampleDataTags )
					{
						dataTags.push_back( { GetDataTag( solidCell, scale ), GetDataTag( emptyCell, scale ) } );
					}
				},
				positions,
				quads );
			if( cancel )
			{
				return {};
			}
			if( m_outputSettings.components.HasBit( Tr2VoxelGeometryComponents::DataTags ) && !sampleDataTags )
			{
				dataTags.assign( positions.size(), {} );
			}
			if( quads.empty() )
			{
				return { lods, decl };
			}
			Relax( positions, quads, cancel );
			if( cancel )
			{
				return {};
			}
			ApplySurfaceCuts( field, positions, quads, cancel );
			if( cancel )
			{
				return {};
			}
			auto indices = Triangulate( positions, quads );
			auto normals = ComputeNormals( positions, indices );
			std::vector<float> curvature;
			std::vector<float> smoothedCurvature;
			ComputeCurvature( positions, normals, quads, m_outputSettings.components, curvature, smoothedCurvature );
			std::vector<Vector3> shadingNormals;
			if( m_outputSettings.components.HasBit( Tr2VoxelGeometryComponents::SplitNormals ) )
			{
				shadingNormals = SplitNormals( positions, normals, curvature, smoothedCurvature, dataTags, indices );
			}

			const Vector3 offset = Vector3( float( m_dimensions.width ), float( m_dimensions.height ), float( m_dimensions.depth ) ) * -0.5f + Vector3( 0.5f, 0.5f, 0.5f );
			cmf::Modify( lods, allocator ).emplace_back( AllocateBuffers( allocator, buffers, positionScale, offset, decl, positions, normals, shadingNormals, curvature, smoothedCurvature, dataTags, indices ) );
			if( lod > 0 )
			{
				auto& lastLod = lods[lod];
				lastLod.threshold = std::max( { window.lattice[0], window.lattice[1], window.lattice[2] } ) * scale * 3;
			}
			if( scale == 1 )
			{
				if( lod + 1 == m_outputSettings.lodLevels )
				{
					break;
				}
				field.Downsample();
				positionScale *= 2.0f;
			}
			else
			{
				scale -= 1;
			}
		}
		return { lods, decl };
	}

	/**
	 * @brief Data tag of the grid voxel nearest to a lattice cell. Cells of the empty ring around the lattice read the
	 * nearest voxel inside the grid.
	 * @param cell The lattice cell, -1..lattice per axis.
	 * @param scale The lattice subdivision.
	 * @return The data tag channel value of the voxel.
	 */
	float GetDataTag( const Int3& cell, int32_t scale ) const
	{
		const Int3 dims = DimensionsToInt3( m_dimensions );
		Int3 voxel;
		for( int j = 0; j < 3; ++j )
		{
			// Lattice cell c sits at c / scale in grid space; rounded to the nearest voxel
			voxel[j] = std::clamp( ( 2 * cell[j] + scale ) / ( 2 * scale ), 0, dims[j] - 1 );
		}
		const size_t index = size_t( voxel[0] ) + size_t( dims[0] ) * ( size_t( voxel[1] ) + size_t( dims[1] ) * size_t( voxel[2] ) );
		if( m_inputSettings.tags.m_type.type == Tr2VoxelDataType::Boolean )
		{
			return ( m_inputSettings.tags.m_data[index / 8] & ( 1 << ( index % 8 ) ) ) != 0 ? 1.0f : 0.0f;
		}
		const auto value = reinterpret_cast<const int8_t*>( m_inputSettings.tags.m_data )[index];
		return float( std::max( float( value ) / float( std::numeric_limits<int8_t>::max() ), -1.0f ) );
	}

	const uint32_t m_subdivision;
	const Tr2SurfaceCutParams m_cutParams;
	const uint32_t m_relaxIterations;

private:
	static constexpr float s_featureCos = 0.70710678f; // cos 45 degrees: quads parting wider than this meet at a feature edge
	static constexpr float s_cutRampSlope = 0.6f; // a cut grows at most this much per cell of mesh distance from a feature
	static constexpr int s_unfoldRounds = 3; // rounds of fold repair before a turned over quad's corners go uncut
	static constexpr std::array<float, 4> s_cutFractions = { 1.f, 0.5f, 0.25f, 0.f }; // the fractions of its cut a vertex steps down
	static constexpr float s_relaxAlpha = 0.5; // Laplacian relaxation strength per iteration
	static constexpr float s_shadingSplitDegrees = 120.f; // adjacent triangles folding wider than this split the shading normal
	static constexpr float s_minShadingArea = 0.05f; // triangles smaller than this, in grid cell units, shade with the smooth normal
	static constexpr uint32_t s_curvaturePasses = 2; // smoothing passes of the curvature element
	static constexpr uint32_t s_smoothedCurvaturePasses = 8; // smoothing passes of the smoothed curvature element, wide enough to hide per-cell ripple

	/**
	 * @brief Builds a per-vertex adjacency list from a quad index buffer.
	 * @param vertexCount The total number of vertices; determines the size of the returned neighbor list.
	 * @param quads A flat array of vertex indices where each consecutive group of 4 indices defines one quad.
	 * @return A vector of neighbor lists, one per vertex, containing unique adjacent vertex indices sorted in ascending order.
	 */
	static std::vector<std::vector<uint32_t>> GetQuadNeighbors( size_t vertexCount, const std::vector<uint32_t>& quads )
	{
		std::vector<std::vector<uint32_t>> neighbors( vertexCount );
		for( size_t q = 0; q < quads.size(); q += 4 )
		{
			for( size_t i = 0; i < 4; ++i )
			{
				const uint32_t a = quads[q + i];
				const uint32_t b = quads[q + ( ( i + 1 ) & 3 )];
				neighbors[a].push_back( b );
				neighbors[b].push_back( a );
			}
		}
		for( auto& list : neighbors )
		{
			std::sort( list.begin(), list.end() );
			list.erase( std::unique( list.begin(), list.end() ), list.end() );
		}
		return neighbors;
	}

	/**
	 * @brief Returns a copy of the input values shifted so their median becomes 0.
	 * @param values The input vector of float values to center on the median.
	 * @return A vector where each element is the original value minus the median; returns the input unchanged if the vector is empty.
	 */
	static std::vector<float> CentreOnMedian( std::vector<float> values )
	{
		if( values.empty() )
		{
			return values;
		}
		auto sorted = values;
		std::nth_element( sorted.begin(), sorted.begin() + sorted.size() / 2, sorted.end() );
		const float median = sorted[sorted.size() / 2];
		for( auto& value : values )
		{
			value -= median;
		}
		return values;
	}

	/**
	 * @brief Computes the area-weighted normal of a quad from its diagonals, outward by the emitted winding.
	 * @param positions A vector of vertex positions.
	 * @param quads A flat array of vertex indices where each consecutive group of 4 indices defines one quad.
	 * @param q The index of the quad to compute the normal for.
	 * @return The area-weighted normal vector of the specified quad.
	 */
	static Vector3 GetQuadNormal( const std::vector<Vector3>& positions, const std::vector<uint32_t>& quads, size_t q )
	{
		const Vector3& p0 = positions[quads[q * 4]];
		const Vector3& p1 = positions[quads[q * 4 + 1]];
		const Vector3& p2 = positions[quads[q * 4 + 2]];
		const Vector3& p3 = positions[quads[q * 4 + 3]];
		return Cross( p2 - p0, p3 - p1 );
	}

	/**
	 * @brief Creates a window over the grid with occupancy straight from the grid and field from the filtered SDF.
	 * @param field The signed distance field to use for creating the window.
	 * @return A Window object representing the grid with occupancy and field values.
	 */
	Window CreateGridWindow( const Tr2SignedDistanceField& field ) const
	{
		const Int3 dims = DimensionsToInt3( field.GetDimensions() );
		Window window( dims );
		size_t i = 0;
		for( int32_t z = 0; z < dims[2]; ++z )
		{
			for( int32_t y = 0; y < dims[1]; ++y )
			{
				for( int32_t x = 0; x < dims[0]; ++x, ++i )
				{
					const size_t index = window.Index( x, y, z );
					window.occupancy[index] = field.GetValue( i ) <= 0.0 ? 1 : 0;
					window.field[index] = field.GetValue( i );
				}
			}
		}
		return window;
	}

	/**
	 * @brief Creates a subdivided window over the grid with trilinear interpolation of the signed distance field.
	 * @param field The signed distance field to use for creating the window.
	 * @param scale The subdivision scale factor; each grid cell is subdivided into scale^3 cells.
	 * @param cancel An atomic boolean flag that can be used to cancel the operation.
	 * @return A Window object representing the subdivided grid with occupancy and field values.
	 */
	Window CreateSubdividedWindow( const Tr2SignedDistanceField& field, int32_t scale, const std::atomic<bool>& cancel ) const
	{
		const Int3 dims = DimensionsToInt3( field.GetDimensions() );
		Window window( { dims[0] * scale, dims[1] * scale, dims[2] * scale } );

		// Cubes are keyed by their minimum sample, -1..dim per axis
		const Int3 cubes = { dims[0] + 2, dims[1] + 2, dims[2] + 2 };
		const std::array<size_t, 3> cubeStrides = { 1, size_t( cubes[0] ), size_t( cubes[0] ) * cubes[1] };
		auto cubeIndex = [&]( int32_t x, int32_t y, int32_t z ) {
			return size_t( x + 1 ) + cubeStrides[1] * size_t( y + 1 ) + cubeStrides[2] * size_t( z + 1 );
		};
		std::vector<uint8_t> solid( cubeStrides[2] * cubes[2], 0 );
		std::vector<uint8_t> band( solid.size(), 0 );
		for( int32_t z = -1; z <= dims[2]; ++z )
		{
			for( int32_t y = -1; y <= dims[1]; ++y )
			{
				for( int32_t x = -1; x <= dims[0]; ++x )
				{
					bool any = false;
					bool all = true;
					for( int32_t corner = 0; corner < 8; ++corner )
					{
						const bool positive = field.GetValue( x + ( corner & 1 ), y + ( ( corner >> 1 ) & 1 ), z + ( corner >> 2 ) ) <= 0.0;
						any = any || positive;
						all = all && positive;
					}
					solid[cubeIndex( x, y, z )] = all ? 1 : 0;
					band[cubeIndex( x, y, z )] = any && !all ? 1 : 0;
				}
			}
		}
		// Chebyshev-1 dilation of the band, which is where the crossing partners of a band cell live
		for( int axis = 0; axis < 3; ++axis )
		{
			const auto mask = band;
			for( int32_t z = -1; z <= dims[2]; ++z )
			{
				for( int32_t y = -1; y <= dims[1]; ++y )
				{
					for( int32_t x = -1; x <= dims[0]; ++x )
					{
						const Int3 coord = { x, y, z };
						const size_t i = cubeIndex( x, y, z );
						if( ( coord[axis] > -1 && mask[i - cubeStrides[axis]] ) || ( coord[axis] < dims[axis] && mask[i + cubeStrides[axis]] ) )
						{
							band[i] = 1;
						}
					}
				}
			}
		}

		std::vector<float> t( scale );
		for( int32_t i = 0; i < scale; ++i )
		{
			t[i] = i / float( scale );
		}

		// Only cubes 0..dim-1 cover lattice cells
		for( int32_t cz = 0; cz < dims[2]; ++cz )
		{
			if( cancel )
			{
				return window;
			}
			for( int32_t cy = 0; cy < dims[1]; ++cy )
			{
				for( int32_t cx = 0; cx < dims[0]; ++cx )
				{
					const size_t cube = cubeIndex( cx, cy, cz );
					if( band[cube] )
					{
						// Lerped hierarchically so each cube costs 4k + 2k^2 + k^3 lerps
						const float s000 = field.GetValue( cx, cy, cz );
						const float s100 = field.GetValue( cx + 1, cy, cz );
						const float s010 = field.GetValue( cx, cy + 1, cz );
						const float s110 = field.GetValue( cx + 1, cy + 1, cz );
						const float s001 = field.GetValue( cx, cy, cz + 1 );
						const float s101 = field.GetValue( cx + 1, cy, cz + 1 );
						const float s011 = field.GetValue( cx, cy + 1, cz + 1 );
						const float s111 = field.GetValue( cx + 1, cy + 1, cz + 1 );
						for( int32_t iz = 0; iz < scale; ++iz )
						{
							const float tz = t[iz];
							const float e00 = s000 + ( s001 - s000 ) * tz;
							const float e10 = s100 + ( s101 - s100 ) * tz;
							const float e01 = s010 + ( s011 - s010 ) * tz;
							const float e11 = s110 + ( s111 - s110 ) * tz;
							for( int32_t iy = 0; iy < scale; ++iy )
							{
								const float ty = t[iy];
								const float f0 = e00 + ( e01 - e00 ) * ty;
								const float f1 = e10 + ( e11 - e10 ) * ty;
								for( int32_t ix = 0; ix < scale; ++ix )
								{
									const float value = f0 + ( f1 - f0 ) * t[ix];
									const size_t index = window.Index( cx * scale + ix, cy * scale + iy, cz * scale + iz );
									window.field[index] = value;
									window.occupancy[index] = value <= 0.0 ? 1 : 0;
								}
							}
						}
					}
					else if( solid[cube] )
					{
						for( int32_t iz = 0; iz < scale; ++iz )
						{
							for( int32_t iy = 0; iy < scale; ++iy )
							{
								const size_t index = window.Index( cx * scale, cy * scale + iy, cz * scale + iz );
								std::fill_n( window.occupancy.begin() + index, scale, uint8_t( 1 ) );
							}
						}
					}
				}
			}
		}
		return window;
	}

	/**
	 * @brief Computes the normal of a triangle defined by three points.
	 * @param p0 The first vertex of the triangle.
	 * @param p1 The second vertex of the triangle.
	 * @param p2 The third vertex of the triangle.
	 * @return The normal vector of the triangle.
	 */
	static Vector3 GetTriangleNormal( const Vector3& p0, const Vector3& p1, const Vector3& p2 )
	{
		const Vector3 a = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
		const Vector3 b = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };
		return { a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0] };
	}

	/**
	 * @brief Computes the cosine of the angle between the normals of a quad's two triangles.
	 * @param n0 The normal vector of the first triangle.
	 * @param n1 The normal vector of the second triangle.
	 * @return The cosine of the angle between the two normals, or -2 if either normal is degenerate.
	 */
	static float GetFoldCosine( const Vector3& n0, const Vector3& n1 )
	{
		const float lengths = std::sqrt( ( n0[0] * n0[0] + n0[1] * n0[1] + n0[2] * n0[2] ) * ( n1[0] * n1[0] + n1[1] * n1[1] + n1[2] * n1[2] ) );
		if( lengths <= 1e-12 )
		{
			return -2.0;
		}
		return ( n0[0] * n1[0] + n0[1] * n1[1] + n0[2] * n1[2] ) / lengths;
	}
};


/**
 * @brief Builds mesh buffers using the Surface Nets dual-vertex algorithm.
 */
class SurfaceNetsBuilder : public DualGridBuilder
{
public:
	SurfaceNetsBuilder( InputSettings inputSettings, OutputSettings outputSettings, uint32_t subdivision, uint32_t relaxIterations, const Tr2SurfaceCutParams& cutParams ) :
		DualGridBuilder( std::move( inputSettings ), outputSettings, subdivision, relaxIterations, cutParams )
	{
	}

protected:
	GeneratedMesh CreateBuffers( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel ) override
	{
		TRINITY_STATS_ZONE( __FUNCTION__ );

		return CreateBuffersImpl( GetDualVertex, allocator, buffers, cancel );
	}

private:
	static constexpr float s_tMin = 0.10; // bound of the edge crossing interpolant off 0/1

	/**
	 * @brief Calculates the position of a new vertex based on the dual contouring algorithm, which places the vertex at the average of the edge crossings of a block in a signed distance field.
	 * @param window The window containing the field and occupancy data.
	 * @param sdf The signed distance field.
	 * @param block The coordinates of the block.
	 * @param scale The scale factor.
	 * @return The position of the new vertex.
	 */
	static Vector3 GetDualVertex( const Window& window, const Tr2SignedDistanceField&, const Int3& block, int32_t scale )
	{
		const size_t base = window.Index( block[0], block[1], block[2] );
		Vector3 sum = { 0.0, 0.0, 0.0 };
		int32_t count = 0;
		for( int axis = 0; axis < 3; ++axis )
		{
			const int u = s_axisUV[axis][0];
			const int v = s_axisUV[axis][1];
			for( int32_t du = 0; du < 2; ++du )
			{
				for( int32_t dv = 0; dv < 2; ++dv )
				{
					Int3 lo = { 0, 0, 0 };
					lo[u] = du;
					lo[v] = dv;
					Int3 hi = lo;
					hi[axis] = 1;
					const size_t ia = base + lo[0] + window.strides[1] * lo[1] + window.strides[2] * lo[2];
					const size_t ib = ia + window.strides[axis];
					if( window.occupancy[ia] == window.occupancy[ib] )
					{
						continue;
					}
					const bool loSolid = window.occupancy[ia] != 0;
					const Int3& solidOffset = loSolid ? lo : hi;
					const Int3& emptyOffset = loSolid ? hi : lo;
					const float da = window.field[loSolid ? ia : ib];
					const float db = window.field[loSolid ? ib : ia];
					float t = 0.5;
					// NaN is a cell without a field value, so split the edge
					if( !std::isnan( da ) && !std::isnan( db ) && da != db )
					{
						t = std::min( std::max( da / ( da - db ), s_tMin ), float( 1.0 - s_tMin ) );
					}
					for( int j = 0; j < 3; ++j )
					{
						sum[j] += solidOffset[j] + t * ( emptyOffset[j] - solidOffset[j] );
					}
					++count;
				}
			}
		}
		Vector3 result;
		for( int j = 0; j < 3; ++j )
		{
			const float position = block[j] + sum[j] / count;
			result[j] = std::min( std::max( position, block[j] + s_roamMargin ), block[j] + 1 - s_roamMargin ) / scale;
		}
		return result;
	}
};


/**
 * @brief Builds mesh buffers using the Dual Contouring dual-vertex algorithm.
 */
class DualContouringBuilder : public DualGridBuilder
{
public:
	DualContouringBuilder( InputSettings inputSettings, OutputSettings outputSettings, uint32_t subdivision, uint32_t relaxIterations, const Tr2SurfaceCutParams& cutParams ) :
		DualGridBuilder( std::move( inputSettings ), outputSettings, subdivision, relaxIterations, cutParams )
	{
	}

protected:
	GeneratedMesh CreateBuffers( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel ) override
	{
		TRINITY_STATS_ZONE( __FUNCTION__ );

		return CreateBuffersImpl( GetDualVertex, allocator, buffers, cancel );
	}

private:
	using Mat3 = std::array<std::array<float, 3>, 3>;

	static constexpr float s_eigenTruncation = 0.01; // QEF eigenvalues below this fraction of the largest are dropped
	static constexpr int s_jacobiSweeps = 6; // cyclic Jacobi sweeps of the 3x3 QEF eigen decomposition

	/**
	 * @brief Computes a gradient vector from a signed distance field at a given cell, using direct lookup at scale 1 or sampled lookup for other scales.
	 * @param field The signed distance field to query.
	 * @param cell The integer cell coordinates used as the gradient sample position.
	 * @param scale The sampling scale factor. A value of 1 uses direct cell gradient lookup; other values use scaled sampling.
	 * @return The gradient vector at the requested position, obtained either directly or via scaled sampling.
	 */
	static Vector3 GetGradient( const Tr2SignedDistanceField& field, const Int3& cell, int32_t scale )
	{
		if( scale == 1 )
		{
			return field.GetGradient( cell[0], cell[1], cell[2] );
		}
		return field.SampleGradient( Vector3( float( cell[0] ), float( cell[1] ), float( cell[2] ) ) / float( scale ) );
	}

	/**
	 * @brief Cyclic Jacobi eigen decomposition of a symmetric 3x3 matrix.
	 * @param a The symmetric 3x3 matrix to decompose. On return, a is diagonal (the eigenvalues).
	 * @param v The matrix to store the eigenvectors. On return, the columns of v are the matching eigenvectors.
	 */
	static void Eigendecompose( Mat3& a, Mat3& v )
	{
		v = { { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } } };
		for( int sweep = 0; sweep < s_jacobiSweeps; ++sweep )
		{
			for( int p = 0; p < 2; ++p )
			{
				for( int q = p + 1; q < 3; ++q )
				{
					if( std::abs( a[p][q] ) < 1e-9 )
					{
						continue;
					}
					// Rotation in the (p, q) plane zeroing a[p][q]: a = J^T a J, v = v J
					const float theta = ( a[q][q] - a[p][p] ) / ( 2 * a[p][q] );
					const float t = ( theta >= 0 ? 1.f : -1.f ) / ( std::abs( theta ) + std::sqrt( theta * theta + 1 ) );
					const float c = 1 / std::sqrt( t * t + 1 );
					const float s = t * c;
					for( int k = 0; k < 3; ++k )
					{
						const float akp = a[k][p];
						const float akq = a[k][q];
						a[k][p] = c * akp - s * akq;
						a[k][q] = s * akp + c * akq;
					}
					for( int k = 0; k < 3; ++k )
					{
						const float apk = a[p][k];
						const float aqk = a[q][k];
						a[p][k] = c * apk - s * aqk;
						a[q][k] = s * apk + c * aqk;
					}
					for( int k = 0; k < 3; ++k )
					{
						const float vkp = v[k][p];
						const float vkq = v[k][q];
						v[k][p] = c * vkp - s * vkq;
						v[k][q] = s * vkp + c * vkq;
					}
				}
			}
		}
	}

	/**
	 * @brief Computes a dual-contouring vertex for a voxel block by finding sign-change edge crossings, fitting a local QEF with normal-equation/eigendecomposition 
	 * stabilization, and clamping the result inside the block before converting to scaled space.
	 * @param window Precomputed local sampling window containing occupancy, field values, indexing, and axis strides for the block neighborhood.
	 * @param field Signed-distance field used to evaluate gradients at crossing endpoints.
	 * @param block Integer block coordinate (minimum cell corner) where the dual vertex is generated.
	 * @param scale Grid-to-output scale factor used to map block-local coordinates to final position.
	 * @return A Vector3 position of the block's dual vertex in scaled coordinates, constrained to remain within the block interior margin.
	 */
	static Vector3 GetDualVertex( const Window& window, const Tr2SignedDistanceField& field, const Int3& block, int32_t scale )
	{
		// Minimiser of the quadric error sum( ( n_i . ( x - p_i ) )^2 ) over the sign crossings p_i on the block's 12 edges with
		// unit SDF gradients n_i. Solved about the crossings' mass point with a truncated pseudo-inverse, so flat and edge-like
		// blocks fall back toward the mass point along their unconstrained directions, then clamped to the block's inset unit
		// cube and scaled back into grid cell space

		// Crossings are relative to the block's minimum cell to keep the solve well conditioned
		const size_t base = window.Index( block[0], block[1], block[2] );
		std::array<Vector3, 12> points;
		std::array<Vector3, 12> normals;
		int32_t count = 0;
		for( int axis = 0; axis < 3; ++axis )
		{
			const int u = s_axisUV[axis][0];
			const int v = s_axisUV[axis][1];
			for( int32_t du = 0; du < 2; ++du )
			{
				for( int32_t dv = 0; dv < 2; ++dv )
				{
					Int3 lo = { 0, 0, 0 };
					lo[u] = du;
					lo[v] = dv;
					Int3 hi = lo;
					hi[axis] = 1;
					const size_t ia = base + lo[0] + window.strides[1] * lo[1] + window.strides[2] * lo[2];
					const size_t ib = ia + window.strides[axis];
					if( window.occupancy[ia] == window.occupancy[ib] )
					{
						continue;
					}
					const float da = window.field[ia];
					const float db = window.field[ib];
					float t = 0.5;
					// NaN is a cell without a field value, so split the edge
					if( !std::isnan( da ) && !std::isnan( db ) && da != db )
					{
						t = std::min( std::max( da / ( da - db ), float( 0.0 ) ), float( 1.0 ) );
					}
					const Vector3 ga = GetGradient( field, { block[0] + lo[0], block[1] + lo[1], block[2] + lo[2] }, scale );
					const Vector3 gb = GetGradient( field, { block[0] + hi[0], block[1] + hi[1], block[2] + hi[2] }, scale );
					Vector3& point = points[count];
					Vector3& normal = normals[count];
					for( int j = 0; j < 3; ++j )
					{
						point[j] = lo[j] + t * ( hi[j] - lo[j] );
						normal[j] = ga[j] + t * ( gb[j] - ga[j] );
					}
					const float length = std::sqrt( normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2] );
					if( length > 1e-6 )
					{
						normal = { normal[0] / length, normal[1] / length, normal[2] / length };
					}
					else
					{
						// Flat field, the crossing only contributes to the mass point
						normal = { 0.0, 0.0, 0.0 };
					}
					++count;
				}
			}
		}

		Vector3 massPoint = { 0.0, 0.0, 0.0 };
		for( int32_t i = 0; i < count; ++i )
		{
			for( int j = 0; j < 3; ++j )
			{
				massPoint[j] += points[i][j] / count;
			}
		}

		// Normal equations A^T A x = A^T b about the mass point, b_i = n_i . ( p_i - massPoint )
		Mat3 ata = {};
		Vector3 atb = { 0.0, 0.0, 0.0 };
		for( int32_t i = 0; i < count; ++i )
		{
			const auto& n = normals[i];
			const float b = n[0] * ( points[i][0] - massPoint[0] ) + n[1] * ( points[i][1] - massPoint[1] ) + n[2] * ( points[i][2] - massPoint[2] );
			for( int r = 0; r < 3; ++r )
			{
				for( int c = 0; c < 3; ++c )
				{
					ata[r][c] += n[r] * n[c];
				}
				atb[r] += n[r] * b;
			}
		}

		Mat3 eigenvectors;
		Eigendecompose( ata, eigenvectors );
		const float largest = std::max( { ata[0][0], ata[1][1], ata[2][2] } );

		// x = massPoint + V diag( 1 / lambda, truncated ) V^T A^T b
		Vector3 offset = { 0.0, 0.0, 0.0 };
		for( int k = 0; k < 3; ++k )
		{
			const float lambda = ata[k][k];
			if( lambda <= s_eigenTruncation * largest || lambda <= 1e-6 )
			{
				continue;
			}
			const float projection = ( eigenvectors[0][k] * atb[0] + eigenvectors[1][k] * atb[1] + eigenvectors[2][k] * atb[2] ) / lambda;
			for( int j = 0; j < 3; ++j )
			{
				offset[j] += eigenvectors[j][k] * projection;
			}
		}

		Vector3 result;
		for( int j = 0; j < 3; ++j )
		{
			const float position = std::min( std::max( massPoint[j] + offset[j], s_roamMargin ), float( 1.0 - s_roamMargin ) );
			result[j] = ( block[j] + position ) / scale;
		}
		return result;
	}
};


}


BlueStdResult Tr2VoxelGeometry::Create( Be::Optional<Tr2VoxelData*> voxelData, Be::Optional<int32_t> shapeChannelIndex, Be::Optional<int32_t> occupancyChannelIndex )
{
	auto shapeChannel = shapeChannelIndex.IsAssigned() ? shapeChannelIndex.GetValue() : -1;
	auto occupancyChannel = occupancyChannelIndex.IsAssigned() ? occupancyChannelIndex.GetValue() : -1;
	if( m_voxelData == voxelData && SameChannelIndex( m_shapeChannelIndex, shapeChannel ) && SameChannelIndex( m_occupancyChannelIndex, occupancyChannel ) )
	{
		return {};
	}

	if( voxelData.GetValue() )
	{
		if( shapeChannel >= 0 )
		{
			if( shapeChannel >= voxelData.GetValue()->GetPayloadChannelCount() )
			{
				return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Invalid shape channel index" );
			}
			if( !Tr2SignedDistanceField::IsValidSdfChannelType( voxelData.GetValue()->GetPayloadType( shapeChannel ) ) )
			{
				return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Not supported shape channel type" );
			}
		}

		if( occupancyChannel >= 0 )
		{
			if( occupancyChannel >= voxelData.GetValue()->GetPayloadChannelCount() )
			{
				return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Invalid occupancy channel index" );
			}
			if( !Tr2SignedDistanceField::IsValidOccupancyChannelType( voxelData.GetValue()->GetPayloadType( occupancyChannel ) ) )
			{
				return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Not supported occupancy channel type" );
			}
		}
	}

	m_shapeChannelIndex = shapeChannel;
	m_occupancyChannelIndex = occupancyChannel;
	if( m_voxelData )
	{
		m_voxelData->m_onDataChanged.UnregisterListener( this );
	}
	m_voxelData = voxelData;
	if( m_voxelData )
	{
		m_voxelData->m_onDataChanged.RegisterListener<Tr2VoxelGeometry, &Tr2VoxelGeometry::DataModified>( this );
	}
	TryRebuild();
	return {};
}

bool Tr2VoxelGeometry::Initialize()
{
	Rebuild();
	return true;
}

bool Tr2VoxelGeometry::OnModified( Be::Var* )
{
	TryRebuild();
	return true;
}

IRootPtr Tr2VoxelGeometry::GetVoxelData() const
{
	return m_voxelData.p;
}

void Tr2VoxelGeometry::SetVoxelData( Tr2VoxelData* voxelData )
{
	if( m_voxelData == voxelData )
	{
		return;
	}
	if( m_voxelData )
	{
		m_voxelData->m_onDataChanged.UnregisterListener( this );
	}
	m_voxelData = voxelData;
	if( m_voxelData )
	{
		m_voxelData->m_onDataChanged.RegisterListener<Tr2VoxelGeometry, &Tr2VoxelGeometry::DataModified>( this );
	}
	TryRebuild();
}

int32_t Tr2VoxelGeometry::GetShapeChannelIndex() const
{
	return m_shapeChannelIndex;
}

void Tr2VoxelGeometry::SetShapeChannelIndex( int32_t channelIndex )
{
	if( SameChannelIndex( m_shapeChannelIndex, channelIndex ) )
	{
		m_shapeChannelIndex = channelIndex;
		return;
	}
	m_shapeChannelIndex = channelIndex;
	TryRebuild();
}

float Tr2VoxelGeometry::GetShapeChannelScale() const
{
	return m_shapeTransform.scale;
}

void Tr2VoxelGeometry::SetShapeChannelScale( float scale )
{
	if( m_shapeTransform.scale == scale )
	{
		return;
	}
	m_shapeTransform.scale = scale;
	TryRebuild();
}

float Tr2VoxelGeometry::GetShapeChannelOffset() const
{
	return m_shapeTransform.offset;
}

void Tr2VoxelGeometry::SetShapeChannelOffset( float offset )
{
	if( m_shapeTransform.offset == offset )
	{
		return;
	}
	m_shapeTransform.offset = offset;
	TryRebuild();
}


int32_t Tr2VoxelGeometry::GetOccupancyChannelIndex() const
{
	return m_occupancyChannelIndex;
}

void Tr2VoxelGeometry::SetOccupancyChannelIndex( int32_t channelIndex )
{
	if( SameChannelIndex( m_occupancyChannelIndex, channelIndex ) )
	{
		m_occupancyChannelIndex = channelIndex;
		return;
	}
	m_occupancyChannelIndex = channelIndex;
	TryRebuild();
}

int32_t Tr2VoxelGeometry::GetDataTagChannelIndex() const
{
	return m_dataTagChannelIndex;
}

void Tr2VoxelGeometry::SetDataTagChannelIndex( int32_t channelIndex )
{
	if( SameChannelIndex( m_dataTagChannelIndex, channelIndex ) )
	{
		m_dataTagChannelIndex = channelIndex;
		return;
	}
	m_dataTagChannelIndex = channelIndex;
	TryRebuild();
}

uint32_t Tr2VoxelGeometry::GetSubdivision() const
{
	return m_subdivision;
}

void Tr2VoxelGeometry::SetSubdivision( uint32_t subdivision )
{
	if( m_subdivision == subdivision )
	{
		return;
	}
	m_subdivision = subdivision;
	TryRebuild();
}

uint32_t Tr2VoxelGeometry::GetRelaxIterations() const
{
	return m_relaxIterations;
}

void Tr2VoxelGeometry::SetRelaxIterations( uint32_t relaxIterations )
{
	if( m_relaxIterations == relaxIterations )
	{
		return;
	}
	m_relaxIterations = relaxIterations;
	TryRebuild();
}

uint32_t Tr2VoxelGeometry::GetLodLevels() const
{
	return m_lodLevels;
}

void Tr2VoxelGeometry::SetLodLevels( uint32_t lodLevels )
{
	lodLevels = std::max( lodLevels, uint32_t( 1 ) );
	if( m_lodLevels == lodLevels )
	{
		return;
	}
	m_lodLevels = lodLevels;
	TryRebuild();
}

uint32_t Tr2VoxelGeometry::GetMethod() const
{
	return static_cast<uint32_t>( m_method );
}

void Tr2VoxelGeometry::SetMethod( Method method )
{
	if( m_method == method )
	{
		return;
	}
	m_method = method;
	TryRebuild();
}

uint32_t Tr2VoxelGeometry::GetCutSeed() const
{
	return m_cutSeed;
}

void Tr2VoxelGeometry::SetCutSeed( uint32_t cutSeed )
{
	if( m_cutSeed == cutSeed )
	{
		return;
	}
	m_cutSeed = cutSeed;
	TryRebuild();
}

float Tr2VoxelGeometry::GetFacetSpacing() const
{
	return m_facetSpacing;
}

void Tr2VoxelGeometry::SetFacetSpacing( float facetSpacing )
{
	if( m_facetSpacing == facetSpacing )
	{
		return;
	}
	m_facetSpacing = facetSpacing;
	TryRebuild();
}

float Tr2VoxelGeometry::GetFacetDepth() const
{
	return m_facetDepth;
}

void Tr2VoxelGeometry::SetFacetDepth( float facetDepth )
{
	if( m_facetDepth == facetDepth )
	{
		return;
	}
	m_facetDepth = facetDepth;
	TryRebuild();
}

float Tr2VoxelGeometry::GetFacetMaxCut() const
{
	return m_facetMaxCut;
}

void Tr2VoxelGeometry::SetFacetMaxCut( float facetMaxCut )
{
	if( m_facetMaxCut == facetMaxCut )
	{
		return;
	}
	m_facetMaxCut = facetMaxCut;
	TryRebuild();
}

float Tr2VoxelGeometry::GetCleaveDepth() const
{
	return m_cleaveDepth;
}

void Tr2VoxelGeometry::SetCleaveDepth( float cleaveDepth )
{
	if( m_cleaveDepth == cleaveDepth )
	{
		return;
	}
	m_cleaveDepth = cleaveDepth;
	TryRebuild();
}

bool Tr2VoxelGeometry::GetPackedTangents() const
{
	return m_packedTangents;
}

void Tr2VoxelGeometry::SetPackedTangents( bool packedTangents )
{
	if( m_packedTangents == packedTangents )
	{
		return;
	}
	m_packedTangents = packedTangents;
	TryRebuild();
}

void Tr2VoxelGeometry::LockBuild()
{
	++m_buildLock;
}

void Tr2VoxelGeometry::UnlockBuild()
{
	CCP_ASSERT_M( m_buildLock > 0, "UnlockBuild called without matching LockBuild" );
	--m_buildLock;
	if( m_buildLock == 0 && m_rebuildPending )
	{
		m_rebuildPending = false;
		Rebuild();
	}
}

void Tr2VoxelGeometry::DataModified( Tr2VoxelData::ModifiedChannels modifiedChannels )
{
	const bool dataTagsModified = m_components.HasBit( Tr2VoxelGeometryComponents::DataTags ) && modifiedChannels.IsChannelModified( m_dataTagChannelIndex );
	if( modifiedChannels.IsChannelModified( m_shapeChannelIndex ) || modifiedChannels.IsChannelModified( m_occupancyChannelIndex ) || dataTagsModified )
	{
		TryRebuild();
	}
}

void Tr2VoxelGeometry::TryRebuild()
{
	if( m_buildLock > 0 )
	{
		m_rebuildPending = true;
		return;
	}
	m_rebuildPending = false;
	Rebuild();
}

std::unique_ptr<Tr2ProceduralGeometryBuilder> Tr2VoxelGeometry::CreateBuilder() const
{
	if( !m_voxelData )
	{
		return std::make_unique<EmptyBuilder>();
	}

	// Shape is defined by an SDF channel, or occupancy, or both.
	auto shapeSnapshot = m_voxelData->GetPayloadSnapshot( m_shapeChannelIndex );
	if( shapeSnapshot.m_data && !Tr2SignedDistanceField::IsValidSdfChannelType( shapeSnapshot.m_type ) )
	{
		return std::make_unique<EmptyBuilder>();
	}
	auto occupancySnapshot = m_voxelData->GetPayloadSnapshot( m_occupancyChannelIndex );
	if( occupancySnapshot.m_data && !Tr2SignedDistanceField::IsValidOccupancyChannelType( occupancySnapshot.m_type ) )
	{
		return std::make_unique<EmptyBuilder>();
	}
	if( !shapeSnapshot.m_data && !occupancySnapshot.m_data )
	{
		return std::make_unique<EmptyBuilder>();
	}
	// An unusable data tag channel leaves the tags zero rather than failing the build
	auto dataTagSnapshot = m_voxelData->GetPayloadSnapshot( m_dataTagChannelIndex );
	if( dataTagSnapshot.m_data && !IsValidDataTagChannelType( dataTagSnapshot.m_type ) )
	{
		CCP_LOGWARN( "Tr2VoxelGeometry: data tag channel %d is not a one-component float channel", m_dataTagChannelIndex );
	}

	Tr2SurfaceCutParams cutParams;
	cutParams.seed = m_cutSeed;
	cutParams.facetSpacing = m_facetSpacing;
	cutParams.facetDepth = m_facetDepth;
	cutParams.facetMaxCut = m_facetMaxCut;
	cutParams.cleaveDepth = m_cleaveDepth;

	auto inputSettings = InputSettings{
		std::move( shapeSnapshot ), m_shapeTransform, std::move( occupancySnapshot ), std::move( dataTagSnapshot )
	};
	auto outputSettings = OutputSettings{
		m_components, m_lodLevels, m_packedTangents
	};

	switch( m_method )
	{
	case Method::Simple:
		return std::make_unique<SimpleBuilder>( std::move( inputSettings ), outputSettings );
	case Method::SurfaceNets:
		return std::make_unique<SurfaceNetsBuilder>( std::move( inputSettings ), outputSettings, m_subdivision, m_relaxIterations, cutParams );
	case Method::DualContouring:
		return std::make_unique<DualContouringBuilder>( std::move( inputSettings ), outputSettings, m_subdivision, m_relaxIterations, cutParams );
	default:
		return std::make_unique<EmptyBuilder>();
	}
}

BlueStdResult Tr2VoxelGeometry::TestBuildFile( const char* resPath )
{
	auto builder = CreateBuilder();

	cmf::MemoryAllocator allocator;
	cmf::BufferManager buffers( allocator );
	cmf::Data data;
	std::atomic_bool cancel = false;

	if( !builder->BuildCmf( allocator, buffers, cancel, data ) )
	{
		return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR );
	}

	auto contents = cmf::BuildFile( data, buffers );
	auto path = BePaths->ResolvePathForWritingW( static_cast<const wchar_t*>( CA2W( resPath ) ) );

	FILE* file = nullptr;
	fopen_s( &file, CW2A( path.c_str() ), "wb" );
	if( !file )
	{
		return BlueStdResult( BLUE_STD_RESULT_OS_ERROR );
	}
	if( fwrite( contents.data(), 1, contents.size(), file ) != contents.size() )
	{
		fclose( file );
		return BlueStdResult( BLUE_STD_RESULT_IO_ERROR );
	}
	fclose( file );
	return {};
}