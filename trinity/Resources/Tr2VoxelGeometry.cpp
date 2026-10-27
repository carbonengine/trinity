// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2VoxelGeometry.h"


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

		auto [vbView, ibView] = CreateBuffers( allocator, buffers );

		cmf::LodMeshArea lodArea = {};
		lodArea.firstElement = 0;
		lodArea.elementCount = vbView.size / vbView.stride / 3;

		cmf::MeshLod lod = {};
		lod.vb = vbView;
		lod.ib = ibView;
		cmf::Modify( lod.areas, allocator ).emplace_back( lodArea );

		cmf::MeshArea area = {};
		area.name = allocator.AllocateString( "CubeMaterial" );
		area.bounds = meshBounds;

		cmf::Mesh mesh = {};
		mesh.name = allocator.AllocateString( "UnitCube" );
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

	std::pair<cmf::BufferView, cmf::BufferView> CreateBuffers( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers )
	{
		// TODO: implement this method to create vertex and index buffers based on the occupancy data and cell size.
		// The method should create triangles for each face of the occupied voxels and return the buffer views for the vertex and index buffers.
		// It should not create faces for voxels that are adjacent to other occupied voxels, as those faces would be internal and not visible.
		// The vertex buffer should contain the position and normal for each vertex, and the index buffer should contain the indices for the triangles for "triangle list" topology.
		// All edges are "hard" edges, so no smoothing is needed. It should reuse vertices for adjacent faces to minimize the number of vertices.

		return {};
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

IRootPtr Tr2VoxelGeometry::GetVoxelData() const
{
	return m_voxelData.p;
}

void Tr2VoxelGeometry::SetVoxelData( Tr2VoxelData* voxelData )
{
	m_voxelData = voxelData;
	Rebuild();
}

float Tr2VoxelGeometry::GetCellSize() const
{
	return m_cellSize;
}

void Tr2VoxelGeometry::SetCellSize( float cellSize )
{
	m_cellSize = cellSize;
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