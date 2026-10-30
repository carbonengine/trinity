// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2VoxelGeometry.h"
#include "../Utilities/Vector3d.h"
#include "../Tr2SignedDistanceField.h"
#include <cmf/writer.h>


namespace
{
class EmptyBuilder : public Tr2ProceduralGeometryBuilder
{
public:
	bool BuildCmf( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel, cmf::Data& data ) override
	{
		return false;
	}
};

class VoxelBuilder : public Tr2ProceduralGeometryBuilder
{
public:
	VoxelBuilder( Tr2VoxelData::PayloadSnapshot shape, float cellSize ) :
		m_sdf( std::move( shape ) ),
		m_cellSize( cellSize ),
		m_dimensions( m_sdf.m_voxelData->GetDimensions() )
	{
	}

	bool BuildCmf( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel, cmf::Data& data ) override
	{
		if( cancel )
		{
			return false;
		}

		CcpMath::AxisAlignedBox meshBounds;
		auto [vbView, ibView, decl] = CreateBuffers( allocator, buffers, cancel, meshBounds );
		if( cancel || vbView.size == 0 )
		{
			return false;
		}

		cmf::LodMeshArea lodArea = {};
		lodArea.firstElement = 0;
		lodArea.elementCount = ibView.size / ibView.stride / 3;

		cmf::MeshLod lod = {};
		lod.vb = vbView;
		lod.ib = ibView;
		cmf::Modify( lod.areas, allocator ).emplace_back( lodArea );

		cmf::MeshArea area = {};
		area.name = allocator.AllocateString( "VoxelDataArea" );
		area.bounds = meshBounds;

		cmf::Mesh mesh = {};
		mesh.name = allocator.AllocateString( "VoxelData" );
		mesh.decl = decl;
		cmf::Modify( mesh.lods, allocator ).emplace_back( lod );
		cmf::Modify( mesh.areas, allocator ).emplace_back( area );
		mesh.topology = cmf::MeshTopology::TriangleList;
		mesh.bounds = meshBounds;

		data = {};
		cmf::Modify( data.meshes, allocator ).emplace_back( mesh );

		return true;
	}

protected:

	// Returns vertex and index buffer views, and the bounds of the generated vertices in bounds
	virtual std::tuple<cmf::BufferView, cmf::BufferView, cmf::Span<cmf::VertexElement>> CreateBuffers( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel, CcpMath::AxisAlignedBox& bounds ) = 0;

	const Tr2VoxelData::PayloadSnapshot m_sdf;
	const float m_cellSize;
	const Tr2VoxelData::Dimensions m_dimensions;
};

class SimpleBuilder : public VoxelBuilder
{
public:
	SimpleBuilder( Tr2VoxelData::PayloadSnapshot shape, float cellSize ) :
		VoxelBuilder( std::move( shape ), cellSize )
	{
	}

protected:
	struct Vertex
	{
		Vector3 position;
		Vector3 normal;
	};

	static CcpMath::AxisAlignedBox GetBounds( const std::vector<Vertex>& vertices )
	{
		if( vertices.empty() )
		{
			return CcpMath::AxisAlignedBox( Vector3( 0.f, 0.f, 0.f ), Vector3( 0.f, 0.f, 0.f ) );
		}
		Vector3 minimum = vertices[0].position;
		Vector3 maximum = vertices[0].position;
		for( const auto& vertex : vertices )
		{
			minimum = Vector3( std::min( minimum.x, vertex.position.x ), std::min( minimum.y, vertex.position.y ), std::min( minimum.z, vertex.position.z ) );
			maximum = Vector3( std::max( maximum.x, vertex.position.x ), std::max( maximum.y, vertex.position.y ), std::max( maximum.z, vertex.position.z ) );
		}
		return CcpMath::AxisAlignedBox( minimum, maximum );
	}

	std::tuple<cmf::BufferView, cmf::BufferView, cmf::Span<cmf::VertexElement>> CreateBuffers( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel, CcpMath::AxisAlignedBox& bounds ) override
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
		const Vector3 origin = Vector3( float( width ), float( height ), float( depth ) ) * -0.5f;

		auto sdf = Tr2SignedDistanceField( m_sdf, {}, cancel );

