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

class SimpleBuilder : public Tr2ProceduralGeometryBuilder
{
public:
	SimpleBuilder( Tr2VoxelData::PayloadSnapshot occupancy, float cellSize ) :
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

		auto dimensions = m_occupancy.m_voxelData->GetDimensions();
		Vector3 meshSize = Vector3( float( dimensions.width ), float( dimensions.height ), float( dimensions.depth ) ) * m_cellSize;
		auto meshBounds = CcpMath::AxisAlignedBox(
			Vector3( -meshSize.x, -meshSize.y, -meshSize.z ) * 0.5f,
			Vector3( meshSize.x, meshSize.y, meshSize.z ) * 0.5f );

		auto [vbView, ibView] = CreateBuffers( allocator, buffers, cancel );
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

private:
	struct Vertex
	{
		Vector3 position;
		Vector3 normal;
	};

	std::pair<cmf::BufferView, cmf::BufferView> CreateBuffers( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel )
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
	return std::make_unique<SimpleBuilder>( std::move( occupancySnapshot ), m_cellSize );
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