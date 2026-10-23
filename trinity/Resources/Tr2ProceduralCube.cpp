// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2ProceduralCube.h"

// based on cmf unit test found in github.com\mesh\src\cmf\test\writer.cpp
bool Tr2ProceduralCubeBuilder::BuildCmf( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel, cmf::Data& data )
{
	const std::array<Vector3, 8> cubeVertices = { {
		{ -m_extent, -m_extent, -m_extent },
		{ m_extent, -m_extent, -m_extent },
		{ m_extent, m_extent, -m_extent },
		{ -m_extent, m_extent, -m_extent },
		{ -m_extent, -m_extent, m_extent },
		{ m_extent, -m_extent, m_extent },
		{ m_extent, m_extent, m_extent },
		{ -m_extent, m_extent, m_extent },
	} };

	static const std::array<uint32_t, 36> s_cubeIndices = { {
		// Front face
		0,
		1,
		2,
		0,
		2,
		3,
		// Back face
		4,
		6,
		5,
		4,
		7,
		6,
		// Left face
		0,
		3,
		7,
		0,
		7,
		4,
		// Right face
		1,
		5,
		6,
		1,
		6,
		2,
		// Top face
		3,
		2,
		6,
		3,
		6,
		7,
		// Bottom face
		0,
		4,
		5,
		0,
		5,
		1,
	} };

	// Allocate vertex buffer
	const uint32_t vertexStride = sizeof( Vector3 );
	cmf::BufferView vbView = buffers.AllocateBuffer(
		cubeVertices.data(),
		uint32_t( cubeVertices.size() * vertexStride ),
		vertexStride );

	// Allocate index buffer
	const uint32_t indexStride = sizeof( uint32_t );
	cmf::BufferView ibView = buffers.AllocateBuffer(
		s_cubeIndices.data(),
		uint32_t( s_cubeIndices.size() * indexStride ),
		indexStride );

	// Vertex declaration: position (float32 x 3)
	cmf::VertexElement posElement = {};
	posElement.usage = cmf::Usage::Position;
	posElement.usageIndex = 0;
	posElement.type = cmf::ElementType::Float32;
	posElement.elementCount = 3;
	posElement.offset = 0;

	// LodMeshArea: one area covering all 36 indices
	cmf::LodMeshArea lodArea = {};
	lodArea.firstElement = 0;
	lodArea.elementCount = uint32_t( s_cubeIndices.size() ) / 3;

	// MeshLod
	cmf::MeshLod lod = {};
	lod.vb = vbView;
	lod.ib = ibView;
	cmf::Modify( lod.areas, allocator ).emplace_back( lodArea );

	// MeshArea
	cmf::MeshArea area = {};
	area.name = allocator.AllocateString( "CubeMaterial" );
	area.bounds = CcpMath::AxisAlignedBox(
		Vector3( -m_extent, -m_extent, -m_extent ),
		Vector3( m_extent, m_extent, m_extent ) );

	// Mesh
	cmf::Mesh mesh = {};
	mesh.name = allocator.AllocateString( "UnitCube" );
	cmf::Modify( mesh.decl, allocator ).emplace_back( posElement );
	cmf::Modify( mesh.lods, allocator ).emplace_back( lod );
	cmf::Modify( mesh.areas, allocator ).emplace_back( area );
	mesh.topology = cmf::MeshTopology::TriangleList;
	mesh.bounds = CcpMath::AxisAlignedBox(
		Vector3( -m_extent, -m_extent, -m_extent ),
		Vector3( m_extent, m_extent, m_extent ) );

	data = {};
	cmf::Modify( data.meshes, allocator ).emplace_back( mesh );

	return true;
}

std::unique_ptr<Tr2ProceduralGeometryBuilder> Tr2ProceduralCube::CreateBuilder() const
{
	auto builder = std::make_unique<Tr2ProceduralCubeBuilder>();
	builder->m_extent = m_extent;
	return builder;
}

float Tr2ProceduralCube::GetExtent()
{
	return m_extent;
}

void Tr2ProceduralCube::SetExtent( float extent )
{
	m_extent = extent;
	Rebuild();
}