		auto isOccupied = [&]( const Tr2VoxelData::Coord& coord ) {
			if( coord.x < 0 || coord.y < 0 || coord.z < 0 || coord.x >= width || coord.y >= height || coord.z >= depth )
			{
				return false;
			}
			return sdf.GetValue( coord.x, coord.y, coord.z ) <= 0.f;
		};

		// Vertices are shared between faces only if they have the same grid corner and the same normal (hard edges)
		std::unordered_map<uint64_t, uint32_t> indicesMap;
		std::vector<uint32_t> indices;
		std::vector<Vertex> vertices;

		auto addVertex = [&]( const Tr2VoxelData::Coord& corner, size_t faceIndex ) {
			uint64_t cornerIndex = uint64_t( corner.x ) + uint64_t( width + 1 ) * ( uint64_t( corner.y ) + uint64_t( height + 1 ) * uint64_t( corner.z ) );
			auto [iter, inserted] = indicesMap.try_emplace( cornerIndex * s_faces.size() + faceIndex, uint32_t( vertices.size() ) );
			if( inserted )
			{
				Vector3 position = ( origin + Vector3( float( corner.x ), float( corner.y ), float( corner.z ) ) ) * m_cellSize;
				vertices.push_back( { position, s_faces[faceIndex].normal } );
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

		bounds = GetBounds( vertices );

		// Allocate vertex buffer
		const uint32_t vertexStride = sizeof( Vertex );
		cmf::BufferView vbView = buffers.AllocateBuffer(
			vertices.data(),
			uint32_t( vertices.size() * vertexStride ),
			vertexStride );

		// Allocate index buffer
		const uint32_t indexStride = sizeof( uint32_t );
		cmf::BufferView ibView = buffers.AllocateBuffer(
			indices.data(),
			uint32_t( indices.size() * indexStride ),
			indexStride );

		cmf::Span<cmf::VertexElement> decl;
		cmf::Modify( decl, allocator ).emplace_back( cmf::Usage::Position, uint8_t( 0 ), cmf::ElementType::Float32, uint8_t( 3 ), 0u );
		cmf::Modify( decl, allocator ).emplace_back( cmf::Usage::Normal, uint8_t( 0 ), cmf::ElementType::Float32, uint8_t( 3 ), uint32_t( sizeof( float ) * 3 ) );

		return { vbView, ibView, decl };
	}
};


class SurfaceNetsBuilder : public VoxelBuilder
{
public:
	SurfaceNetsBuilder( Tr2VoxelData::PayloadSnapshot sdf, float cellSize, uint32_t subdivision, uint32_t relaxIterations ) :
		VoxelBuilder( std::move( sdf ), cellSize ),
		m_subdivision( subdivision ),
		m_relaxIterations( relaxIterations )
	{
	}

protected:
	struct Vertex
	{
		Vector3 position;
		Vector3 normal;
	};
	using Float = float;
	using VectorT = Vector3;

	static CcpMath::AxisAlignedBox GetBounds( const std::vector<Vertex>& vertices )
	{
		if( vertices.empty() )
		{
			return CcpMath::AxisAlignedBox( Vector3( 0.f, 0.f, 0.f ), Vector3( 0.f, 0.f, 0.f ) );
		}
		Vector3 minimum = vertices[0].position;
		Vector3 maximum = vertices[0].position;
		for( const auto& vertex : vertices )
		{
			minimum = Vector3( std::min( minimum.x, vertex.position.x ), std::min( minimum.y, vertex.position.y ), std::min( minimum.z, vertex.position.z ) );
			maximum = Vector3( std::max( maximum.x, vertex.position.x ), std::max( maximum.y, vertex.position.y ), std::max( maximum.z, vertex.position.z ) );
		}
		return CcpMath::AxisAlignedBox( minimum, maximum );
	}

