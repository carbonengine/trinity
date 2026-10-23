// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Include/TriMath.h"
#include "Tr2GeometryProvider.h"
#include "Tr2PerObjectData.h"
#include "Tr2VertexDefinitionUtilities.h"
#include "TriDevice.h"

#include "TriSettingsRegistrar.h"

#include "Utilities/GeometryUtils.h"
#include "Utilities/BoundingSphere.h"
#include "Utilities/BoundingBox.h"

#include <cctype>
#include "TbbStub.h"

using namespace Tr2RenderContextEnum;

std::atomic<uint64_t> AudioGeometryResData::s_nextId{ 1 };

bool g_eveIsAudioOcclusionGeometryEnabled = false;
TRI_REGISTER_SETTING( "eveIsAudioOcclusionGeometryEnabled", g_eveIsAudioOcclusionGeometryEnabled );

CCP_STATS_DECLARE( geometryResBytes, "Trinity/geometryResBytes", false, CST_MEMORY, "Size of memory occupied by geometry resources." );

#if TRINITY_PLATFORM != TRINITY_DIRECTX11
const auto gpuUsage = Tr2GpuUsage::VERTEX_BUFFER | Tr2GpuUsage::INDEX_BUFFER | Tr2GpuUsage::SHADER_RESOURCE;
#else
const auto gpuUsage = Tr2GpuUsage::VERTEX_BUFFER | Tr2GpuUsage::INDEX_BUFFER;
#endif

Tr2SuballocatedBuffer g_sharedBuffer( "TriGeometryRes shared vertex/index buffer", gpuUsage, SHARED_BUFFER_BLOCK_SIZE, SHARED_BUFFER_MAX_SIZE );

TriMorphTargetGeometryConstants CreateMorphGeometryConstants( const Tr2VertexDefinition& vertexDefinition, uint32_t stride, uint32_t vertexCount, Tr2RenderContext& renderContext )
{
	TriMorphTargetGeometryConstants data = TriMorphTargetGeometryConstants{};
	for( const auto& item : vertexDefinition.m_items )
	{
		if( item.m_usage == Tr2VertexDefinition::POSITION && item.m_usageIndex == 0 && item.m_stream == 0 )
		{
			data.positionOffset = item.m_offset;
			data.positionType = item.m_dataType;

			uint32_t type = data.positionType;
			CCP_ASSERT_M(
				type == Tr2VertexDefinition::DataType::FLOAT32_3,
				"position type has to be FLOAT32_3!" );
		}

		if( item.m_usage == Tr2VertexDefinition::TANGENT && item.m_usageIndex == 0 && item.m_stream == 0 )
		{
			data.tangentOffset = item.m_offset;
			data.tangentType = item.m_dataType;

			uint32_t type = data.tangentType;
			CCP_ASSERT_M(
				type == Tr2VertexDefinition::DataType::FLOAT16_3 ||
					type == Tr2VertexDefinition::DataType::FLOAT32_3 ||
					type == Tr2VertexDefinition::DataType::UBYTE_4_NORM ||
					type == Tr2VertexDefinition::DataType::USHORT_4_NORM ||
					type == Tr2VertexDefinition::DataType::SHORT_4_NORM,
				"tangent type has to be FLOAT16_3 or FLOAT32_3 or UBYTE_4_NORM or USHORT_4_NORM or SHORT_4_NORM!" );
		}
	}

	data.vertexBufferStride = stride;
	data.vertexCount = vertexCount;
	return data;
}

uint32_t GetPrimitiveCount( const TriGeometryResLodData& lod, uint32_t index, uint32_t count )
{
	if( index >= lod.m_areas.size() )
	{
		return 0;
	}

	if( index + count > lod.m_areas.size() )
	{
		count = uint32_t( lod.m_areas.size() - index );
	}

	auto& meshArea = lod.m_areas[index];

	uint32_t primCount = uint32_t( meshArea.m_primitiveCount );
	for( uint32_t i = 1; i < count; ++i )
	{
		primCount += lod.m_areas[index + i].m_primitiveCount;
	}
	return primCount;
}

TriGeometryAreaIndexRange GetAreaIndexRange( const TriGeometryResLodData& lod, uint32_t areaIndex, uint32_t primCount, bool reversed )
{
	TriGeometryAreaIndexRange range;
	if( primCount == 0 || areaIndex >= lod.m_areas.size() )
	{
		return range;
	}
	if( reversed && !lod.m_reversedIndicesValid )
	{
		return range;
	}
	range.indices = reversed ? &lod.m_reversedIndexAllocation : &lod.m_indexAllocation;
	range.startIndex = reversed ?
		range.indices->GetStartIndex() + lod.m_primitiveCount * 3 - lod.m_areas[areaIndex].m_firstIndex - primCount * 3 :
		range.indices->GetStartIndex() + lod.m_areas[areaIndex].m_firstIndex;
	range.valid = true;
	return range;
}


Tr2GeometryProvider::Tr2GeometryProvider( IRoot* lockobj ) :
	m_memoryUse( 0 ),
	m_meshes( "Tr2GeometryProvider/m_meshes" ),
	m_skeletons( "Tr2GeometryProvider/m_skeletons" )
{
}

unsigned int Tr2GeometryProvider::GetMeshCount() const
{
	return unsigned( m_meshes.size() );
}

TriGeometryResMeshData* Tr2GeometryProvider::GetMeshData( unsigned int meshIx ) const
{
	if( !m_isGood || meshIx >= m_meshes.size() )
	{
		return nullptr;
	}

	return m_meshes[meshIx].get();
}

TriGeometryResLodData* Tr2GeometryProvider::GetMeshLod( unsigned int meshIx, float screenSize ) const
{
	auto mesh = GetMeshData( meshIx );

	if( !mesh )
	{
		return nullptr;
	}

	int lodIndex = GetLodIndexForScreenSize( meshIx, screenSize );
	return mesh->m_lods[lodIndex].get();
}

TriGeometryResLodData* Tr2GeometryProvider::GetMeshLod( unsigned int meshIx, int lodIndex ) const
{

	auto mesh = GetMeshData( meshIx );

	if( !mesh || lodIndex < 0 || lodIndex >= mesh->m_lods.size() )
	{
		return nullptr;
	}

	return mesh->m_lods[lodIndex].get();
}

int Tr2GeometryProvider::GetLodIndexForScreenSize( unsigned int meshIx, float screenSize ) const
{
	auto mesh = GetMeshData( meshIx );

	if( !mesh )
	{
		return -1;
	}

	int32_t lastLod = static_cast<int32_t>( mesh->m_lods.size() ) - 1;
	if( m_forceLod && m_forcedLodIndex >= 0 )
	{
		return int( std::min( m_forcedLodIndex, lastLod ) );
	}

	for( int32_t i = lastLod; i >= 0; i-- )
	{
		if( mesh->m_lods[i]->m_maxScreenSize >= screenSize )
		{
			return i;
		}
	}
	//If we end up here, it's because we requested a LOD size larger than the best one we have.
	//Just return the highest quality LOD in this case.
	return 0;
}

const AudioGeometryResData* Tr2GeometryProvider::GetAudioGeometry( unsigned int meshIx ) const
{
	if( !g_eveIsAudioOcclusionGeometryEnabled )
		return nullptr;

	auto mesh = GetMeshData( meshIx );
	if( !mesh || !mesh->m_audioGeometry )
	{
		return nullptr;
	}

	return mesh->m_audioGeometry.get();
}

bool Tr2GeometryProvider::GetBoundingBox( unsigned int meshIx, Vector3& min, Vector3& max ) const
{
	TriGeometryResMeshData* mesh = GetMeshData( meshIx );
	if( !mesh )
	{
		return false;
	}

	min = mesh->m_minBounds;
	max = mesh->m_maxBounds;

	return true;
}

