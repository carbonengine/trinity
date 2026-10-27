// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2VoxelGeometry.h"
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
	VoxelBuilder( Tr2VoxelData::PayloadSnapshot occupancy, float cellSize ) :
		m_occupancy( std::move( occupancy ) ),
		m_cellSize( cellSize ),
		m_dimensions( m_occupancy.m_voxelData->GetDimensions() )
	{
	}

	bool BuildCmf( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel, cmf::Data& data ) override
	{
		if( cancel )
		{
			return false;
		}

		CcpMath::AxisAlignedBox meshBounds;
		auto [vbView, ibView] = CreateBuffers( allocator, buffers, cancel, meshBounds );
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
		cmf::Modify( mesh.decl, allocator ).emplace_back( cmf::Usage::Position, uint8_t( 0 ), cmf::ElementType::Float32, uint8_t( 3 ), 0u );
		cmf::Modify( mesh.decl, allocator ).emplace_back( cmf::Usage::Normal, uint8_t( 0 ), cmf::ElementType::Float32, uint8_t( 3 ), uint32_t( sizeof( float ) * 3 ) );
		cmf::Modify( mesh.lods, allocator ).emplace_back( lod );
		cmf::Modify( mesh.areas, allocator ).emplace_back( area );
		mesh.topology = cmf::MeshTopology::TriangleList;
		mesh.bounds = meshBounds;

		data = {};
		cmf::Modify( data.meshes, allocator ).emplace_back( mesh );

		return true;
	}

protected:
	struct Vertex
	{
		Vector3 position;
		Vector3 normal;
	};

	// Returns vertex and index buffer views, and the bounds of the generated vertices in bounds
	virtual std::pair<cmf::BufferView, cmf::BufferView> CreateBuffers( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel, CcpMath::AxisAlignedBox& bounds ) = 0;

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

	bool GetCellOccupancy( const Tr2VoxelData::Coord& coord ) const
	{
		size_t index = coord.x + m_dimensions.width * ( coord.y + m_dimensions.height * coord.z );
		auto byte = m_occupancy.m_data[index / 8];
		return ( byte & ( 1 << ( index % 8 ) ) ) != 0;
	}

	const Tr2VoxelData::PayloadSnapshot m_occupancy;
	const float m_cellSize;
	const Tr2VoxelData::Dimensions m_dimensions;
};

class SimpleBuilder : public VoxelBuilder
{
public:
	SimpleBuilder( Tr2VoxelData::PayloadSnapshot occupancy, float cellSize ) :
		VoxelBuilder( std::move( occupancy ), cellSize )
	{
	}

protected:
	std::pair<cmf::BufferView, cmf::BufferView> CreateBuffers( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel, CcpMath::AxisAlignedBox& bounds ) override
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

		auto isOccupied = [&]( const Tr2VoxelData::Coord& coord ) {
			if( coord.x < 0 || coord.y < 0 || coord.z < 0 || coord.x >= width || coord.y >= height || coord.z >= depth )
			{
				return false;
			}
			return GetCellOccupancy( coord );
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
					if( !GetCellOccupancy( { x, y, z } ) )
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

		return { vbView, ibView };
	}
};