	// Surface Nets extraction. Cell centres sit at integer lattice coordinates, a dual
	// vertex lives in every 2x2x2 block of cells with mixed occupancy, and every occupancy sign-change edge emits the quad
	// joining the four blocks sharing it. Cells outside the grid read as empty, so the surface closes at the grid boundary.
	// m_subdivision > 1 runs the construction on a lattice subdivided k-fold whose field is the trilinear interpolation of
	// the SDF. Quads are emitted as two triangles each.
	std::tuple<cmf::BufferView, cmf::BufferView, cmf::Span<cmf::VertexElement>> CreateBuffers( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel, CcpMath::AxisAlignedBox& bounds ) override
	{
		std::chrono::time_point<std::chrono::steady_clock> startTime = std::chrono::steady_clock::now();
		ON_BLOCK_EXIT( [&]() {
			std::chrono::time_point<std::chrono::steady_clock> endTime = std::chrono::steady_clock::now();
			std::chrono::duration<double> elapsedSeconds = endTime - startTime;
			CCP_LOG( "SurfaceNetsBuilder: elapsed time %.3f ms\n", elapsedSeconds.count() * 1000.0 );
		} );
		if( m_dimensions.width == 0 || m_dimensions.height == 0 || m_dimensions.depth == 0 )
		{
			return {};
		}
		const int32_t scale = int32_t( std::max( m_subdivision, 1u ) );

		auto field = Tr2SignedDistanceField( m_sdf, {}, cancel );
		if( cancel )
		{
			return {};
		}
		auto window = scale == 1 ? CreateGridWindow( field ) : CreateSubdividedWindow( field, scale, cancel );
		if( cancel )
		{
			return {};
		}

		std::vector<VectorT> positions;
		std::vector<uint32_t> quads;
		Sweep( window, scale, cancel, positions, quads );
		if( cancel || quads.empty() )
		{
			return {};
		}
		Relax( positions, quads, cancel );
		if( cancel )
		{
			return {};
		}
		auto normals = ComputeNormals( positions, quads );

		// Lattice coordinates put cell centres at integers; the grid spans [-0.5, dim - 0.5] and is centred on the origin
		const Vector3 origin = Vector3( float( m_dimensions.width ), float( m_dimensions.height ), float( m_dimensions.depth ) ) * -0.5f;
		std::vector<Vertex> vertices( positions.size() );
		for( size_t i = 0; i < positions.size(); ++i )
		{
			const auto& p = positions[i];
			const auto& n = normals[i];
			vertices[i].position = ( origin + Vector3( float( p[0] + 0.5 ), float( p[1] + 0.5 ), float( p[2] + 0.5 ) ) ) * m_cellSize;
			vertices[i].normal = Vector3( float( n[0] ), float( n[1] ), float( n[2] ) );
		}

		std::vector<uint32_t> indices;
		indices.reserve( quads.size() / 4 * 6 );
		for( size_t q = 0; q < quads.size(); q += 4 )
		{
			indices.insert( indices.end(), { quads[q], quads[q + 1], quads[q + 2], quads[q], quads[q + 2], quads[q + 3] } );
		}

		bounds = GetBounds( vertices );

		// Allocate vertex buffer
		const uint32_t vertexStride = sizeof( Vertex );
		cmf::BufferView vbView = buffers.AllocateBuffer(
			vertices.data(),
			uint32_t( vertices.size() * vertexStride ),
			vertexStride );

		// Allocate index buffer
		const uint32_t indexStride = sizeof( uint32_t );
		cmf::BufferView ibView = buffers.AllocateBuffer(
			indices.data(),
			uint32_t( indices.size() * indexStride ),
			indexStride );

		cmf::Span<cmf::VertexElement> decl;
		cmf::Modify( decl, allocator ).emplace_back( cmf::Usage::Position, uint8_t( 0 ), cmf::ElementType::Float32, uint8_t( 3 ), 0u );
		cmf::Modify( decl, allocator ).emplace_back( cmf::Usage::Normal, uint8_t( 0 ), cmf::ElementType::Float32, uint8_t( 3 ), uint32_t( sizeof( float ) * 3 ) );

		return { vbView, ibView, decl };
	}

private:
	using Vec3d = std::array<Float, 3>;
	using Int3 = std::array<int32_t, 3>;

	static constexpr Float s_relaxAlpha = 0.5; // Laplacian relaxation strength per iteration
	static constexpr Float s_roamMargin = 0.015; // dual vertex clamp cube inset, keeps neighbouring vertices apart
	static constexpr Float s_tMin = 0.10; // bound of the edge crossing interpolant off 0/1

	// (u, v) per axis, right-handed: u x v = +axis
	static constexpr std::array<std::array<int, 2>, 3> s_axisUV = { { { 1, 2 }, { 2, 0 }, { 0, 1 } } };

