// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveDecalTriangleCache.h"
#include "Utilities/BoundingBox.h"
#include "Shader/Tr2EffectStateManager.h"
#include <thread>
#include <unordered_map>

namespace
{
// average number of triangles per cell the finest grid level aims for
constexpr float TRIANGLES_PER_CELL = 4.f;
constexpr uint32_t MAX_CELLS_PER_AXIS = 256;
constexpr float MAX_CELLS = float( 1u << 21 );

// Raw copy of one LOD's buffers, taken on the main thread and converted on the worker
struct RawLod
{
	std::vector<uint8_t> vertices;
	std::vector<uint8_t> indices;
	uint32_t vertexCount = 0;
	uint32_t indexStride = 4;
	uint32_t primitiveCount = 0;
};

// Reads one LOD back from the GPU. Returns false if the buffers can't be read.
bool CopyLod( const TriGeometryResMeshData& meshData, TriGeometryResLodData& lod, RawLod& out )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	const unsigned char* vertices = nullptr;
	const unsigned char* indices = nullptr;
	if( FAILED( lod.m_vertexAllocation.MapForReading( vertices, renderContext ) ) )
	{
		return false;
	}
	ON_BLOCK_EXIT( [&] { lod.m_vertexAllocation.UnmapForReading( renderContext ); } );
	if( FAILED( lod.m_indexAllocation.MapForReading( indices, renderContext ) ) )
	{
		return false;
	}
	ON_BLOCK_EXIT( [&] { lod.m_indexAllocation.UnmapForReading( renderContext ); } );

	out.vertexCount = meshData.m_bytesPerVertex ? std::min( lod.m_vertexCount, lod.m_vertexAllocation.GetSize() / meshData.m_bytesPerVertex ) : 0;
	out.indexStride = lod.m_indexAllocation.GetStride() == 2 ? 2 : 4;
	out.primitiveCount = std::min( lod.m_primitiveCount, lod.m_indexAllocation.GetSize() / ( out.indexStride * 3 ) );
	out.vertices.assign( vertices, vertices + size_t( out.vertexCount ) * meshData.m_bytesPerVertex );
	out.indices.assign( indices, indices + size_t( out.primitiveCount ) * 3 * out.indexStride );
	return true;
}

void ConvertLod( const RawLod& raw, uint32_t bytesPerVertex, bool positionsCompressed, DecalTriangleCache::Lod& out )
{
	out.positions.resize( raw.vertexCount );
	for( uint32_t v = 0; v < raw.vertexCount; ++v )
	{
		const uint8_t* vertex = raw.vertices.data() + size_t( v ) * bytesPerVertex;
		if( positionsCompressed )
		{
			out.positions[v] = *reinterpret_cast<const Vector3_16*>( vertex );
		}
		else
		{
			out.positions[v] = *reinterpret_cast<const Vector3*>( vertex );
		}
	}

	const size_t indexCount = size_t( raw.primitiveCount ) * 3;
	out.indices.resize( indexCount );
	for( size_t i = 0; i < indexCount; ++i )
	{
		uint32_t index = raw.indexStride == 2 ? reinterpret_cast<const uint16_t*>( raw.indices.data() )[i] : reinterpret_cast<const uint32_t*>( raw.indices.data() )[i];
		out.indices[i] = index < raw.vertexCount ? index : 0;
	}
}

uint32_t CellCoordinate( float value, float origin, float invCellSize, uint32_t dim )
{
	return uint32_t( std::clamp( ( value - origin ) * invCellSize, 0.f, float( dim - 1 ) ) );
}