class SmoothingBuilder : public VoxelBuilder
{
public:
	SmoothingBuilder( Tr2VoxelData::PayloadSnapshot occupancy, float cellSize, uint32_t subdivision, uint32_t relaxIterations ) :
		VoxelBuilder( std::move( occupancy ), cellSize ),
		m_subdivision( subdivision ),
		m_relaxIterations( relaxIterations )
	{
	}

protected:
	// Surface Nets extraction, a port of extract_surface from spacecomponents compositeasteroid_meshing.py (with the SDF
	// encoded from occupancy as compositeasteroid._encode_sdf does). Cell centres sit at integer lattice coordinates, a dual
	// vertex lives in every 2x2x2 block of cells with mixed occupancy, and every occupancy sign-change edge emits the quad
	// joining the four blocks sharing it. Cells outside the grid read as empty, so the surface closes at the grid boundary.
	// m_subdivision > 1 runs the construction on a lattice subdivided k-fold whose field is the trilinear interpolation of
	// the SDF. Quads are emitted as two triangles each.
	std::pair<cmf::BufferView, cmf::BufferView> CreateBuffers( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel, CcpMath::AxisAlignedBox& bounds ) override
	{
		if( m_dimensions.width == 0 || m_dimensions.height == 0 || m_dimensions.depth == 0 )
		{
			return {};
		}
		const int32_t scale = int32_t( std::max( m_subdivision, 1u ) );

		auto field = CreateField( cancel );
		if( cancel )
		{
			return {};
		}
		auto window = scale == 1 ? CreateGridWindow( field ) : CreateSubdividedWindow( field, scale, cancel );
		if( cancel )
		{
			return {};
		}

		std::vector<Vec3d> positions;
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

		// Quads are wound counter-clockwise seen from the empty side
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

		return { vbView, ibView };
	}

private:
	using Vec3d = std::array<double, 3>;
	using Int3 = std::array<int32_t, 3>;

	static constexpr double s_sdfClamp = 3.0;             // encode clamp in cells
	static constexpr double s_sdfDecode = s_sdfClamp / 127.5;
	static constexpr double s_deepEmpty = -s_sdfClamp;    // field value sampled outside the grid
	static constexpr int32_t s_sdfSearchRadius = 4;       // surface band radius for the chamfer distance transform
	static constexpr double s_relaxAlpha = 0.5;           // Laplacian relaxation strength per iteration
	static constexpr double s_blurAlpha = 0.15;           // sign-preserving low-pass kernel [a/2, 1-a, a/2] of the extraction field
	static constexpr double s_signFloor = 0.12;           // min |field| kept at an in-grid cell centre after the low-pass
	static constexpr double s_roamMargin = 0.15;          // dual vertex clamp cube inset, keeps neighbouring vertices apart
	static constexpr double s_tMin = 0.10;                // bound of the edge crossing interpolant off 0/1

	// (u, v) per axis, right-handed: u x v = +axis
	static constexpr std::array<std::array<int, 2>, 3> s_axisUV = { { { 1, 2 }, { 2, 0 }, { 0, 1 } } };

	// Occupancy and field over the lattice plus a one cell ring of empty cells (lattice coordinates -1..lattice)
	struct Window
	{
		explicit Window( const Int3& lattice_ ) :
			lattice( lattice_ ),
			strides( { 1, size_t( lattice_[0] + 2 ), size_t( lattice_[0] + 2 ) * size_t( lattice_[1] + 2 ) } ),
			occupancy( strides[2] * size_t( lattice_[2] + 2 ), 0 ),
			field( occupancy.size(), std::numeric_limits<double>::quiet_NaN() )
		{
		}

		size_t Index( int32_t x, int32_t y, int32_t z ) const
		{
			return size_t( x + 1 ) + strides[1] * size_t( y + 1 ) + strides[2] * size_t( z + 1 );
		}

		const Int3 lattice;
		const std::array<size_t, 3> strides;
		std::vector<uint8_t> occupancy;
		std::vector<double> field; // NaN where the cell has no field value
	};

	Int3 GetGridSize() const
	{
		return { Tr2VoxelData::CoordType( m_dimensions.width ), Tr2VoxelData::CoordType( m_dimensions.height ), Tr2VoxelData::CoordType( m_dimensions.depth ) };
	}

	std::vector<uint8_t> GetOccupancy() const
	{
		const Int3 dims = GetGridSize();
		std::vector<uint8_t> occupancy( size_t( dims[0] ) * dims[1] * dims[2] );
		size_t i = 0;
		for( Tr2VoxelData::CoordType z = 0; z < dims[2]; ++z )
		{
			for( Tr2VoxelData::CoordType y = 0; y < dims[1]; ++y )
			{
				for( Tr2VoxelData::CoordType x = 0; x < dims[0]; ++x )
				{
					occupancy[i++] = GetCellOccupancy( { x, y, z } ) ? 1 : 0;
				}
			}
		}
		return occupancy;
	}