Be::Result<std::string> Tr2GeometryProvider::GetBoundingBoxFromScript( unsigned int meshIx, std::pair<Vector3, Vector3>& bounds ) const
{
	if( meshIx >= m_meshes.size() )
	{
		return Be::Result<std::string>( "Mesh index out of bounds" );
	}
	TriGeometryResMeshData* mesh = m_meshes[meshIx].get();
	if( !mesh )
	{
		return Be::Result<std::string>( "Invalid mesh" );
	}

	bounds = std::make_pair( mesh->m_minBounds, mesh->m_maxBounds );

	return Be::Result<std::string>();
}

bool Tr2GeometryProvider::GetAreaBoundingBox( unsigned int meshIx, unsigned int areaIx, Vector3& min, Vector3& max ) const
{
	auto mesh = GetMeshData( meshIx );
	if( !mesh )
	{
		return false;
	}

	//Get the first LOD.
	auto& lod = mesh->m_lods[0];

	// Bail out if the area index is out of range
	if( areaIx >= lod->m_areas.size() )
	{
		return false;
	}

	// Finally, get the min and max bounds for the mesh area
	min = lod->m_areas[areaIx].m_minBounds;
	max = lod->m_areas[areaIx].m_maxBounds;

	return true;
}


bool Tr2GeometryProvider::GetBoundingSphere( unsigned int meshIx, Vector4& sphere ) const
{
	TriGeometryResMeshData* mesh = GetMeshData( meshIx );
	if( !mesh )
	{
		return false;
	}

	sphere = mesh->m_boundingSphere;

	return true;
}

unsigned int Tr2GeometryProvider::GetAreaCount( unsigned int meshIx ) const
{
	auto mesh = GetMeshData( meshIx );
	if( !mesh )
	{
		return 0;
	}

	//Get the first LOD.
	auto& lod = mesh->m_lods[0];

	return (unsigned int)lod->m_areas.size();
}

TriGeometryResAreaData* Tr2GeometryProvider::GetAreaData( unsigned int meshIx, unsigned int areaIx ) const
{
	auto mesh = GetMeshData( meshIx );
	if( !mesh )
	{
		return NULL;
	}

	//Get the first LOD.
	auto& lod = mesh->m_lods[0];

	if( areaIx < 0 || areaIx >= lod->m_areas.size() )
	{
		return NULL;
	}

	return &lod->m_areas[areaIx];
}

#if TRINITYDEV
void Tr2GeometryProvider::GetDescription( std::string& desc )
{
	desc = "Tr2GeometryProvider: '";
	desc += CW2A( GetPath() );
	desc += "'";
}
#endif

//This entire function is sus. We should be able to compute this when we upload the mesh to the GPU, as it can never change unless the vertex data changes.
void Tr2GeometryProvider::RecalculateBoundingSphere()
{
	TRINITY_STATS_ZONE( __FUNCTION__ );
	USE_MAIN_THREAD_RENDER_CONTEXT();

	for( auto& mesh : m_meshes )
	{
		//Try to get the vertex declaration elements
		Tr2VertexDefinition decl;
		if( mesh == nullptr || !Tr2EffectStateManager::GetVertexDeclarationElements( mesh->m_vertexDeclarationHandle, decl ) )
		{
			continue;
		}

		uint32_t vertSize = mesh->m_bytesPerVertex;

		//For now, we just compute the bounding sphere of the first LOD and ignore the lower LODs, assuming that they should fit in the highest quality LOD's bounding sphere.
		auto& lod = mesh->m_lods[0];


		//Get the vertex data of the

		const uint8_t* pVertices;
		if( !lod->m_allocationsValid || FAILED( lod->m_vertexAllocation.MapForReading( pVertices, renderContext ) ) )
		{
			continue;
		}
		auto position = decl.Find( decl.POSITION, 0 );

		// build a list of pointers to the positions
		std::vector<const Vector3*> points( lod->m_vertexCount );
		for( uint32_t p = 0; p < lod->m_vertexCount; ++p )
		{
			points[p] = (const Vector3*)&pVertices[p * vertSize + position->m_offset];
		}

		// all is done in this recursive function
		BoundingSphereFromPoints( mesh->m_boundingSphere, &points[0], points.size() );
		lod->m_vertexAllocation.UnmapForReading( renderContext );
	}
}

struct CalcBoundingBoxContext
{
	Vector3 min;
	Vector3 max;
	Matrix transform;
};

void CalcBoundingBox( void* context, const Vector3& p1, const Vector3& p2, const Vector3& p3 )
{
	CalcBoundingBoxContext* ctx = static_cast<CalcBoundingBoxContext*>( context );

	Vector4 v[3];
	v[0] = Vector4( p1.x, p1.y, p1.z, 1.0f );
	v[1] = Vector4( p2.x, p2.y, p2.z, 1.0f );
	v[2] = Vector4( p3.x, p3.y, p3.z, 1.0f );

	for( int i = 0; i < 3; ++i )
	{
		v[i] = Transform( v[i], ctx->transform );
		v[i] /= v[i].w;

		ctx->max.x = std::max( ctx->max.x, v[i].x );
		ctx->max.y = std::max( ctx->max.y, v[i].y );
		ctx->max.z = std::max( ctx->max.z, v[i].z );

		ctx->min.x = std::min( ctx->min.x, v[i].x );
		ctx->min.y = std::min( ctx->min.y, v[i].y );
		ctx->min.z = std::min( ctx->min.z, v[i].z );
	}
}

static void ConvertTriangleData( Tr2VertexDefinition::DataType elementType, unsigned int vertexByteSize, const uint8_t* vertexBase, unsigned int index1, unsigned int index2, unsigned int index3, Vector3* v1, Vector3* v2, Vector3* v3 )
{

	ConvertDataToVector3( elementType, vertexBase + index1 * vertexByteSize, v1 );
	ConvertDataToVector3( elementType, vertexBase + index2 * vertexByteSize, v2 );
	ConvertDataToVector3( elementType, vertexBase + index3 * vertexByteSize, v3 );
}

void Tr2GeometryProvider::ProcessMeshTriangles( int meshIx, PerTriangleCallback cb, void* cbContext )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );
	USE_MAIN_THREAD_RENDER_CONTEXT();

	int i = meshIx;

	if( m_meshes[i] == NULL )
	{
		return;
	}

	auto& mesh = m_meshes[i];

	auto& lod = mesh->m_lods[0];

	if( !lod->m_allocationsValid )
	{
		return;
	}

	const uint8_t* pVertices;
	const uint8_t* pIndices;

	int vertSize = mesh->m_bytesPerVertex;

	if( FAILED( lod->m_vertexAllocation.MapForReading( pVertices, renderContext ) ) )
	{
		return;
	}
	ON_BLOCK_EXIT( [&] { lod->m_vertexAllocation.UnmapForReading( renderContext ); } );
	if( FAILED( lod->m_indexAllocation.MapForReading( pIndices, renderContext ) ) )
	{
		return;
	}
	ON_BLOCK_EXIT( [&] { lod->m_indexAllocation.UnmapForReading( renderContext ); } );

	const uint16_t* pShortIndices = (uint16_t*)pIndices;
	const uint32_t* pLongIndices = (uint32_t*)pIndices;


	Tr2VertexDefinition decl;
	if( !Tr2EffectStateManager::GetVertexDeclarationElements( mesh->m_vertexDeclarationHandle, decl ) )
	{
		return;
	}

	auto foundPosition = decl.Find( Tr2VertexDefinition::POSITION, 0 );
	if( !foundPosition )
	{
		return;
	}
	unsigned int positionByteOffset = foundPosition->m_offset;
	Tr2VertexDefinition::DataType declType = foundPosition->m_dataType;

	int numPrim = lod->m_primitiveCount;
	for( int j = 0; j < numPrim; j++ )
	{
		unsigned int index1 = 0;
		unsigned int index2 = 0;
		unsigned int index3 = 0;
		Vector3 p1;
		Vector3 p2;
		Vector3 p3;
		if( lod->m_indexAllocation.GetStride() == 2 )
		{
			index1 = pShortIndices[j * 3];
			index2 = pShortIndices[( j * 3 ) + 1];
			index3 = pShortIndices[( j * 3 ) + 2];
		}
		else
		{
			index1 = pLongIndices[j * 3];
			index2 = pLongIndices[( j * 3 ) + 1];
			index3 = pLongIndices[( j * 3 ) + 2];
		}

		ConvertTriangleData( declType, vertSize, pVertices + positionByteOffset, index1, index2, index3, &p1, &p2, &p3 );

		( *cb )( cbContext, p1, p2, p3 );
	}
}