void BuildGrid( DecalTriangleCache::Lod& lod )
{
	const size_t triangleCount = lod.indices.size() / 3;
	if( triangleCount == 0 || lod.positions.empty() )
	{
		lod.levels.assign( 1, {} );
		lod.cellStart.assign( 2, 0 );
		return;
	}

	Vector3 minBounds( FLT_MAX, FLT_MAX, FLT_MAX );
	Vector3 maxBounds( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	for( const auto& p : lod.positions )
	{
		minBounds = Minimize( minBounds, p );
		maxBounds = Maximize( maxBounds, p );
	}

	// finest level: cells proportional to the extents, aiming for TRIANGLES_PER_CELL triangles per cell;
	// every coarser level halves the cell count per axis until one cell covers everything
	Vector3 extents = maxBounds - minBounds;
	float largest = std::max( { extents.x, extents.y, extents.z, 1e-6f } );
	const float e[3] = { std::max( extents.x, largest * 1e-3f ), std::max( extents.y, largest * 1e-3f ), std::max( extents.z, largest * 1e-3f ) };
	float targetCells = std::clamp( float( triangleCount ) / TRIANGLES_PER_CELL, 1.f, MAX_CELLS );
	float cellEdge = std::cbrt( e[0] * e[1] * e[2] / targetCells );
	uint32_t dims[3];
	for( int a = 0; a < 3; ++a )
	{
		dims[a] = std::clamp( uint32_t( std::ceil( e[a] / cellEdge ) ), 1u, MAX_CELLS_PER_AXIS );
	}
	lod.levels.clear();
	uint32_t totalCells = 0;
	for( ;; )
	{
		DecalTriangleCache::Level level;
		level.origin = minBounds;
		level.cellSize = Vector3( e[0] / dims[0], e[1] / dims[1], e[2] / dims[2] );
		level.invCellSize = Vector3( dims[0] / e[0], dims[1] / e[1], dims[2] / e[2] );
		std::copy( dims, dims + 3, level.dims );
		level.firstCell = totalCells;
		totalCells += dims[0] * dims[1] * dims[2];
		lod.levels.push_back( level );
		if( dims[0] == 1 && dims[1] == 1 && dims[2] == 1 )
		{
			break;
		}
		for( auto& d : dims )
		{
			d = std::max( 1u, ( d + 1 ) / 2 );
		}
	}
	// the coarsest level is one cell: make sure it takes any triangle, including degenerate bounds
	lod.levels.back().cellSize = Vector3( FLT_MAX, FLT_MAX, FLT_MAX );

	// each triangle goes to the cell of its centre on the finest level whose cells it fits in
	std::vector<uint32_t> triangleCell( triangleCount );
	lod.cellStart.assign( size_t( totalCells ) + 1, 0 );
	for( size_t t = 0; t < triangleCount; ++t )
	{
		const Vector3& a = lod.positions[lod.indices[t * 3 + 0]];
		const Vector3& b = lod.positions[lod.indices[t * 3 + 1]];
		const Vector3& c = lod.positions[lod.indices[t * 3 + 2]];
		Vector3 lo = Minimize( Minimize( a, b ), c );
		Vector3 hi = Maximize( Maximize( a, b ), c );
		Vector3 size = hi - lo;
		Vector3 centre = ( lo + hi ) * 0.5f;
		size_t l = 0;
		while( size.x > lod.levels[l].cellSize.x || size.y > lod.levels[l].cellSize.y || size.z > lod.levels[l].cellSize.z )
		{
			++l;
		}
		const auto& level = lod.levels[l];
		uint32_t x = CellCoordinate( centre.x, level.origin.x, level.invCellSize.x, level.dims[0] );
		uint32_t y = CellCoordinate( centre.y, level.origin.y, level.invCellSize.y, level.dims[1] );
		uint32_t z = CellCoordinate( centre.z, level.origin.z, level.invCellSize.z, level.dims[2] );
		uint32_t cell = level.firstCell + ( z * level.dims[1] + y ) * level.dims[0] + x;
		triangleCell[t] = cell;
		++lod.cellStart[cell + 1];
	}
	for( uint32_t c = 0; c < totalCells; ++c )
	{
		lod.cellStart[c + 1] += lod.cellStart[c];
	}

	// counting sort the triangles by cell
	std::vector<uint32_t> sorted( lod.indices.size() );
	std::vector<uint32_t> cursor( lod.cellStart.begin(), lod.cellStart.end() - 1 );
	for( size_t t = 0; t < triangleCount; ++t )
	{
		uint32_t to = cursor[triangleCell[t]]++;
		std::copy( &lod.indices[t * 3], &lod.indices[t * 3] + 3, &sorted[size_t( to ) * 3] );
	}
	lod.indices.swap( sorted );
}
}