	static uint8_t EncodeDistance( double distance )
	{
		return uint8_t( std::max( 0.0, std::min( 255.0, 127.5 + distance * 127.5 / s_sdfClamp + 0.5 ) ) );
	}

	// Encoded signed distance (byte = 127.5 + dist * 127.5 / clamp, positive inside solid) for every cell via a two-pass
	// chamfer distance transform restricted to the surface band. Deep cells clamp to the sentinels.
	std::vector<uint8_t> EncodeSdf( const std::vector<uint8_t>& occupancy, const std::atomic<bool>& cancel ) const
	{
		const Int3 dims = GetGridSize();
		const std::array<size_t, 3> strides = { 1, size_t( dims[0] ), size_t( dims[0] ) * dims[1] };
		const size_t count = occupancy.size();

		// Cells within Chebyshev s_sdfSearchRadius of a face occupancy transition
		std::vector<uint8_t> band( count, 0 );
		for( int32_t z = 0; z < dims[2]; ++z )
		{
			for( int32_t y = 0; y < dims[1]; ++y )
			{
				for( int32_t x = 0; x < dims[0]; ++x )
				{
					const Int3 coord = { x, y, z };
					const size_t i = x + strides[1] * y + strides[2] * z;
					for( int axis = 0; axis < 3; ++axis )
					{
						if( coord[axis] + 1 < dims[axis] && occupancy[i + strides[axis]] != occupancy[i] )
						{
							band[i] = 1;
							band[i + strides[axis]] = 1;
						}
					}
				}
			}
		}
		for( int axis = 0; axis < 3; ++axis )
		{
			const int u = s_axisUV[axis][0];
			const int v = s_axisUV[axis][1];
			const auto mask = band;
			for( int32_t j = 0; j < dims[v]; ++j )
			{
				for( int32_t i = 0; i < dims[u]; ++i )
				{
					const size_t base = strides[u] * i + strides[v] * j;
					int32_t last = -( 1 << 30 );
					for( int32_t k = 0; k < dims[axis]; ++k )
					{
						if( mask[base + strides[axis] * k] )
						{
							last = k;
						}
						if( k - last <= s_sdfSearchRadius )
						{
							band[base + strides[axis] * k] = 1;
						}
					}
					last = 1 << 30;
					for( int32_t k = dims[axis] - 1; k >= 0; --k )
					{
						if( mask[base + strides[axis] * k] )
						{
							last = k;
						}
						if( last - k <= s_sdfSearchRadius )
						{
							band[base + strides[axis] * k] = 1;
						}
					}
				}
			}
		}
		if( cancel )
		{
			return {};
		}

		std::vector<uint32_t> cells;
		const double infinity = std::numeric_limits<double>::infinity();
		std::vector<double> distanceToEmpty( count, infinity );
		std::vector<double> distanceToSolid( count, infinity );
		for( size_t i = 0; i < count; ++i )
		{
			if( band[i] )
			{
				cells.push_back( uint32_t( i ) );
				( occupancy[i] ? distanceToSolid : distanceToEmpty )[i] = 0.0;
			}
		}

		// Local Euclidean step lengths for the 26 neighbours, split by raster direction
		struct Neighbor
		{
			Int3 offset;
			ptrdiff_t flatOffset;
			double weight;
		};
		std::vector<Neighbor> forward;
		std::vector<Neighbor> backward;
		for( int32_t dz = -1; dz <= 1; ++dz )
		{
			for( int32_t dy = -1; dy <= 1; ++dy )
			{
				for( int32_t dx = -1; dx <= 1; ++dx )
				{
					if( dx || dy || dz )
					{
						const ptrdiff_t flatOffset = dx + ptrdiff_t( strides[1] ) * dy + ptrdiff_t( strides[2] ) * dz;
						( flatOffset < 0 ? forward : backward ).push_back( { { dx, dy, dz }, flatOffset, std::sqrt( double( dx * dx + dy * dy + dz * dz ) ) } );
					}
				}
			}
		}

		auto propagate = [&]( uint32_t i, const std::vector<Neighbor>& neighbors ) {
			const Int3 coord = { int32_t( i % strides[1] ), int32_t( ( i / strides[1] ) % dims[1] ), int32_t( i / strides[2] ) };
			double bestEmpty = distanceToEmpty[i];
			double bestSolid = distanceToSolid[i];
			for( const auto& neighbor : neighbors )
			{
				bool inside = true;
				for( int axis = 0; axis < 3; ++axis )
				{
					const int32_t c = coord[axis] + neighbor.offset[axis];
					inside = inside && c >= 0 && c < dims[axis];
				}
				if( !inside )
				{
					continue;
				}
				const size_t j = size_t( ptrdiff_t( i ) + neighbor.flatOffset );
				bestEmpty = std::min( bestEmpty, distanceToEmpty[j] + neighbor.weight );
				bestSolid = std::min( bestSolid, distanceToSolid[j] + neighbor.weight );
			}
			distanceToEmpty[i] = bestEmpty;
			distanceToSolid[i] = bestSolid;
		};
		for( auto i : cells )
		{
			propagate( i, forward );
		}
		if( cancel )
		{
			return {};
		}
		for( auto iter = cells.rbegin(); iter != cells.rend(); ++iter )
		{
			propagate( *iter, backward );
		}

		const uint8_t encodedSolid = EncodeDistance( s_sdfClamp );
		const uint8_t encodedEmpty = EncodeDistance( -s_sdfClamp );
		std::vector<uint8_t> sdf( count );
		for( size_t i = 0; i < count; ++i )
		{
			if( !band[i] )
			{
				sdf[i] = occupancy[i] ? encodedSolid : encodedEmpty;
			}
			else if( occupancy[i] )
			{
				const double d = distanceToEmpty[i] - 0.5;
				sdf[i] = d >= s_sdfClamp ? encodedSolid : EncodeDistance( d );
			}
			else
			{
				const double d = distanceToSolid[i] - 0.5;
				sdf[i] = d >= s_sdfClamp ? encodedEmpty : EncodeDistance( -d );
			}
		}
		return sdf;
	}