std::pair<bool, std::pair<int, std::pair<Vector3, Vector3>>> Tr2GeometryProvider::GetIntersectionPointNormalBoneFromScript( const Vector3& pos, const Vector3& dir )
{
	RayCastResult hitInfo;
	bool hit = GetIntersectionPoints( pos, dir, hitInfo );
	Vector3 normal = Normalize( hitInfo.unnormalizedNormal );
	return std::make_pair( hit, std::make_pair( hitInfo.boneIndex, std::make_pair( hitInfo.position, normal ) ) );
}

Be::Result<std::string> Tr2GeometryProvider::GetAreaIntersectionPointNormalBoneFromScript( const Vector3& pos, const Vector3& dir, int areaIx, std::pair<bool, std::pair<int, std::pair<Vector3, Vector3>>>& result )
{
	if( areaIx < -1 )
	{
		// -1 is a special case handled to maintain legacy behavior.
		return Be::Result<std::string>( "Invalid area index" );
	}

	// In Python 3, passing a signed value to an unsigned C exposed attribute is an error.
	// In order to maintain backwards compatible behavior with Python 2
	// this method got changed to accept signed integers and cast them to unsigned.
	auto unsignedAreaIndex = static_cast<unsigned int>( areaIx );

	RayCastResult hitInfo;
	bool hit = GetIntersectionPoints( pos, dir, hitInfo, unsignedAreaIndex );
	Vector3 normal = Normalize( hitInfo.unnormalizedNormal );
	result = std::make_pair( hit, std::make_pair( hitInfo.boneIndex, std::make_pair( hitInfo.position, normal ) ) );
	return Be::Result<std::string>();
}

std::pair<bool, std::pair<int, std::pair<Vector3, std::pair<Vector3, std::pair<Color, Color>>>>> Tr2GeometryProvider::GetIntersectionPointNormalBoneColorFromScript( const Vector3& pos, const Vector3& dir )
{
	RayCastResult hitInfo;
	bool hit = GetIntersectionPoints( pos, dir, hitInfo );
	Vector3 normal = Normalize( hitInfo.unnormalizedNormal );
	return std::make_pair( hit, std::make_pair( hitInfo.boneIndex, std::make_pair( hitInfo.position, std::make_pair( normal, std::make_pair( hitInfo.firstVertexColor, hitInfo.interpolatedVertexColor ) ) ) ) );
}

Be::Result<std::string> Tr2GeometryProvider::GetAreaIntersectionPointNormalBoneColorFromScript( const Vector3& pos, const Vector3& dir, int areaIx, std::pair<bool, std::pair<int, std::pair<Vector3, std::pair<Vector3, std::pair<Color, Color>>>>>& result )
{
	if( areaIx < -1 )
	{
		// -1 is a special case handled to maintain legacy behavior.
		return Be::Result<std::string>( "Invalid area index" );
	}

	// In Python 3, passing a signed value to an unsigned C exposed attribute is an error.
	// In order to maintain backwards compatible behavior with Python 2
	// this method got changed to accept signed integers and cast them to unsigned.
	auto unsignedAreaIndex = static_cast<unsigned int>( areaIx );

	RayCastResult hitInfo;
	bool hit = GetIntersectionPoints( pos, dir, hitInfo, unsignedAreaIndex );
	Vector3 normal = Normalize( hitInfo.unnormalizedNormal );
	result = std::make_pair( hit, std::make_pair( hitInfo.boneIndex, std::make_pair( hitInfo.position, std::make_pair( normal, std::make_pair( hitInfo.firstVertexColor, hitInfo.interpolatedVertexColor ) ) ) ) );
	return Be::Result<std::string>();
}

void Tr2GeometryProvider::PrepareRayCaster()
{
}

void Tr2GeometryProvider::DestroyRayCaster()
{
}

void Tr2GeometryProvider::ResetRayCaster()
{
}

bool Tr2GeometryProvider::IsRayCasterReady() const
{
	return false;
}

bool Tr2GeometryProvider::HasRayCasterPreparationFailed() const
{
	return true;
}

bool Tr2GeometryProvider::GetIntersectionPoints( const Vector3& pos, const Vector3& dir, RayCastResult& result, uint32_t areaIx, float rayLength )
{
	if( BeResMan->IsOnMainThread() )
	{
		return GetIntersectionPointsLegacy( pos, dir, result, areaIx, rayLength );
	}
	CCP_ASSERT_M( false, "Raycast fallback is not available in a multithreaded environment! You need to wait for IsRayCasterReady before raycasting!" );
	return false;
}

// Careful: This function is slow and only works on main thread!
bool Tr2GeometryProvider::GetIntersectionPointsLegacy( const Vector3& pos, const Vector3& dir, RayCastResult& result, uint32_t areaIx, float rayLength )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	bool hit = false;

	for( size_t i = 0; i < m_meshes.size(); i++ )
	{
		if( m_meshes[i] == NULL )
		{
			continue;
		}

		//Get the first LOD.
		auto& lod = m_meshes[i]->m_lods[0];

		const uint8_t* pVertices;
		const uint8_t* pIndices;

		int vertSize = m_meshes[i]->m_bytesPerVertex;
		if( FAILED( lod->m_vertexAllocation.MapForReading( pVertices, renderContext ) ) )
		{
			return false;
		}
		ON_BLOCK_EXIT( [&] { lod->m_vertexAllocation.UnmapForReading( renderContext ); } );
		if( FAILED( lod->m_indexAllocation.MapForReading( pIndices, renderContext ) ) )
		{
			return false;
		}
		ON_BLOCK_EXIT( [&] { lod->m_indexAllocation.UnmapForReading( renderContext ); } );

		const uint16_t* pShortIndices = (uint16_t*)pIndices;
		const uint32_t* pLongIndices = (uint32_t*)pIndices;

		Tr2VertexDefinition decl;
		if( !Tr2EffectStateManager::GetVertexDeclarationElements( m_meshes[i]->m_vertexDeclarationHandle, decl ) )
		{
			return false;
		}
		const Tr2VertexDefinition::Item* const position = decl.Find( decl.POSITION );
		if( !position )
		{
			return false;
		}

		const Tr2VertexDefinition::Item* const blendIndices = decl.Find( decl.BLENDINDICES );
		const Tr2VertexDefinition::Item* const colors = decl.Find( decl.COLOR );

		int numPrim = lod->m_primitiveCount;
		auto currentIndex = 0;
		if( areaIx != -1 )
		{
			if( areaIx >= lod->m_areas.size() )
			{
				continue;
			}
			currentIndex = lod->m_areas[areaIx].m_firstIndex;
			numPrim = lod->m_areas[areaIx].m_primitiveCount;
		}

		for( int j = 0; j < numPrim; j++ )
		{
			unsigned int index1 = 0;
			unsigned int index2 = 0;
			unsigned int index3 = 0;
			Vector3 p1;
			Vector3 p2;
			Vector3 p3;
			float pu, pv, dist;
			if( lod->m_indexAllocation.GetStride() == 2 )
			{
				index1 = pShortIndices[currentIndex++];
				index2 = pShortIndices[currentIndex++];
				index3 = pShortIndices[currentIndex++];
			}
			else
			{
				index1 = pLongIndices[currentIndex++];
				index2 = pLongIndices[currentIndex++];
				index3 = pLongIndices[currentIndex++];
			}

			ConvertTriangleData( position->m_dataType, vertSize, pVertices + position->m_offset, index1, index2, index3, &p1, &p2, &p3 );

			if( IntersectTri( p1, p2, p3, pos, dir, pu, pv, dist ) )
			{
				if( rayLength > dist )
				{
					float v1 = 1.0f - ( pu + pv );

					result.position = p1 * v1 + p2 * pu + p3 * pv;

					Vector3 avec = p2 - p1;
					Vector3 bvec = p3 - p1;
					result.unnormalizedNormal = Cross( avec, bvec );

					result.distance = rayLength = dist;

					int boneIndex = 0;
					if( blendIndices && GetBoneIndex( blendIndices->m_dataType, pVertices + index1 * vertSize + blendIndices->m_offset, boneIndex ) )
					{
						result.boneIndex = boneIndex;
					}

					Color color1;
					Color color2;
					Color color3;
					if( colors &&
						GetColor( colors->m_dataType, pVertices + index1 * vertSize + colors->m_offset, color1 ) &&
						GetColor( colors->m_dataType, pVertices + index2 * vertSize + colors->m_offset, color2 ) &&
						GetColor( colors->m_dataType, pVertices + index3 * vertSize + colors->m_offset, color3 ) )
					{
						result.firstVertexColor = color1;
						result.interpolatedVertexColor = color1 * v1 + color2 * pu + color3 * pv;
					}

					hit = true;
				}
			}
		}
	}

	return hit;
}