const DecalTriangleCache* DecalTriangleCache::Get( TriGeometryResMeshData* meshData )
{
	if( !meshData )
	{
		return nullptr;
	}

	auto& cache = meshData->m_decalTriangleCache;
	if( cache && cache->lodMask == meshData->m_lodMask && cache->lods.size() == meshData->m_lods.size() )
	{
		return cache->ready.load( std::memory_order_acquire ) ? cache.get() : nullptr;
	}

	TRINITY_STATS_ZONE( __FUNCTION__ );

	for( auto& lod : meshData->m_lods )
	{
		if( !lod->m_allocationsValid )
		{
			return nullptr;
		}
	}

	bool positionsCompressed = false;
	Tr2VertexDefinition vd;
	if( Tr2EffectStateManager::GetVertexDeclarationElements( meshData->m_vertexDeclarationHandle, vd ) )
	{
		if( auto pos = vd.Find( vd.POSITION ) )
		{
			positionsCompressed = pos->m_dataType == vd.FLOAT16_4;
		}
	}

	std::vector<RawLod> raw( meshData->m_lods.size() );
	for( size_t i = 0; i < meshData->m_lods.size(); ++i )
	{
		if( !CopyLod( *meshData, *meshData->m_lods[i], raw[i] ) )
		{
			return nullptr;
		}
	}

	auto newCache = std::make_shared<DecalTriangleCache>();
	newCache->lodMask = meshData->m_lodMask;
	newCache->lods.resize( meshData->m_lods.size() );
	cache = newCache;

	// the worker only touches its own copies, so the mesh may be released while it runs
	std::thread( [newCache, raw = std::move( raw ), bytesPerVertex = meshData->m_bytesPerVertex, positionsCompressed]() {
		for( size_t i = 0; i < raw.size(); ++i )
		{
			ConvertLod( raw[i], bytesPerVertex, positionsCompressed, newCache->lods[i] );
			BuildGrid( newCache->lods[i] );
		}
		newCache->ready.store( true, std::memory_order_release );
	} ).detach();
	return nullptr;
}