	// Decoded field at the grid cell centres, low-passed and clamped back to the occupancy sign so no solid cell can
	// vanish from the mesh and no empty cell can seal over.
	std::vector<double> CreateField( const std::atomic<bool>& cancel ) const
	{
		const Int3 dims = GetGridSize();
		const auto occupancy = GetOccupancy();
		const auto sdf = EncodeSdf( occupancy, cancel );
		if( cancel )
		{
			return {};
		}

		// Decode into a copy padded by one ring of deep empty so every in-grid sample has its full blur stencil
		const Int3 padded = { dims[0] + 2, dims[1] + 2, dims[2] + 2 };
		const std::array<size_t, 3> strides = { 1, size_t( padded[0] ), size_t( padded[0] ) * padded[1] };
		auto paddedIndex = [&]( int32_t x, int32_t y, int32_t z ) {
			return size_t( x + 1 ) + strides[1] * size_t( y + 1 ) + strides[2] * size_t( z + 1 );
		};
		std::vector<double> values( strides[2] * padded[2], s_deepEmpty );
		size_t i = 0;
		for( int32_t z = 0; z < dims[2]; ++z )
		{
			for( int32_t y = 0; y < dims[1]; ++y )
			{
				for( int32_t x = 0; x < dims[0]; ++x )
				{
					values[paddedIndex( x, y, z )] = ( double( sdf[i++] ) - 127.5 ) * s_sdfDecode;
				}
			}
		}

		// Separable Jacobi low-pass per axis. Only interior samples of each line move. The correction form is an exact
		// identity wherever the field is locally linear, so only cell-frequency curvature is damped.
		const double half = 0.5 * s_blurAlpha;
		for( int axis = 0; axis < 3; ++axis )
		{
			const int u = s_axisUV[axis][0];
			const int v = s_axisUV[axis][1];
			const size_t step = strides[axis];
			const auto source = values;
			for( int32_t b = 0; b < padded[v]; ++b )
			{
				for( int32_t a = 0; a < padded[u]; ++a )
				{
					const size_t base = strides[u] * a + strides[v] * b;
					for( int32_t k = 1; k + 1 < padded[axis]; ++k )
					{
						const size_t index = base + step * k;
						const double value = source[index];
						values[index] = value + half * ( ( source[index - step] - value ) + ( source[index + step] - value ) );
					}
				}
			}
		}

		std::vector<double> field( occupancy.size() );
		i = 0;
		for( int32_t z = 0; z < dims[2]; ++z )
		{
			for( int32_t y = 0; y < dims[1]; ++y )
			{
				for( int32_t x = 0; x < dims[0]; ++x, ++i )
				{
					double value = values[paddedIndex( x, y, z )];
					if( occupancy[i] )
					{
						if( value < s_signFloor )
						{
							value = s_signFloor;
						}
					}
					else if( value > -s_signFloor )
					{
						value = -s_signFloor;
					}
					field[i] = value;
				}
			}
		}
		return field;
	}