unsigned int Tr2GeometryProvider::GetSkeletonCount() const
{
	return (unsigned int)m_skeletons.size();
}

TriGeometryResSkeletonData* Tr2GeometryProvider::GetSkeletonData( unsigned int skelIx ) const
{
	CCP_ASSERT( skelIx < m_skeletons.size() );

	return m_skeletons[skelIx].get();
}

bool Tr2GeometryProvider::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t Tr2GeometryProvider::GetMemoryUsage()
{
	return m_memoryUse;
}

void Tr2GeometryProvider::ReleaseResourcesHelper()
{
	m_meshes.clear();
	m_skeletons.clear();

	CCP_STATS_ADD( geometryResBytes, -(int)m_memoryUse );
	m_memoryUse = 0;
}

bool Tr2GeometryProvider::IsUsingCMF() const
{
	return true;
}

unsigned int TriGeometryResSkeletonData::FindJoint( const char* name ) const
{
	unsigned int jointCount = (unsigned int)m_joints.size();
	for( unsigned int ix = 0; ix < jointCount; ++ix )
	{
		if( strcmp( name, m_joints[ix].m_name.c_str() ) == 0 )
		{
			return ix;
		}
	}

	return 0xffffffff;
}

TriGeometryResSkeletonData::TriGeometryResSkeletonData() :
	m_joints( "TriGeometryResSkeletonData/m_joints" )
{
}

TriGeometryResAreaData::TriGeometryResAreaData() :
	m_firstIndex( 0 ),
	m_primitiveCount( 0 ),
	m_jointBindings( "TriGeometryResAreaData/m_jointBindings" ),
	m_isSkinned( false ),
	m_isMorphed( false )
{
}

TriGeometryResLodData::TriGeometryResLodData() :
	m_vertexCount( 0 ),
	m_primitiveCount( 0 ),
	m_areas( "TriGeometryResMeshData/m_areas" ),
	m_morphVertexDeclaration( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_allocationsValid( false ),
	m_reversedIndicesValid( false ),
	m_uvDensities(),
	m_bytesPerMorphTargetVertex( 0 )
{
}


TriGeometryResMeshData::TriGeometryResMeshData() :

	m_vertexDeclarationHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_bytesPerVertex( -1 ),
	m_jointBindings( "TriGeometryResMeshData/m_jointBindings" ),
	m_lods( "TriGeometryResModelData/m_lods" )
{
}

bool Tr2GeometryProvider::RenderAreas( unsigned int meshIx, unsigned int areaIx, unsigned int areaCount, Tr2RenderContext& renderContext, bool reversed )
{
	return RenderAreas( std::numeric_limits<float>::max(), meshIx, areaIx, areaCount, renderContext, reversed );
}

bool Tr2GeometryProvider::RenderAreas( float screenSize, unsigned int meshIx, unsigned int areaIx, unsigned int areaCount, Tr2RenderContext& renderContext, bool reversed )
{
	if( !m_isGood )
	{
		return false;
	}

	if( meshIx >= m_meshes.size() )
	{
		return false;
	}

	TriGeometryResMeshData* mesh = m_meshes[meshIx].get();
	if( !mesh )
	{
		return false;
	}

	TriGeometryResLodData* lod = GetMeshLod( meshIx, screenSize );

	unsigned int primCount = GetPrimitiveCount( *lod, areaIx, areaCount );

	if( primCount )
	{
		auto indexRange = GetAreaIndexRange( *lod, areaIx, primCount, reversed );
		if( !indexRange.valid )
		{
			return false;
		}

		renderContext.m_esm.ApplyVertexDeclaration( mesh->m_vertexDeclarationHandle );
		renderContext.m_esm.ApplyStreamSource( 0, lod->m_vertexAllocation );
		renderContext.m_esm.ApplyIndexBuffer( *indexRange.indices );

		renderContext.SetTopology( TOP_TRIANGLES );
		renderContext.DrawIndexedPrimitive( lod->m_vertexCount, indexRange.startIndex, primCount );
	}

	return true;
}

Be::Result<std::string> Tr2GeometryProvider::SaveMesh( const char* filename, uint32_t meshIndex ) const
{
	auto meshData = GetMeshData( meshIndex );
	if( !meshData )
	{
		return Be::Result<std::string>( "Invalid mesh index" );
	}

	if( !SaveMeshToCMFFile( *meshData, filename ) )
	{
		return Be::Result<std::string>( "Failed to save the mesh" );
	}

	return Be::Result<std::string>();
}

bool Tr2GeometryProvider::SaveMeshToCMFFile( TriGeometryResMeshData& mesh, const char* filename )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( mesh.m_vertexDeclarationHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return false;
	}

	auto& lod = mesh.m_lods[0];

	if( !lod->m_allocationsValid )
	{
		return false;
	}

	Tr2VertexDefinition decl;
	if( !Tr2EffectStateManager::GetVertexDeclarationElements( mesh.m_vertexDeclarationHandle, decl ) )
	{
		return false;
	}

	cmf::MemoryAllocator allocator;
	cmf::BufferManager bufferAllocator( allocator );
	cmf::Span<cmf::VertexElement> cmfVertexDecl = allocator.AllocateSpan<cmf::VertexElement>( decl.m_items.size() );

	if( !ConvertVertexDeclToCMF( decl, cmfVertexDecl, (uint32_t)cmfVertexDecl.size() ) )
	{
		return false;
	}

	uint32_t vertexBufferSize = lod->m_vertexAllocation.GetSize();
	uint32_t vertexBufferStride = lod->m_vertexAllocation.GetStride();
	const void* pVertices;
	if( FAILED( lod->m_vertexAllocation.MapForReading( pVertices, renderContext ) ) )
	{
		return false;
	}
	ON_BLOCK_EXIT( [&] { lod->m_vertexAllocation.UnmapForReading( renderContext ); } );

	uint32_t indexBufferSize = lod->m_indexAllocation.GetSize();
	uint32_t indexBufferStride = lod->m_indexAllocation.GetStride();
	const void* pIndices;
	if( FAILED( lod->m_indexAllocation.MapForReading( pIndices, renderContext ) ) )
	{
		return false;
	}
	ON_BLOCK_EXIT( [&] { lod->m_indexAllocation.UnmapForReading( renderContext ); } );


	cmf::Data cmfData;
	cmfData.meshes = allocator.AllocateSpan<cmf::Mesh>( 1 );

	cmf::Mesh& cmfMesh = cmfData.meshes[0] = {};
	cmfMesh.name = allocator.AllocateString( mesh.m_name );
	cmfMesh.decl = cmfVertexDecl;
	cmfMesh.lods = allocator.AllocateSpan<cmf::MeshLod>( 1 );
	cmfMesh.areas = allocator.AllocateSpan<cmf::MeshArea>( lod->m_areas.size() );
	cmfMesh.boneBindings = allocator.AllocateSpan<cmf::BoneBinding>( mesh.m_jointBindings.size() );
	cmfMesh.uvDensities = allocator.AllocateSpan<float>( lod->m_uvDensities.size() );
	cmfMesh.bounds = AxisAlignedBoundingBox( mesh.m_minBounds, mesh.m_maxBounds );

	memcpy( cmfMesh.uvDensities.data(), lod->m_uvDensities.data(), lod->m_uvDensities.size() * sizeof( float ) );

	cmf::MeshLod& cmfMeshLod = cmfMesh.lods[0];
	cmfMeshLod.vb = bufferAllocator.AddBuffer( (void*)pVertices, vertexBufferSize, vertexBufferStride );
	cmfMeshLod.ib = bufferAllocator.AddBuffer( (void*)pIndices, indexBufferSize, indexBufferStride );
	cmfMeshLod.areas = allocator.AllocateSpan<cmf::LodMeshArea>( lod->m_areas.size() );
	cmfMeshLod.morphTargets = cmf::Span<cmf::LodMorphTarget>();
	cmfMeshLod.threshold = (uint32_t)lod->m_maxScreenSize;

	for( int32_t i = 0; i < cmfMesh.areas.size(); i++ )
	{
		cmf::MeshArea& cmfMeshArea = cmfMesh.areas[i];
		cmfMeshArea.name = allocator.AllocateString( lod->m_areas[i].m_name );
		cmfMeshArea.bounds = AxisAlignedBoundingBox( lod->m_areas[i].m_minBounds, lod->m_areas[i].m_maxBounds );
		cmfMeshArea.bones = allocator.AllocateSpan<uint16_t>( lod->m_areas[i].m_jointBindings.size() );
		cmfMeshArea.affectedByBones = lod->m_areas[i].m_isSkinned;
		cmfMeshArea.affectedByMorphTargets = false;

		for( int32_t j = 0; j < lod->m_areas[i].m_jointBindings.size(); j++ )
		{
			cmfMeshArea.bones[j] = (uint16_t)lod->m_areas[i].m_jointBindings[j];
		}

		cmf::LodMeshArea& lodMeshArea = cmfMeshLod.areas[i];
		lodMeshArea.firstElement = lod->m_areas[i].m_firstIndex / 3;
		lodMeshArea.elementCount = lod->m_areas[i].m_primitiveCount;
	}

	for( int32_t i = 0; i < cmfMesh.boneBindings.size(); i++ )
	{
		cmf::BoneBinding& cmfBoneBinding = cmfMesh.boneBindings[i];
		cmfBoneBinding.name = allocator.AllocateString( mesh.m_jointBindings[i].m_name );
		cmfBoneBinding.bounds = AxisAlignedBoundingBox( mesh.m_jointBindings[i].m_obbMin, mesh.m_jointBindings[i].m_obbMax );
	}

	auto file = cmf::BuildFile( cmfData, bufferAllocator );

	std::wstring filenameW = (const wchar_t*)CA2W( filename );
	std::wstring fullPath = BePaths->ResolvePathForWritingW( filenameW );

	FILE* outFile;
	auto err = fopen_s( &outFile, CW2A( fullPath.c_str() ), "wb" );

	if( err != 0 )
	{
		CCP_LOGERR( "Failed to open output file: %ls\n", fullPath.c_str() );
		return false;
	}
	if( fwrite( file.data(), 1, file.size(), outFile ) != file.size() )
	{
		CCP_LOGERR( "Failed to write output file: %ls\n", fullPath.c_str() );
		fclose( outFile );
		return false;
	}
	fclose( outFile );

	return true;
}

