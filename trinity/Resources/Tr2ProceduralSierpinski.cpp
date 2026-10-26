// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2ProceduralSierpinski.h"
#include "Include/TriMath.h"

namespace
{

struct Vector3Hash
{
	size_t operator()( const Vector3& v ) const
	{
		return CcpHashFNV1( &v, sizeof( Vector3 ) );
	}
};

uint32_t AddVertex( const Vector3& vertex, std::unordered_map<Vector3, uint32_t, Vector3Hash>& indicesMap, std::vector<Vector3>& vertices )
{
	auto [iter, inserted] = indicesMap.try_emplace( vertex, uint32_t( vertices.size() ) );
	if( inserted )
	{
		vertices.push_back( vertex );
	}
	return iter->second;
}

void Sierpinski( const Vector3& up, const Vector3& left, const Vector3& right, const Vector3& back, int depth,
	std::unordered_map<Vector3, uint32_t, Vector3Hash>& indicesMap, std::vector<uint32_t>& indices, std::vector<Vector3>& vertices, const std::atomic<bool>& cancel )
{
	if( cancel )
	{
		return;
	}

	if( depth == 0 )
	{
		uint32_t upIndex = AddVertex( up, indicesMap, vertices );
		uint32_t leftIndex = AddVertex( left, indicesMap, vertices );
		uint32_t rightIndex = AddVertex( right, indicesMap, vertices );
		uint32_t backIndex = AddVertex( back, indicesMap, vertices );

		indices.insert( indices.end(), { upIndex, leftIndex, rightIndex } );
		indices.insert( indices.end(), { upIndex, rightIndex, backIndex } );
		indices.insert( indices.end(), { upIndex, backIndex, leftIndex } );
		indices.insert( indices.end(), { leftIndex, backIndex, rightIndex } );

		return;
	}

	Vector3 upLeft = ( up + left ) * .5f;
	Vector3 upRight = ( up + right ) * .5f;
	Vector3 upBack = ( up + back ) * .5f;
	Vector3 leftRight = ( left + right ) * .5f;
	Vector3 leftBack = ( left + back ) * .5f;
	Vector3 rightBack = ( right + back ) * .5f;

	Sierpinski( up, upLeft, upRight, upBack, depth - 1, indicesMap, indices, vertices, cancel );
	Sierpinski( upLeft, left, leftRight, leftBack, depth - 1, indicesMap, indices, vertices, cancel );
	Sierpinski( upRight, leftRight, right, rightBack, depth - 1, indicesMap, indices, vertices, cancel );
	Sierpinski( upBack, leftBack, rightBack, back, depth - 1, indicesMap, indices, vertices, cancel );
}

}

bool Tr2ProceduralSierpinskiBuilder::BuildCmf( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel, cmf::Data& data )
{
	std::unordered_map<Vector3, uint32_t, Vector3Hash> indicesMap;
	std::vector<uint32_t> indices;
	std::vector<Vector3> vertices;

	float r = sqrtf( 6.f ) / 4.f;
	float yb = -r / 3.f;
	float cb = 1.f / sqrtf( 3.f );
	float b = cb * .5f;

	auto up = Vector3( 0.f, r, 0.f ) * m_size;
	auto left = Vector3( -.5f, yb, -b ) * m_size;
	auto right = Vector3( .5f, yb, -b ) * m_size;
	auto back = Vector3( 0.f, yb, cb ) * m_size;

	Sierpinski( up, left, right, back, ClampInt( m_depth, 0, 5 ), indicesMap, indices, vertices, cancel );

	if( cancel )
	{
		return false;
	}

	// Allocate vertex buffer
	const uint32_t vertexStride = sizeof( Vector3 );
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

	// Vertex declaration: position (float32 x 3)
	cmf::VertexElement posElement = {};
	posElement.usage = cmf::Usage::Position;
	posElement.usageIndex = 0;
	posElement.type = cmf::ElementType::Float32;
	posElement.elementCount = 3;
	posElement.offset = 0;

	// LodMeshArea: one area covering all indices
	cmf::LodMeshArea lodArea = {};
	lodArea.firstElement = 0;
	lodArea.elementCount = uint32_t( indices.size() ) / 3;

	// MeshLod
	cmf::MeshLod lod = {};
	lod.vb = vbView;
	lod.ib = ibView;
	cmf::Modify( lod.areas, allocator ).emplace_back( lodArea );

	// MeshArea
	cmf::MeshArea area = {};
	area.name = allocator.AllocateString( "SierpinskiMaterial" );
	area.bounds = CcpMath::AxisAlignedBox(
		Vector3( -.5f, yb, -b ) * m_size,
		Vector3( .5f, r, cb ) * m_size );

	// Mesh
	cmf::Mesh mesh = {};
	mesh.name = allocator.AllocateString( "Sierpinski" );
	cmf::Modify( mesh.decl, allocator ).emplace_back( posElement );
	cmf::Modify( mesh.lods, allocator ).emplace_back( lod );
	cmf::Modify( mesh.areas, allocator ).emplace_back( area );
	mesh.topology = cmf::MeshTopology::TriangleList;
	mesh.bounds = CcpMath::AxisAlignedBox(
		Vector3( -.5f, yb, -b ) * m_size,
		Vector3( .5f, r, cb ) * m_size );

	data = {};
	cmf::Modify( data.meshes, allocator ).emplace_back( mesh );

	return true;
}

std::unique_ptr<Tr2ProceduralGeometryBuilder> Tr2ProceduralSierpinski::CreateBuilder() const
{
	auto builder = std::make_unique<Tr2ProceduralSierpinskiBuilder>();
	builder->m_name = "Tr2ProceduralSierpinski";
	builder->m_size = m_size;
	builder->m_depth = m_depth;
	return builder;
}

float Tr2ProceduralSierpinski::GetSize()
{
	return m_size;
}

void Tr2ProceduralSierpinski::SetSize( float size )
{
	m_size = size;
	Rebuild();
}

int32_t Tr2ProceduralSierpinski::GetDepth()
{
	return m_depth;
}

void Tr2ProceduralSierpinski::SetDepth( int32_t depth )
{
	m_depth = depth;
	Rebuild();
}