	// Lattice equal to the grid: occupancy straight from the grid, field from the filtered SDF
	Window CreateGridWindow( const std::vector<double>& field ) const
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
					window.occupancy[index] = field[i] > 0.0 ? 1 : 0;
					window.field[index] = field[i];
				}
			}
		}
		return window;
	}

	// Lattice of dim * scale cells per axis, each the trilinear interpolation of the 8 grid samples of its cube (samples outside
	// the grid read deep empty). Only cubes straddling a sign change, plus one cube of dilation, get the real per-cell trilinear;
	// the rest take the cube's uniform sign and have no field value.
	Window CreateSubdividedWindow( const std::vector<double>& field, int32_t scale, const std::atomic<bool>& cancel ) const
	{
		const Int3 dims = GetGridSize();
		Window window( { dims[0] * scale, dims[1] * scale, dims[2] * scale } );

		auto sample = [&]( int32_t x, int32_t y, int32_t z ) {
			if( x < 0 || y < 0 || z < 0 || x >= dims[0] || y >= dims[1] || z >= dims[2] )
			{
				return s_deepEmpty;
			}
			return field[x + size_t( dims[0] ) * ( y + size_t( dims[1] ) * z )];
		};

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
						const bool positive = sample( x + ( corner & 1 ), y + ( ( corner >> 1 ) & 1 ), z + ( corner >> 2 ) ) > 0.0;
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

		std::vector<double> t( scale );
		for( int32_t i = 0; i < scale; ++i )
		{
			t[i] = i / double( scale );
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
						const double s000 = sample( cx, cy, cz );
						const double s100 = sample( cx + 1, cy, cz );
						const double s010 = sample( cx, cy + 1, cz );
						const double s110 = sample( cx + 1, cy + 1, cz );
						const double s001 = sample( cx, cy, cz + 1 );
						const double s101 = sample( cx + 1, cy, cz + 1 );
						const double s011 = sample( cx, cy + 1, cz + 1 );
						const double s111 = sample( cx + 1, cy + 1, cz + 1 );
						for( int32_t iz = 0; iz < scale; ++iz )
						{
							const double tz = t[iz];
							const double e00 = s000 + ( s001 - s000 ) * tz;
							const double e10 = s100 + ( s101 - s100 ) * tz;
							const double e01 = s010 + ( s011 - s010 ) * tz;
							const double e11 = s110 + ( s111 - s110 ) * tz;
							for( int32_t iy = 0; iy < scale; ++iy )
							{
								const double ty = t[iy];
								const double f0 = e00 + ( e01 - e00 ) * ty;
								const double f1 = e10 + ( e11 - e10 ) * ty;
								for( int32_t ix = 0; ix < scale; ++ix )
								{
									const double value = f0 + ( f1 - f0 ) * t[ix];
									const size_t index = window.Index( cx * scale + ix, cy * scale + iy, cz * scale + iz );
									window.field[index] = value;
									window.occupancy[index] = value >= 0.0 ? 1 : 0;
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
	static Vec3d GetDualVertex( const Window& window, const Int3& block, int32_t scale )
	{
		const size_t base = window.Index( block[0], block[1], block[2] );
		Vec3d sum = { 0.0, 0.0, 0.0 };
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
					const double da = window.field[loSolid ? ia : ib];
					const double db = window.field[loSolid ? ib : ia];
					double t = 0.5;
					// NaN is a cell without a field value, so split the edge
					if( !std::isnan( da ) && !std::isnan( db ) && da != db )
					{
						t = std::min( std::max( da / ( da - db ), s_tMin ), 1.0 - s_tMin );
					}
					for( int j = 0; j < 3; ++j )
					{
						sum[j] += solidOffset[j] + t * ( emptyOffset[j] - solidOffset[j] );
					}
					++count;
				}
			}
		}
		Vec3d result;
		for( int j = 0; j < 3; ++j )
		{
			const double position = block[j] + sum[j] / count;
			result[j] = std::min( std::max( position, block[j] + s_roamMargin ), block[j] + 1 - s_roamMargin ) / scale;
		}
		return result;
	}

	// Emits a quad for every occupancy sign change, as 4 indices into shared dual vertices (one per block), wound
	// counter-clockwise seen from the empty side
	static void Sweep( const Window& window, int32_t scale, const std::atomic<bool>& cancel, std::vector<Vec3d>& positions, std::vector<uint32_t>& quads )
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
	void Relax( std::vector<Vec3d>& positions, const std::vector<uint32_t>& quads, const std::atomic<bool>& cancel ) const
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

		std::vector<Vec3d> moved( positions.size() );
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
				Vec3d sum = { 0.0, 0.0, 0.0 };
				for( auto neighbor : neighbors[i] )
				{
					for( int j = 0; j < 3; ++j )
					{
						sum[j] += positions[neighbor][j];
					}
				}
				const double count = double( neighbors[i].size() );
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
	static std::vector<Vec3d> ComputeNormals( const std::vector<Vec3d>& positions, const std::vector<uint32_t>& quads )
	{
		std::vector<Vec3d> normals( positions.size(), Vec3d{ 0.0, 0.0, 0.0 } );
		for( size_t q = 0; q < quads.size(); q += 4 )
		{
			const auto& p0 = positions[quads[q]];
			const auto& p1 = positions[quads[q + 1]];
			const auto& p2 = positions[quads[q + 2]];
			const auto& p3 = positions[quads[q + 3]];
			const Vec3d a = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };
			const Vec3d b = { p3[0] - p1[0], p3[1] - p1[1], p3[2] - p1[2] };
			const Vec3d n = { a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0] };
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
			const double length = std::sqrt( normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2] );
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


void Tr2VoxelGeometry::Create( Be::Optional<Tr2VoxelData*> voxelData, Be::Optional<uint32_t> occupancyChannelIndex, Be::Optional<float> cellSize )
{
	auto size = cellSize.IsAssigned() ? cellSize.GetValue() : 1.f;
	if( m_voxelData == voxelData && m_occupancyChannelIndex == occupancyChannelIndex && m_cellSize == size )
	{
		return;
	}
	m_occupancyChannelIndex = occupancyChannelIndex;
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

uint32_t Tr2VoxelGeometry::GetOccupancyChannelIndex() const
{
	return m_occupancyChannelIndex;
}

void Tr2VoxelGeometry::SetOccupancyChannelIndex( uint32_t channelIndex )
{
	if( m_occupancyChannelIndex == channelIndex )
	{
		return;
	}
	m_occupancyChannelIndex = channelIndex;
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
	Rebuild();
}

std::unique_ptr<Tr2ProceduralGeometryBuilder> Tr2VoxelGeometry::CreateBuilder() const
{
	if( !m_voxelData )
	{
		return std::make_unique<EmptyBuilder>();
	}
	auto occupancySnapshot = m_voxelData->GetPayloadSnapshot( 0 );
	if( !occupancySnapshot.m_data )
	{
		return std::make_unique<EmptyBuilder>();
	}
	if( m_method == Method::Simple )
	{
		return std::make_unique<SimpleBuilder>( std::move( occupancySnapshot ), m_cellSize );
	}
	return std::make_unique<SmoothingBuilder>( std::move( occupancySnapshot ), m_cellSize, m_subdivision, m_relaxIterations );
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