#if WITH_GRANNY
bool Tr2GeometryProvider::SaveMeshToGrannyFile( TriGeometryResMeshData& mesh, const char* filename )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( mesh.m_vertexDeclarationHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return false;
	}

	auto& lod = mesh.m_lods[0];

	if( !lod->m_allocationsValid )
	{
		return false;
	}

	granny_data_type_definition grannyVertexDecl[16];

	Tr2VertexDefinition decl;
	if( !Tr2EffectStateManager::GetVertexDeclarationElements( mesh.m_vertexDeclarationHandle, decl ) )
	{
		return false;
	}

	if( !ConvertVertexDeclToGranny( decl, grannyVertexDecl, sizeof( grannyVertexDecl ) / sizeof( grannyVertexDecl[0] ) ) )
	{
		return false;
	}

	const void* pVertices;
	if( FAILED( lod->m_vertexAllocation.MapForReading( pVertices, renderContext ) ) )
	{
		return false;
	}
	ON_BLOCK_EXIT( [&] { lod->m_vertexAllocation.UnmapForReading( renderContext ); } );

	const void* pIndices;
	if( FAILED( lod->m_indexAllocation.MapForReading( pIndices, renderContext ) ) )
	{
		return false;
	}
	ON_BLOCK_EXIT( [&] { lod->m_indexAllocation.UnmapForReading( renderContext ); } );


	granny_vertex_data myVertexData;
	memset( &myVertexData, 0, sizeof( myVertexData ) );
	myVertexData.Vertices = (granny_uint8*)pVertices;
	myVertexData.VertexCount = lod->m_vertexCount;
	myVertexData.VertexType = grannyVertexDecl;

	granny_tri_topology myTopology;
	memset( &myTopology, 0, sizeof( myTopology ) );

	if( lod->m_vertexCount <= 65535 )
	{
		myTopology.Indices16 = (granny_uint16*)pIndices;
		myTopology.Index16Count = lod->m_primitiveCount * 3;
	}
	else
	{
		myTopology.Indices = (granny_int32*)pIndices;
		myTopology.IndexCount = lod->m_primitiveCount * 3;
	}

	myTopology.GroupCount = (granny_int32)lod->m_areas.size();
	myTopology.Groups = CCP_NEW( "myTopology.Groups" ) granny_tri_material_group[myTopology.GroupCount];
	if( myTopology.Groups )
	{
		for( int i = 0; i < myTopology.GroupCount; ++i )
		{
			myTopology.Groups[i].MaterialIndex = i;
			myTopology.Groups[i].TriFirst = lod->m_areas[i].m_firstIndex / 3;
			myTopology.Groups[i].TriCount = lod->m_areas[i].m_primitiveCount;
		}
	}
	else
	{
		myTopology.GroupCount = 0;
	}

	granny_mesh grannyMesh;
	memset( &grannyMesh, 0, sizeof( grannyMesh ) );

	grannyMesh.PrimaryVertexData = &myVertexData;
	grannyMesh.PrimaryTopology = &myTopology;

	granny_file_info fileInfo;
	memset( &fileInfo, 0, sizeof( fileInfo ) );

	granny_mesh* meshes[] = { &grannyMesh };
	granny_tri_topology* topologies[] = { grannyMesh.PrimaryTopology };
	granny_vertex_data* vertexDatas[] = { grannyMesh.PrimaryVertexData };

	fileInfo.ModelCount = 0;
	fileInfo.Models = 0;
	fileInfo.MeshCount = 1;
	fileInfo.Meshes = meshes;
	fileInfo.VertexDataCount = 1;
	fileInfo.VertexDatas = vertexDatas;
	fileInfo.TriTopologyCount = 1;
	fileInfo.TriTopologies = topologies;

	fileInfo.MaterialCount = (granny_int32)lod->m_areas.size();
	fileInfo.Materials = CCP_NEW( "fileInfo.Materials" ) granny_material * [fileInfo.MaterialCount];
	if( fileInfo.Materials == nullptr )
	{
		fileInfo.MaterialCount = 0;
	}

	grannyMesh.MaterialBindingCount = fileInfo.MaterialCount;
	grannyMesh.MaterialBindings = CCP_NEW( "grannyMesh.MaterialBindings" ) granny_material_binding[fileInfo.MaterialCount];
	if( grannyMesh.MaterialBindings == nullptr )
	{
		grannyMesh.MaterialBindingCount = 0;
	}

	granny_material* pMaterials = nullptr;
	if( fileInfo.MaterialCount )
	{
		pMaterials = CCP_NEW( "pMaterials" ) granny_material[fileInfo.MaterialCount];
		if( pMaterials )
		{
			memset( pMaterials, 0, sizeof( granny_material ) * fileInfo.MaterialCount );

			for( int i = 0; i < fileInfo.MaterialCount; ++i )
			{
				fileInfo.Materials[i] = &pMaterials[i];
				fileInfo.Materials[i]->Name = lod->m_areas[i].m_name.c_str();
				grannyMesh.MaterialBindings[i].Material = fileInfo.Materials[i];
			}
		}
	}

	// Bones
	grannyMesh.BoneBindingCount = (granny_int32)mesh.m_jointBindings.size();
	grannyMesh.BoneBindings = CCP_NEW( "grannyMesh.BoneBindings" ) granny_bone_binding[grannyMesh.BoneBindingCount];
	if( grannyMesh.BoneBindings == nullptr )
	{
		grannyMesh.BoneBindingCount = 0;
	}
	if( grannyMesh.BoneBindingCount > 0 )
	{
		memset( grannyMesh.BoneBindings, 0, sizeof( granny_bone_binding ) * grannyMesh.BoneBindingCount );
		for( int i = 0; i < grannyMesh.BoneBindingCount; ++i )
		{
			grannyMesh.BoneBindings[i].BoneName = mesh.m_jointBindings[i].m_name.c_str();
			grannyMesh.BoneBindings[i].OBBMin[0] = mesh.m_jointBindings[i].m_obbMin.x;
			grannyMesh.BoneBindings[i].OBBMin[1] = mesh.m_jointBindings[i].m_obbMin.y;
			grannyMesh.BoneBindings[i].OBBMin[2] = mesh.m_jointBindings[i].m_obbMin.z;
			grannyMesh.BoneBindings[i].OBBMax[0] = mesh.m_jointBindings[i].m_obbMax.x;
			grannyMesh.BoneBindings[i].OBBMax[1] = mesh.m_jointBindings[i].m_obbMax.y;
			grannyMesh.BoneBindings[i].OBBMax[2] = mesh.m_jointBindings[i].m_obbMax.z;
		}
	}


	granny_file_builder* Builder = GrannyBeginFile( 1, GrannyCurrentGRNStandardTag, GrannyGRNFileMV_32Bit_LittleEndian, GrannyGetTemporaryDirectory(), "GRNFileTemp" );
	granny_file_data_tree_writer* Writer =
		GrannyBeginFileDataTreeWriting( GrannyFileInfoType, &fileInfo, 0, 0 );
	GrannyWriteDataTreeToFileBuilder( Writer, Builder );
	GrannyEndFileDataTreeWriting( Writer );

	std::wstring filenameW = (const wchar_t*)CA2W( filename );
	std::wstring fullPath = BePaths->ResolvePathForWritingW( filenameW );
	GrannyEndFile( Builder, CW2A( fullPath.c_str() ) );

	CCP_DELETE[] pMaterials;
	CCP_DELETE[] grannyMesh.MaterialBindings;
	CCP_DELETE[] grannyMesh.BoneBindings;
	CCP_DELETE[] fileInfo.Materials;

	return true;
}
#endif

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface. Query if the instance data is ready to be
//   used, i.e. if the resource is good.
// Return Value:
//   true if data is ready to be used
//   false otherwise
// --------------------------------------------------------------------------------------
bool Tr2GeometryProvider::IsInstanceDataReady() const
{
	return IsGood();
}

