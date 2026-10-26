// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2ProceduralSierpinski.h"
#include "Include/TriMath.h"

namespace
{

struct Vertex
{
	Vector3 position;
	std::array<int16_t, 4> packedTangent;

	bool operator==( const Vertex& other ) const
	{
		return position == other.position && packedTangent == other.packedTangent;
	}
};

struct VertexHash
{
	size_t operator()( const Vertex& v ) const
	{
		return CcpHashFNV1( &v, sizeof( Vertex ) );
	}
};

uint32_t AddVertex( const Vector3& vertex,
					const std::array<int16_t, 4>& packedTangent,
					std::unordered_map<Vertex, uint32_t, VertexHash>& indicesMap,
					std::vector<Vertex>& vertices )
{
	auto [iter, inserted] = indicesMap.try_emplace( { vertex, packedTangent }, uint32_t( vertices.size() ) );
	if( inserted )
	{
		vertices.push_back( { vertex, packedTangent } );
	}
	return iter->second;
}

void AddTriangle( const Vector3& vert0,
				  const Vector3& vert1,
				  const Vector3& vert2,
				  const std::array<int16_t, 4>& packedTangent,
				  std::unordered_map<Vertex, uint32_t, VertexHash>& indicesMap,
				  std::vector<uint32_t>& indices,
				  std::vector<Vertex>& vertices )
{
	uint32_t index0 = AddVertex( vert0, packedTangent, indicesMap, vertices );
	uint32_t index1 = AddVertex( vert1, packedTangent, indicesMap, vertices );
	uint32_t index2 = AddVertex( vert2, packedTangent, indicesMap, vertices );
	indices.insert( indices.end(), { index0, index1, index2 } );
}

void Sierpinski( const Vector3& up,
				 const Vector3& left,
				 const Vector3& right,
				 const Vector3& back,
				 int depth,
				 const std::array<std::array<int16_t, 4>, 4>& packedTangents,
				 const std::atomic<bool>& cancel,
				 std::unordered_map<Vertex, uint32_t, VertexHash>& indicesMap,
				 std::vector<uint32_t>& indices,
				 std::vector<Vertex>& vertices )
{
	if( cancel )
	{
		return;
	}

	if( depth == 0 )
	{
		AddTriangle( up, left, right, packedTangents[0], indicesMap, indices, vertices );
		AddTriangle( up, right, back, packedTangents[1], indicesMap, indices, vertices );
		AddTriangle( up, back, left, packedTangents[2], indicesMap, indices, vertices );
		AddTriangle( left, back, right, packedTangents[3], indicesMap, indices, vertices );
		return;
	}

	Vector3 upLeft = ( up + left ) * .5f;
	Vector3 upRight = ( up + right ) * .5f;
	Vector3 upBack = ( up + back ) * .5f;
	Vector3 leftRight = ( left + right ) * .5f;
	Vector3 leftBack = ( left + back ) * .5f;
	Vector3 rightBack = ( right + back ) * .5f;

	Sierpinski( up, upLeft, upRight, upBack, depth - 1, packedTangents, cancel, indicesMap, indices, vertices );
	Sierpinski( upLeft, left, leftRight, leftBack, depth - 1, packedTangents, cancel, indicesMap, indices, vertices );
	Sierpinski( upRight, leftRight, right, rightBack, depth - 1, packedTangents, cancel, indicesMap, indices, vertices );
	Sierpinski( upBack, leftBack, rightBack, back, depth - 1, packedTangents, cancel, indicesMap, indices, vertices );
}

}

bool Tr2ProceduralSierpinskiBuilder::BuildCmf( cmf::MemoryAllocator& allocator, cmf::BufferManager& buffers, const std::atomic<bool>& cancel, cmf::Data& data )
{
	std::unordered_map<Vertex, uint32_t, VertexHash> indicesMap;
	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;

	float r = sqrtf( 6.f ) / 4.f;
	float yb = -r / 3.f;
	float cb = 1.f / sqrtf( 3.f );
	float b = cb * .5f;

	auto up = Vector3( 0.f, r, 0.f ) * m_size;
	auto left = Vector3( -.5f, yb, -b ) * m_size;
	auto right = Vector3( .5f, yb, -b ) * m_size;
	auto back = Vector3( 0.f, yb, cb ) * m_size;

	std::array<Vector3, 4> edges = {
		Normalize( left - up ),
		Normalize( right - up ),
		Normalize( left - back ),
		Normalize( right - back )
	};
	std::array<Vector3, 4> normals = {
		Normalize( Cross( edges[0], edges[1] ) ),
		Normalize( Cross( edges[3], edges[1] ) ),
		Normalize( Cross( edges[0], edges[2] ) ),
		Normalize( Cross( edges[3], edges[2] ) )
	};
	std::array<Vector4, 4> packedTangents = {
		cmf::PackTangents( cmf::TangentCompression::PackedTangent, normals[0], edges[0], Cross( normals[0], edges[0] ) ),
		cmf::PackTangents( cmf::TangentCompression::PackedTangent, normals[1], edges[3], Cross( normals[1], edges[3] ) ),
		cmf::PackTangents( cmf::TangentCompression::PackedTangent, normals[2], edges[0], Cross( normals[2], edges[0] ) ),
		cmf::PackTangents( cmf::TangentCompression::PackedTangent, normals[3], edges[3], Cross( normals[3], edges[3] ) ),
	};

	auto convert = cmf::GetScalarConversionFunction<float>( cmf::ElementType::Int16Norm ).first;
	std::array<std::array<int16_t, 4>, 4> convertedTangents;
	for( int32_t i = 0; i < 4; i++ )
	{
		for( int32_t j = 0; j < 4; j++ )
		{
			convert.from( &convertedTangents[i][j], packedTangents[i][j] );
		}
	}

	Sierpinski( up, left, right, back, ClampInt( m_depth, 0, 5 ), convertedTangents, cancel, indicesMap, indices, vertices );

	if( cancel )
	{
		return false;
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

	// Vertex declaration: position (float32 x 3)
	cmf::VertexElement posElement = {};
	posElement.usage = cmf::Usage::Position;
	posElement.usageIndex = 0;
	posElement.type = cmf::ElementType::Float32;
	posElement.elementCount = 3;
	posElement.offset = 0;
	// Vertex declaration: packed tangent (int16norm x 4)
	cmf::VertexElement packedTangentElement = {};
	packedTangentElement.usage = cmf::Usage::PackedTangent;
	packedTangentElement.usageIndex = 0;
	packedTangentElement.type = cmf::ElementType::Int16Norm;
	packedTangentElement.elementCount = 4;
	packedTangentElement.offset = offsetof( Vertex, packedTangent );

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
	cmf::Modify( mesh.decl, allocator ).emplace_back( packedTangentElement );
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