	// Occupancy and field over the lattice plus a one cell ring of empty cells (lattice coordinates -1..lattice)
	struct Window
	{
		explicit Window( const Int3& lattice_ ) :
			lattice( lattice_ ),
			strides( { 1, size_t( lattice_[0] + 2 ), size_t( lattice_[0] + 2 ) * size_t( lattice_[1] + 2 ) } ),
			occupancy( strides[2] * size_t( lattice_[2] + 2 ), 0 ),
			field( occupancy.size(), std::numeric_limits<Float>::quiet_NaN() )
		{
		}

		size_t Index( int32_t x, int32_t y, int32_t z ) const
		{
			return size_t( x + 1 ) + strides[1] * size_t( y + 1 ) + strides[2] * size_t( z + 1 );
		}

		const Int3 lattice;
		const std::array<size_t, 3> strides;
		std::vector<uint8_t> occupancy;
		std::vector<Float> field; // NaN where the cell has no field value
	};

	Int3 GetGridSize() const
	{
		return { Tr2VoxelData::CoordType( m_dimensions.width ), Tr2VoxelData::CoordType( m_dimensions.height ), Tr2VoxelData::CoordType( m_dimensions.depth ) };
	}

	// Lattice equal to the grid: occupancy straight from the grid, field from the filtered SDF
	Window CreateGridWindow( const Tr2SignedDistanceField& field ) const
	{
		const Int3 dims = GetGridSize();
		Window window( dims );
		size_t i = 0;
		for( int32_t z = 0; z < dims[2]; ++z )
		{
			for( int32_t y = 0; y < dims[1]; ++y )
			{
				for( int32_t x = 0; x < dims[0]; ++x, ++i )
				{
					const size_t index = window.Index( x, y, z );
					window.occupancy[index] = field.GetValue(i) <= 0.0 ? 1 : 0;
					window.field[index] = field.GetValue(i);
				}
			}
		}
		return window;
	}