ITr2InstanceData::InstanceData Tr2GeometryProvider::GetInstanceData( unsigned int bufferIndex, float screenSize ) const
{
	auto mesh = GetMeshData( bufferIndex );

	if( !mesh )
	{
		static Tr2BufferAL nullVB;
		return { nullVB };
	}

	auto lod = GetMeshLod( bufferIndex, screenSize );
	return {
		lod->m_vertexAllocation.GetBuffer(), lod->m_vertexAllocation.GetOffset(), mesh->m_bytesPerVertex, lod->m_vertexCount
	};
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface. Returns returns vertex declaration handle
//   for the given instance buffer index.
// Arguments:
//   bufferIndex - instance buffer index
// Return Value:
//   vertex declaration handle
// --------------------------------------------------------------------------------------
unsigned int Tr2GeometryProvider::GetInstanceBufferVertexDeclaration( unsigned int bufferIndex ) const
{
	if( bufferIndex < m_meshes.size() && m_meshes[bufferIndex] )
	{
		return m_meshes[bufferIndex]->m_vertexDeclarationHandle;
	}
	return Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
}

CcpMath::AxisAlignedBox Tr2GeometryProvider::GetInstanceBufferBoundingBox( unsigned int bufferIndex ) const
{
	auto mesh = GetMeshData( bufferIndex );
	if( !mesh )
	{
		return CcpMath::AxisAlignedBox();
	}
	return CcpMath::AxisAlignedBox( mesh->m_minBounds, mesh->m_maxBounds );
}

Be::Result<std::string> Tr2GeometryProvider::GetMeshName( unsigned int meshIx, std::string& name ) const
{
	TriGeometryResMeshData* mesh = GetMeshData( meshIx );
	if( !mesh )
	{
		return Be::Result<std::string>( "Mesh index out of range, or invalid mesh" );
	}

	name = mesh->m_name;

	return Be::Result<std::string>();
}

Be::Result<std::string> Tr2GeometryProvider::GetMeshAreaCount( unsigned int meshIx, int& count ) const
{
	TriGeometryResMeshData* mesh = GetMeshData( meshIx );
	if( !mesh )
	{
		return Be::Result<std::string>( "Mesh index out of range, or invalid mesh" );
	}

	//Get the first LOD.
	auto& lod = mesh->m_lods[0];

	count = (int)lod->m_areas.size();

	return Be::Result<std::string>();
}

Be::Result<std::string> Tr2GeometryProvider::GetMeshAreaName( unsigned int meshIx, unsigned int areaIx, std::string& name ) const
{
	TriGeometryResMeshData* mesh = GetMeshData( meshIx );
	if( !mesh )
	{
		return Be::Result<std::string>( "Mesh index out of range, or invalid mesh" );
	}

	//Get the first LOD.
	auto& lod = mesh->m_lods[0];

	if( areaIx < 0 || areaIx >= lod->m_areas.size() )
	{
		return Be::Result<std::string>( "Area index out of range" );
	}

	name = lod->m_areas[areaIx].m_name;

	return Be::Result<std::string>();
}

Be::Result<std::string> Tr2GeometryProvider::GetAreaBoundingBoxFromScript( unsigned int meshIx, unsigned int areaIx, std::pair<Vector3, Vector3>& bounds )
{
	TriGeometryResMeshData* mesh = GetMeshData( meshIx );
	if( !mesh )
	{
		return Be::Result<std::string>( "Mesh index out of range, or invalid mesh" );
	}

	//Get the first LOD.
	auto& lod = mesh->m_lods[0];

	if( areaIx < 0 || areaIx >= lod->m_areas.size() )
	{
		return Be::Result<std::string>( "Area index out of range" );
	}

	bounds = std::make_pair( lod->m_areas[areaIx].m_minBounds, lod->m_areas[areaIx].m_maxBounds );

	return Be::Result<std::string>();
}

Be::Result<std::string> Tr2GeometryProvider::GetBoundingSphereFromScript( unsigned int meshIx, std::pair<Vector3, float>& bounds ) const
{
	if( meshIx >= m_meshes.size() )
	{
		return Be::Result<std::string>( "Mesh index out of range" );
	}
	auto& mesh = m_meshes[meshIx];
	if( !mesh )
	{
		return Be::Result<std::string>( "Invalid mesh" );
	}

	bounds = std::make_pair( Vector3( mesh->m_boundingSphere.x, mesh->m_boundingSphere.y, mesh->m_boundingSphere.z ), mesh->m_boundingSphere.w );
	return Be::Result<std::string>();
}

Be::Result<std::string> Tr2GeometryProvider::CalculateBoundingBoxFromTransform( unsigned int meshIx, const Matrix& transform, std::pair<Vector3, Vector3>& bounds )
{
	if( meshIx >= m_meshes.size() )
	{
		return Be::Result<std::string>( "Mesh index out of range" );
	}
	auto& mesh = m_meshes[meshIx];
	if( !mesh )
	{
		return Be::Result<std::string>( "Invalid mesh" );
	}

	CalcBoundingBoxContext ctx;
	ctx.min = Vector3( FLT_MAX, FLT_MAX, FLT_MAX );
	ctx.max = Vector3( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	ctx.transform = transform;

	if( meshIx < m_meshes.size() )
	{
		ProcessMeshTriangles( meshIx, &CalcBoundingBox, &ctx );
	}

	bounds = std::make_pair( ctx.min, ctx.max );
	return Be::Result<std::string>();
}


BlueStdResult Tr2GeometryProvider::GetMeshVertexElements( size_t meshIndex, std::vector<std::pair<uint32_t, uint32_t>>& elements ) const
{
	elements.clear();

	if( !IsGood() )
	{
		return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "TriGeometryRes is not prepared" );
	}
	if( meshIndex >= m_meshes.size() || !m_meshes[meshIndex] )
	{
		return BlueStdResult( BLUE_STD_RESULT_INDEX_ERROR, "mesh index out of range" );
	}

	auto& mesh = m_meshes[meshIndex];
	Tr2VertexDefinition decl;
	if( !Tr2EffectStateManager::GetVertexDeclarationElements( mesh->m_vertexDeclarationHandle, decl ) )
	{
		return BlueStdResult( BLUE_STD_RESULT_RUNTIME_ERROR, "could not retrieve vertex declaration" );
	}

	for( auto it = std::begin( decl.m_items ); it != std::end( decl.m_items ); ++it )
	{
		elements.push_back( std::make_pair( it->m_usage, it->m_usageIndex ) );
	}
	return BLUE_STD_RESULT_OK;
}

bool Tr2GeometryProvider::SetupMeshes( const cmf::Data& cmfData )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	int minimumLOD = gTriDev->GetMinimumModelLOD();

	for( const cmf::Mesh& cmfMesh : cmfData.meshes )
	{
		Tr2VertexDefinition vertexDefinition = BuildFromCMFVertexDecl( cmfMesh.decl );
		const uint32_t vertexDeclarationHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( vertexDefinition );
		unsigned int bytesPerVertex = cmfMesh.lods[0].vb.stride;

		TriGeometryResMeshData* mesh = new TriGeometryResMeshData();

		mesh->m_name = cmf::ToStdString( cmfMesh.name );

		mesh->m_minBounds = cmfMesh.bounds.m_min;
		mesh->m_maxBounds = cmfMesh.bounds.m_max;

		CcpMath::Sphere sphere( cmfMesh.bounds );
		mesh->m_boundingSphere = Vector4( sphere.center, sphere.radius );

		mesh->m_vertexDeclarationHandle = vertexDeclarationHandle;
		mesh->m_bytesPerVertex = bytesPerVertex;

		for( const auto& joint : cmfMesh.boneBindings )
		{
			TriJointBinding binding;
			binding.m_name = cmf::ToStdString( joint.name );
			binding.m_obbMin = joint.bounds.m_min;
			binding.m_obbMax = joint.bounds.m_max;
			mesh->m_jointBindings.push_back( binding );
		}

		int32_t firstLodIndex = ClampInt( minimumLOD, 0, int32_t( cmfMesh.lods.size() ) - 1 );

		for( int32_t i = firstLodIndex; i < cmfMesh.lods.size(); i++ )
		{
			const cmf::MeshLod& cmfLod = cmfMesh.lods[i];

			//Setup a new LOD, which will be added as an LOD of the mesh.
			std::unique_ptr<TriGeometryResLodData> lod( new TriGeometryResLodData() );

			lod->m_mesh = mesh;

			// TODO: intern, delete field once granny has been removed.
			lod->m_grannyMeshIndex = 0xFFFFFFFF;

			lod->m_name = cmf::ToStdString( cmfMesh.name );

			lod->m_maxScreenSize = float( cmfLod.threshold );

			lod->m_vertexCount = cmf::GetStreamElementCount( cmfLod.vb );

			lod->m_primitiveCount = cmf::GetStreamElementCount( cmfLod.ib ) / 3;

			// Get the UV densities, which are used for texture streaming.
			lod->m_uvDensities = std::vector<float>( cmfMesh.uvDensities.begin(), cmfMesh.uvDensities.end() );

			lod->m_areas.resize( cmfLod.areas.size() );

			for( int groupIndex = 0; groupIndex < cmfLod.areas.size(); ++groupIndex )
			{
				const cmf::LodMeshArea& cmfLodMeshArea = cmfLod.areas[groupIndex];
				const cmf::MeshArea& cmfMeshArea = cmfMesh.areas[groupIndex];

				TriGeometryResAreaData& area = lod->m_areas[groupIndex];
				area.m_name = cmf::ToStdString( cmfMeshArea.name );

				area.m_firstIndex = cmfLodMeshArea.firstElement * 3;
				area.m_primitiveCount = cmfLodMeshArea.elementCount;

				area.m_isSkinned = cmfMeshArea.affectedByBones;
				area.m_isMorphed = cmfMeshArea.affectedByMorphTargets;

				area.m_minBounds = cmfMeshArea.bounds.m_min;
				area.m_maxBounds = cmfMeshArea.bounds.m_max;
			}

			lod->m_originalLodIndex = i;

			mesh->m_lods.push_back( std::move( lod ) );
		}

		uint32_t lodMask = 0u;
		for( const auto& lod : mesh->m_lods )
		{
			lodMask |= 1u << lod->m_originalLodIndex;
		}
		mesh->m_lodMask = lodMask;

		m_meshes.push_back( std::unique_ptr<TriGeometryResMeshData>( mesh ) );
	}

	return true;
}