namespace
{
// Calls fn( triangle ) for each triangle of lod whose bounds touch [aabbMin, aabbMax].
template<typename Fn>
void ForEachTriangleNear( const DecalTriangleCache::Lod& lod, const Vector3& aabbMin, const Vector3& aabbMax, Fn&& fn )
{
	for( const auto& level : lod.levels )
	{
		// a triangle stored here is no larger than a cell, so its centre is within half a cell of the box
		Vector3 margin = Minimize( level.cellSize * 0.5f, Vector3( 1e30f, 1e30f, 1e30f ) );
		Vector3 lo = aabbMin - margin;
		Vector3 hi = aabbMax + margin;
		uint32_t x0 = CellCoordinate( lo.x, level.origin.x, level.invCellSize.x, level.dims[0] );
		uint32_t y0 = CellCoordinate( lo.y, level.origin.y, level.invCellSize.y, level.dims[1] );
		uint32_t z0 = CellCoordinate( lo.z, level.origin.z, level.invCellSize.z, level.dims[2] );
		uint32_t x1 = CellCoordinate( hi.x, level.origin.x, level.invCellSize.x, level.dims[0] );
		uint32_t y1 = CellCoordinate( hi.y, level.origin.y, level.invCellSize.y, level.dims[1] );
		uint32_t z1 = CellCoordinate( hi.z, level.origin.z, level.invCellSize.z, level.dims[2] );
		for( uint32_t z = z0; z <= z1; ++z )
		{
			for( uint32_t y = y0; y <= y1; ++y )
			{
				// cells x0..x1 of a row are contiguous, and so are their triangles
				uint32_t row = level.firstCell + ( z * level.dims[1] + y ) * level.dims[0];
				for( uint32_t t = lod.cellStart[row + x0]; t < lod.cellStart[row + x1 + 1]; ++t )
				{
					const uint32_t* tri = &lod.indices[size_t( t ) * 3];
					const Vector3& v0 = lod.positions[tri[0]];
					const Vector3& v1 = lod.positions[tri[1]];
					const Vector3& v2 = lod.positions[tri[2]];
					// cheap bounds reject before any exact test
					if( std::max( { v0.x, v1.x, v2.x } ) < aabbMin.x || std::min( { v0.x, v1.x, v2.x } ) > aabbMax.x ||
						std::max( { v0.y, v1.y, v2.y } ) < aabbMin.y || std::min( { v0.y, v1.y, v2.y } ) > aabbMax.y ||
						std::max( { v0.z, v1.z, v2.z } ) < aabbMin.z || std::min( { v0.z, v1.z, v2.z } ) > aabbMax.z )
					{
						continue;
					}
					fn( tri );
				}
			}
		}
	}
}

// A vertex position rounded to a tenth of a millimetre, so split vertices compare equal
struct VertexKey
{
	int64_t x, y, z;
	bool operator==( const VertexKey& other ) const
	{
		return x == other.x && y == other.y && z == other.z;
	}
	bool operator<( const VertexKey& other ) const
	{
		return x != other.x ? x < other.x : y != other.y ? y < other.y : z < other.z;
	}
};

VertexKey MakeKey( const Vector3& p )
{
	return { int64_t( std::llround( p.x * 1e4f ) ), int64_t( std::llround( p.y * 1e4f ) ), int64_t( std::llround( p.z * 1e4f ) ) };
}

struct EdgeKey
{
	VertexKey a, b;
	bool operator==( const EdgeKey& other ) const
	{
		return a == other.a && b == other.b;
	}
};

struct EdgeKeyHash
{
	size_t operator()( const EdgeKey& key ) const
	{
		size_t h = 1469598103934665603ull;
		for( int64_t v : { key.a.x, key.a.y, key.a.z, key.b.x, key.b.y, key.b.z } )
		{
			h = ( h ^ size_t( v ) ) * 1099511628211ull;
		}
		return h;
	}
};

struct EdgeFaces
{
	Vector3 p0, p1;
	Vector3 normals[2];
	uint32_t count = 0;
};

Vector3 ClosestPointOnSegment( const Vector3& p, const Vector3& a, const Vector3& b )
{
	Vector3 ab = b - a;
	float lengthSq = Dot( ab, ab );
	float t = lengthSq > 0.f ? std::clamp( Dot( p - a, ab ) / lengthSq, 0.f, 1.f ) : 0.f;
	return a + ab * t;
}
}

void DecalTriangleCache::Query( size_t lodIndex, const Vector3& aabbMin, const Vector3& aabbMax, const Matrix& inverseDecalMatrix, const Vector3& projectionAxis, float minFacing, std::vector<uint32_t>& out ) const
{
	if( lodIndex >= lods.size() )
	{
		return;
	}
	const Lod& lod = lods[lodIndex];
	ForEachTriangleNear( lod, aabbMin, aabbMax, [&]( const uint32_t* tri ) {
		const Vector3* v0 = &lod.positions[tri[0]];
		const Vector3* v1 = &lod.positions[tri[1]];
		const Vector3* v2 = &lod.positions[tri[2]];
		if( minFacing > 0.f )
		{
			Vector3 faceNormal = Cross( *v1 - *v0, *v2 - *v0 );
			float length = Length( faceNormal );
			if( length < 1e-12f || std::abs( Dot( faceNormal, projectionAxis ) ) < minFacing * length )
			{
				return;
			}
		}
		if( IntersectTriangleAABB( v0, v1, v2, aabbMin, aabbMax ) && IntersectTriangleOrientedBox( v0, v1, v2, inverseDecalMatrix ) )
		{
			out.insert( out.end(), tri, tri + 3 );
		}
	} );
}