	// Lattice of dim * scale cells per axis, each the trilinear interpolation of the 8 grid samples of its cube (samples outside
	// the grid read deep empty). Only cubes straddling a sign change, plus one cube of dilation, get the real per-cell trilinear;
	// the rest take the cube's uniform sign and have no field value.
	Window CreateSubdividedWindow( const Tr2SignedDistanceField& field, int32_t scale, const std::atomic<bool>& cancel ) const
	{
		const Int3 dims = GetGridSize();
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

		std::vector<Float> t( scale );
		for( int32_t i = 0; i < scale; ++i )
		{
			t[i] = i / Float( scale );
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
						const Float s000 = field.GetValue( cx, cy, cz );
						const Float s100 = field.GetValue( cx + 1, cy, cz );
						const Float s010 = field.GetValue( cx, cy + 1, cz );
						const Float s110 = field.GetValue( cx + 1, cy + 1, cz );
						const Float s001 = field.GetValue( cx, cy, cz + 1 );
						const Float s101 = field.GetValue( cx + 1, cy, cz + 1 );
						const Float s011 = field.GetValue( cx, cy + 1, cz + 1 );
						const Float s111 = field.GetValue( cx + 1, cy + 1, cz + 1 );
						for( int32_t iz = 0; iz < scale; ++iz )
						{
							const Float tz = t[iz];
							const Float e00 = s000 + ( s001 - s000 ) * tz;
							const Float e10 = s100 + ( s101 - s100 ) * tz;
							const Float e01 = s010 + ( s011 - s010 ) * tz;
							const Float e11 = s110 + ( s111 - s110 ) * tz;
							for( int32_t iy = 0; iy < scale; ++iy )
							{
								const Float ty = t[iy];
								const Float f0 = e00 + ( e01 - e00 ) * ty;
								const Float f1 = e10 + ( e11 - e10 ) * ty;
								for( int32_t ix = 0; ix < scale; ++ix )
								{
									const Float value = f0 + ( f1 - f0 ) * t[ix];
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

	// Mean of the sign crossings on the block's 12 edges, clamped to the block's inset unit cube and scaled back into
	// grid cell space
	static VectorT GetDualVertex( const Window& window, const Int3& block, int32_t scale )
	{
		const size_t base = window.Index( block[0], block[1], block[2] );
		VectorT sum = { 0.0, 0.0, 0.0 };
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
					const Float da = window.field[loSolid ? ia : ib];
					const Float db = window.field[loSolid ? ib : ia];
					Float t = 0.5;
					// NaN is a cell without a field value, so split the edge
					if( !std::isnan( da ) && !std::isnan( db ) && da != db )
					{
						t = std::min( std::max( da / ( da - db ), s_tMin ), Float( 1.0 - s_tMin ) );
					}
					for( int j = 0; j < 3; ++j )
					{
						sum[j] += solidOffset[j] + t * ( emptyOffset[j] - solidOffset[j] );
					}
					++count;
				}
			}
		}
		VectorT result;
		for( int j = 0; j < 3; ++j )
		{
			const Float position = block[j] + sum[j] / count;
			result[j] = std::min( std::max( position, block[j] + s_roamMargin ), block[j] + 1 - s_roamMargin ) / scale;
		}
		return result;
	}

	// Emits a quad for every occupancy sign change, as 4 indices into shared dual vertices (one per block), wound
	// counter-clockwise seen from the empty side
	static void Sweep( const Window& window, int32_t scale, const std::atomic<bool>& cancel, std::vector<VectorT>& positions, std::vector<uint32_t>& quads )
	{
		const Int3& lattice = window.lattice;
		// Blocks are keyed by their minimum cell, -1..lattice-1 per axis
		const std::array<size_t, 3> blockStrides = { 1, size_t( lattice[0] + 1 ), size_t( lattice[0] + 1 ) * size_t( lattice[1] + 1 ) };
		std::vector<uint32_t> blockVertices( blockStrides[2] * size_t( lattice[2] + 1 ), std::numeric_limits<uint32_t>::max() );

		auto getVertex = [&]( const Int3& block ) {
			auto& vertex = blockVertices[size_t( block[0] + 1 ) + blockStrides[1] * size_t( block[1] + 1 ) + blockStrides[2] * size_t( block[2] + 1 )];
			if( vertex == std::numeric_limits<uint32_t>::max() )
			{
				vertex = uint32_t( positions.size() );
				positions.push_back( GetDualVertex( window, block, scale ) );
			}
			return vertex;
		};

		// Block fan around the edge, counter-clockwise seen from +axis
		static const std::array<std::array<int32_t, 2>, 4> s_quadUV = { { { -1, -1 }, { 0, -1 }, { 0, 0 }, { -1, 0 } } };

		for( int axis = 0; axis < 3; ++axis )
		{
			const int u = s_axisUV[axis][0];
			const int v = s_axisUV[axis][1];
			// Edges between a cell and its +axis neighbour; the axis coordinate runs -1..lattice-1 so boundary faces are emitted
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
						std::array<uint32_t, 4> quad;
						for( size_t i = 0; i < 4; ++i )
						{
							Int3 block = { x, y, z };
							block[u] += s_quadUV[i][0];
							block[v] += s_quadUV[i][1];
							quad[i] = getVertex( block );
						}
						if( !window.occupancy[index] )
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

	// Jacobi Laplacian smoothing: each vertex moves a fraction s_relaxAlpha toward the mean of its quad-perimeter neighbours
	void Relax( std::vector<VectorT>& positions, const std::vector<uint32_t>& quads, const std::atomic<bool>& cancel ) const
	{
		if( m_relaxIterations == 0 || quads.empty() )
		{
			return;
		}
		std::vector<std::vector<uint32_t>> neighbors( positions.size() );
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

		std::vector<VectorT> moved( positions.size() );
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
				VectorT sum = { 0.0, 0.0, 0.0 };
				for( auto neighbor : neighbors[i] )
				{
					sum += positions[neighbor];
				}
				const Float count = Float( neighbors[i].size() );
				for( int j = 0; j < 3; ++j )
				{
					moved[i][j] = p[j] + s_relaxAlpha * ( sum[j] / count - p[j] );
				}
			}
			positions.swap( moved );
		}
	}

	// Area-weighted vertex normals: every quad adds its diagonal cross product (outward by the emitted winding) to each of
	// its four corners, then each vertex normalises its sum
	static std::vector<VectorT> ComputeNormals( const std::vector<VectorT>& positions, const std::vector<uint32_t>& quads )
	{
		std::vector<VectorT> normals( positions.size(), VectorT{ 0.0, 0.0, 0.0 } );
		for( size_t q = 0; q < quads.size(); q += 4 )
		{
			const auto& p0 = positions[quads[q]];
			const auto& p1 = positions[quads[q + 1]];
			const auto& p2 = positions[quads[q + 2]];
			const auto& p3 = positions[quads[q + 3]];
			const VectorT a = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };
			const VectorT b = { p3[0] - p1[0], p3[1] - p1[1], p3[2] - p1[2] };
			const VectorT n = { a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0] };
			for( size_t i = 0; i < 4; ++i )
			{
				auto& normal = normals[quads[q + i]];
				for( int j = 0; j < 3; ++j )
				{
					normal[j] += n[j];
				}
			}
		}
		for( auto& normal : normals )
		{
			const Float length = std::sqrt( normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2] );
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

	const uint32_t m_subdivision;
	const uint32_t m_relaxIterations;
};



}


BlueStdResult Tr2VoxelGeometry::Create( Be::Optional<Tr2VoxelData*> voxelData, Be::Optional<uint32_t> shapeChannelIndex, Be::Optional<float> cellSize )
{
	auto size = cellSize.IsAssigned() ? cellSize.GetValue() : 1.f;
	if( m_voxelData == voxelData && m_shapeChannelIndex == shapeChannelIndex && m_cellSize == size )
	{
		return {};
	}

	if ( voxelData.GetValue() )
	{
		if( shapeChannelIndex >= voxelData.GetValue()->GetPayloadChannelCount() )
		{
			return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Invalid shape channel index" );
		}
		auto& type = voxelData.GetValue()->GetPayloadType( shapeChannelIndex );
		if( !Tr2SignedDistanceField::IsValidSdfChannelType( type ) )
		{
			return BlueStdResult( BLUE_STD_RESULT_VALUE_ERROR, "Not supported shape channel type" );
		}
	}

	m_shapeChannelIndex = shapeChannelIndex;
	m_cellSize = size;
	if( m_voxelData )
	{
		m_voxelData->m_onDataChanged.UnregisterListener( this );
	}
	m_voxelData = voxelData;
	if( m_voxelData )
	{
		m_voxelData->m_onDataChanged.RegisterListener<Tr2VoxelGeometry, &Tr2VoxelGeometry::DataModified>( this );
	}
	Rebuild();
	return {};
}

bool Tr2VoxelGeometry::Initialize()
{
	Rebuild();
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
	Rebuild();
}

uint32_t Tr2VoxelGeometry::GetShapeChannelIndex() const
{
	return m_shapeChannelIndex;
}

void Tr2VoxelGeometry::SetShapeChannelIndex( uint32_t channelIndex )
{
	if( m_shapeChannelIndex == channelIndex )
	{
		return;
	}
	m_shapeChannelIndex = channelIndex;
	Rebuild();
}

float Tr2VoxelGeometry::GetCellSize() const
{
	return m_cellSize;
}

void Tr2VoxelGeometry::SetCellSize( float cellSize )
{
	if( m_cellSize == cellSize )
	{
		return;
	}
	m_cellSize = cellSize;
	Rebuild();
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
	Rebuild();
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
	Rebuild();
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
	Rebuild();
}


void Tr2VoxelGeometry::DataModified( Tr2VoxelData::ModifiedChannels modifiedChannels )
{
	if( modifiedChannels.IsChannelModified( m_shapeChannelIndex ) )
	{
		Rebuild();
	}
}

std::unique_ptr<Tr2ProceduralGeometryBuilder> Tr2VoxelGeometry::CreateBuilder() const
{
	if( !m_voxelData )
	{
		return std::make_unique<EmptyBuilder>();
	}
	// Shape channel is optional; builders compute the SDF from occupancy when it is missing
	auto shapeSnapshot = m_voxelData->GetPayloadSnapshot( m_shapeChannelIndex );
	if( !shapeSnapshot.m_data )
	{
		return std::make_unique<EmptyBuilder>();
	}
	if( !Tr2SignedDistanceField::IsValidSdfChannelType( shapeSnapshot.m_type ) )
	{
		return std::make_unique<EmptyBuilder>();
	}

	switch( m_method )
	{
	case Method::Simple:
		return std::make_unique<SimpleBuilder>( std::move( shapeSnapshot ), m_cellSize );
	case Method::SurfaceNets:
		return std::make_unique<SurfaceNetsBuilder>( std::move( shapeSnapshot ), m_cellSize, m_subdivision, m_relaxIterations );
	//case Method::DualContouring:
	//	return std::make_unique<DualContouringBuilder>( std::move( shapeSnapshot ), m_cellSize, m_subdivision, m_noiseStrength );
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