void Tr2GeometryProvider::SetupSkeletons( const cmf::Data& cmfData )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	m_skeletons.resize( cmfData.skeletons.size() );
	for( int skelIx = 0; skelIx < cmfData.skeletons.size(); ++skelIx )
	{
		const cmf::Skeleton& skel = cmfData.skeletons[skelIx];
		std::unique_ptr<TriGeometryResSkeletonData> pSkelData( new TriGeometryResSkeletonData() );

		pSkelData->m_name = cmf::ToStdString( skel.name );
		pSkelData->m_joints.resize( skel.bones.size() );
		for( int jointIx = 0; jointIx < skel.bones.size(); ++jointIx )
		{
			TriGeometryResJointData& joint = pSkelData->m_joints[jointIx];
			joint.m_name = cmf::ToStdString( skel.bones[jointIx] );
			joint.m_parentJoint = skel.parents[jointIx];
			joint.m_inverseWorldTransform = *(Matrix*)&skel.invBindTransforms[jointIx];
		}

		std::swap( m_skeletons[skelIx], pSkelData );
	}
}

bool Tr2GeometryProvider::CreateMeshesFromCMFFile( Tr2CmfContents& cmfContents, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContext& renderContext )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	if( !cmfContents.GetData() )
	{
		return false;
	}

	for( int32_t i = 0; i < m_meshes.size(); i++ )
	{
		auto& mesh = m_meshes[i];

		for( const auto& lod : mesh->m_lods )
		{
			CreateLodFromCMFMesh( cmfContents, cmfContents.GetData()->meshes[i], cmfContents.GetData()->meshes[i].lods[lod->m_originalLodIndex], lod.get(), cpuUsage, renderContext );
		}

		if( g_eveIsAudioOcclusionGeometryEnabled )
		{
			const auto& audioMesh = cmfContents.GetData()->meshes[i].audioOcclusionMesh;
			if( !audioMesh.vertices.empty() && !audioMesh.indices.empty() )
			{
				auto audioGeometry = std::make_unique<AudioGeometryResData>();
				audioGeometry->m_indices.insert( audioGeometry->m_indices.end(), audioMesh.indices.begin(), audioMesh.indices.end() );
				audioGeometry->m_vertices.insert( audioGeometry->m_vertices.end(), audioMesh.vertices.begin(), audioMesh.vertices.end() );
				audioGeometry->m_minBounds = audioMesh.bounds.m_min;
				audioGeometry->m_maxBounds = audioMesh.bounds.m_max;

				mesh->m_audioGeometry = std::move( audioGeometry );
			}
		}
	}

	CCP_STATS_ADD( geometryResBytes, m_memoryUse );

	return true;
}