bool DecalTriangleCache::FindNearestEdge( size_t lodIndex, const Vector3& point, float radius, float sharpCos, Vector3& outPosition, Vector3& outNormal, float& outDistance ) const
{
	if( lodIndex >= lods.size() || radius <= 0.f )
	{
		return false;
	}
	const Lod& lod = lods[lodIndex];
	const Vector3 extent( radius, radius, radius );

	// Every edge that comes within radius of point lies in this box, and so does every
	// triangle on either side of it: an edge's faces are all gathered, so a fold is never
	// mistaken for an open edge because its other face was out of the query.
	std::unordered_map<EdgeKey, EdgeFaces, EdgeKeyHash> edges;
	ForEachTriangleNear( lod, point - extent, point + extent, [&]( const uint32_t* tri ) {
		const Vector3& v0 = lod.positions[tri[0]];
		const Vector3& v1 = lod.positions[tri[1]];
		const Vector3& v2 = lod.positions[tri[2]];
		Vector3 faceNormal = Cross( v1 - v0, v2 - v0 );
		float area = Length( faceNormal );
		if( area < 1e-8f )
		{
			return;
		}
		faceNormal = faceNormal / area;
		const Vector3* corners[3] = { &v0, &v1, &v2 };
		for( int i = 0; i < 3; ++i )
		{
			const Vector3& a = *corners[i];
			const Vector3& b = *corners[( i + 1 ) % 3];
			VertexKey ka = MakeKey( a );
			VertexKey kb = MakeKey( b );
			if( ka == kb )
			{
				continue;
			}
			EdgeKey key = kb < ka ? EdgeKey{ kb, ka } : EdgeKey{ ka, kb };
			EdgeFaces& faces = edges[key];
			if( faces.count == 0 )
			{
				faces.p0 = a;
				faces.p1 = b;
			}
			if( faces.count < 2 )
			{
				faces.normals[faces.count] = faceNormal;
			}
			++faces.count;
		}
	} );

	bool found = false;
	float bestDistance = radius;
	for( const auto& entry : edges )
	{
		const EdgeFaces& faces = entry.second;
		Vector3 normal;
		if( faces.count == 1 )
		{
			normal = faces.normals[0];
		}
		else if( faces.count == 2 && Dot( faces.normals[0], faces.normals[1] ) < sharpCos )
		{
			normal = faces.normals[0] + faces.normals[1];
			float length = Length( normal );
			// two faces folded flat onto each other (a thin fin): either face will do
			normal = length > 1e-4f ? normal / length : faces.normals[0];
		}
		else
		{
			// a smooth fold, or a non-manifold edge shared by more than two triangles
			continue;
		}
		Vector3 closest = ClosestPointOnSegment( point, faces.p0, faces.p1 );
		float distance = Length( closest - point );
		if( distance <= bestDistance )
		{
			found = true;
			bestDistance = distance;
			outPosition = closest;
			outNormal = normal;
		}
	}
	outDistance = bestDistance;
	return found;
}

size_t DecalTriangleCache::GetMemoryUsage() const
{
	size_t bytes = sizeof( *this );
	for( const auto& lod : lods )
	{
		bytes += sizeof( lod );
		bytes += lod.positions.capacity() * sizeof( Vector3 );
		bytes += lod.indices.capacity() * sizeof( uint32_t );
		bytes += lod.levels.capacity() * sizeof( Level );
		bytes += lod.cellStart.capacity() * sizeof( uint32_t );
	}
	return bytes;
}