bool Tr2GeometryProvider::CreateLodFromCMFMesh( Tr2CmfContents& cmfContents, const cmf::Mesh& cmfMesh, const cmf::MeshLod& cmfMeshLod, TriGeometryResLodData* lod, Tr2CpuUsage::Type cpuUsage, Tr2PrimaryRenderContext& renderContext )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return false;
	}

	Tr2VertexDefinition vertexDefinition = BuildFromCMFVertexDecl( cmfMesh.decl );
	const unsigned bytesPerVertex = cmfMeshLod.vb.stride;
	const unsigned bytesPerIndex = cmfMeshLod.ib.stride;

	int vertexCount = cmf::GetStreamElementCount( cmfMeshLod.vb );
	int vbSize = cmfMeshLod.vb.size;

	CCP_ASSERT_M( cmfContents, "TriGeometryRes::CreateLodFromCMFMesh: cmfContents must be valid!" );

	const void* pSrcVB = cmfContents.GetViewData( cmfMeshLod.vb );

	int indexCount = cmf::GetStreamElementCount( cmfMeshLod.ib );

	CR_RETURN_VAL( g_sharedBuffer.Allocate( bytesPerVertex, vertexCount, const_cast<void*>( pSrcVB ), renderContext, lod->m_vertexAllocation ), false );

	// create d3d index buffer, this one is shared, either for dynamic or static geometry
	int ibSize = indexCount * bytesPerIndex;

	const void* pSrcIB = cmfContents.GetViewData( cmfMeshLod.ib );

	if( indexCount > 0 )
	{
		ALResult hr = g_sharedBuffer.Allocate( bytesPerIndex, indexCount, const_cast<void*>( pSrcIB ), renderContext, lod->m_indexAllocation );
		if( FAILED( hr ) )
		{
			g_sharedBuffer.Free( lod->m_vertexAllocation );
			return false;
		}

		std::vector<uint8_t> tempBuffer( indexCount * bytesPerIndex );
		memcpy( tempBuffer.data(), pSrcIB, tempBuffer.size() );
		if( bytesPerIndex == 2 )
		{
			std::reverse( reinterpret_cast<uint16_t*>( tempBuffer.data() ), reinterpret_cast<uint16_t*>( tempBuffer.data() + cmfMeshLod.ib.size ) );
		}
		else
		{
			std::reverse( reinterpret_cast<uint32_t*>( tempBuffer.data() ), reinterpret_cast<uint32_t*>( tempBuffer.data() + cmfMeshLod.ib.size ) );
		}

		if( FAILED( g_sharedBuffer.Allocate( bytesPerIndex, indexCount, tempBuffer.data(), renderContext, lod->m_reversedIndexAllocation ) ) )
		{
			g_sharedBuffer.Free( lod->m_vertexAllocation );
			g_sharedBuffer.Free( lod->m_indexAllocation );
			return false;
		}
		lod->m_reversedIndicesValid = true;
	}

	lod->m_morphVertexDeclaration = -1;
	{ // Morph Target
		if( cmfMesh.morphTargets.targets.size() > 0 )
		{
			// Allocate the morph target array based on the size of the first morph target
			Tr2VertexDefinition morphTargetVertexDefinition = BuildFromCMFVertexDecl( cmfMesh.morphTargets.decl );
			lod->m_bytesPerMorphTargetVertex = cmfMeshLod.morphTargets[0].vb.stride;
			uint32_t morphDataSize = vertexCount * lod->m_bytesPerMorphTargetVertex;
			uint32_t morphTargetBufferSize = (uint32_t)( morphDataSize * cmfMesh.morphTargets.targets.size() + sizeof( TriMorphTargetGeometryConstants ) );

			CR_RETURN_VAL( g_sharedBuffer.Allocate(
							   4,
							   morphTargetBufferSize >> 2,
							   nullptr,
							   renderContext,
							   lod->m_morphTargetAllocation ),
						   false );

			TriMorphTargetGeometryConstants morphTargetConstants = CreateMorphGeometryConstants(
				morphTargetVertexDefinition,
				lod->m_bytesPerMorphTargetVertex,
				vertexCount,
				renderContext );

			lod->m_morphTargetAllocation.Update( &morphTargetConstants, 0, sizeof( TriMorphTargetGeometryConstants ), renderContext );

			for( int i = 0; i < cmfMesh.morphTargets.targets.size(); ++i )
			{
				const cmf::MorphTarget& morphTarget = cmfMesh.morphTargets.targets[i];

				size_t nameLength = morphTarget.name.size();
				// TODO: intern, this "Shape" suffix will likely be gone later
				// By convention (due to the exporter), the morph target name ends with "Shape". Assert that it does, and also that it is not an empty string!
				std::string morphTargetName = cmf::ToStdString( morphTarget.name );
				if( nameLength > 5 && strcmp( morphTargetName.c_str() + nameLength - 5, "Shape" ) == 0 )
				{
					morphTargetName = std::string( morphTarget.name.data(), nameLength - 5 );
				}

				lod->m_morphTargetNames.push_back( morphTargetName );

				lod->m_isBakedMorphTarget.push_back( false );

				const void* pMorphSrc = cmfContents.GetViewData( cmfMeshLod.morphTargets[i].vb );
				lod->m_morphTargetAllocation.Update(
					const_cast<void*>( pMorphSrc ),
					morphDataSize * i + sizeof( TriMorphTargetGeometryConstants ),
					morphDataSize,
					renderContext );

				lod->m_morphTargetDeformationAmounts.push_back( morphTarget.maxDisplacement );
			}

			lod->m_morphVertexDeclaration = Tr2EffectStateManager::GetVertexDeclarationHandle( morphTargetVertexDefinition );
		}
	}

	lod->m_allocationsValid = true;

	m_memoryUse += vbSize + ibSize; // Memory use is only approximate as a hint for the resource cache

	// Append reversed index buffer size also
	m_memoryUse += ibSize;

	return true;
}
