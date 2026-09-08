// Copyright © 2011 CCP ehf.

#include "StdAfx.h"
#include "Tr2InstancedMesh.h"
#include "Resources/TriGeometryRes.h"
#include "Tr2Renderer.h"
#include "Utilities/BoundingBox.h"


CCP_STATS_DECLARED_ELSEWHERE( primitiveCount );
CCP_STATS_DECLARED_ELSEWHERE( vertexCount );
CCP_STATS_DECLARED_ELSEWHERE( sceneDrawcallCount );

CCP_STATS_DECLARE( instancesRendered, "Trinity/instancesRendered", true, CST_COUNTER_HIGH, "Number of instances rendered with Tr2InstancedMesh" );

#if CCP_STATS_ENABLED
#define CCP_STATS_INC_PTR( name ) g_ccpStatistics_##name->Inc()
// Allow external code (such as shadow cubemap) to change the counter used for counting draw calls,
// so we get more refined results than just one scene-wide "you had this many drawcalls in the _entire_ frame".
extern CcpStaticStatisticsEntry* g_ccpStatistics_drawcallCount;
#else
#define CCP_STATS_INC_PTR( name )
#endif

extern bool g_brokenMacOSNvidiaDrivers;

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InstancedMesh default constructor
// --------------------------------------------------------------------------------------
Tr2InstancedMesh::Tr2InstancedMesh( IRoot* lockobj ) :
	Tr2Mesh( lockobj ),
	m_instanceMeshIndex( 0 ),
	m_vertexDeclaration( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_instanceDeclaration( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_boundsMethod( STATIC ),
	m_minBounds( 0.0f, 0.0f, 0.0f ),
	m_maxBounds( 0.0f, 0.0f, 0.0f ),
	m_maxInstanceSize( 0 )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InstancedMesh destructor.
// --------------------------------------------------------------------------------------
Tr2InstancedMesh::~Tr2InstancedMesh()
{
	SetInstanceGeometryRes( nullptr );
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IInitialize interface. Starts loading instance geometry resource if
//   mesh has non-empty path and loading is not deferred. Also initializes Tr2Mesh.
// Return Value:
//   true If initialization is successfull
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2InstancedMesh::Initialize()
{
	if( !m_deferGeometryLoad )
	{
		if( !m_instanceGeometryResPath.empty() )
		{
			TriGeometryResPtr res;
			BeResMan->GetResource( m_instanceGeometryResPath.c_str(), "", res );
			m_loadedGeometryResource = res;
		}
	}
	return Tr2Mesh::Initialize();
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriDeviceResource interface. Deinitializes combined vertex declarations.
// --------------------------------------------------------------------------------------
void Tr2InstancedMesh::ReleaseResources( TriStorage s )
{
	m_vertexDeclaration = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_instanceDeclaration = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriDeviceResource interface. Initializes combined vertex declarations if
//   geometry is ready.
// --------------------------------------------------------------------------------------
bool Tr2InstancedMesh::OnPrepareResources()
{
	CreateVertexDeclaration();
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements INotify interface.  Allows the mesh to respond to parameter
//   changes generated in Python.  Loads instance geometry resource when geometry path
//   is modified or when deferred flag is reset. Passes control to Tr2Mesh.
// Arguments:
//   value - The Blue-exposed parameter that changed
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2InstancedMesh::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_instanceGeometryResPath ) )
	{
		if( !m_deferGeometryLoad )
		{
			if( !m_instanceGeometryResPath.empty() )
			{
				TriGeometryResPtr res;
				BeResMan->GetResource( m_instanceGeometryResPath.c_str(), "", res );
				SetInstanceGeometryRes( res );
			}
			else
			{
				SetInstanceGeometryRes( nullptr );
			}
		}
	}
	else if( IsMatch( value, m_deferGeometryLoad ) )
	{
		if( !m_deferGeometryLoad && !m_loadedGeometryResource )
		{
			Initialize();
		}
	}
	else if( IsMatch( value, m_instanceMeshIndex ) || IsMatch( value, m_meshIndex ) )
	{
		CreateVertexDeclaration();
	}

	return Tr2Mesh::OnModified( value );
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IAsyncLoadedResNotifyTarget interface. Resets combined vertex
//   declarations.
// Arguments:
//   p - Resource that being unloaded
// --------------------------------------------------------------------------------------
void Tr2InstancedMesh::ReleaseCachedData( BlueAsyncRes* p )
{
	CreateVertexDeclaration();
	Tr2Mesh::ReleaseCachedData( p );
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IAsyncLoadedResNotifyTarget interface. Creates combined vertex
//   declarations when geometry is loaded.
// Arguments:
//   p - Resource that being loaded
// --------------------------------------------------------------------------------------
void Tr2InstancedMesh::RebuildCachedData( BlueAsyncRes* p )
{
	CreateVertexDeclaration();
	Tr2Mesh::RebuildCachedData( p );
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns an instance resource (either loaded from gr2 or assigned).
// --------------------------------------------------------------------------------------
ITr2InstanceData* Tr2InstancedMesh::GetInstanceGeometryResource() const
{
	return m_loadedGeometryResource ? m_loadedGeometryResource : m_instanceGeometryResource;
}

// --------------------------------------------------------------------------------------
// Description:
//   Set a new instance geometry path from the outside. This will trigger an initialize
//   of the new geometry resource!
// Arguments:
//   path - gr2 res path
// --------------------------------------------------------------------------------------
void Tr2InstancedMesh::SetInstanceMeshResPath( const char* path )
{
	m_instanceGeometryResPath = path;

	// trigger change, this will automatically be triggered when set through python
	OnModified( (Be::Var*)&m_instanceGeometryResPath );
}

// --------------------------------------------------------------------------------------
// Description:
//   Changes instance geometry resource.
// Arguments:
//   res - New instanced geometry resource
// --------------------------------------------------------------------------------------
void Tr2InstancedMesh::SetInstanceGeometryRes( ITr2InstanceData* res )
{
	if( m_instanceGeometryResource == res )
	{
		return;
	}
	m_instanceGeometryResource = res;
	m_loadedGeometryResource = nullptr;
	CreateVertexDeclaration();
}

// --------------------------------------------------------------------------------------
// Description:
//   Collects render batches from this instanced mesh.
// Arguments:
//   batches - Batch accumulator to add batches to
//   areas - Vector of mesh areas for which batches need to be added
//   data - Per-object data
// --------------------------------------------------------------------------------------
void Tr2InstancedMesh::GetBatches( ITriRenderBatchAccumulator* batches,
								   const Tr2MeshAreaVector* areas,
								   const Tr2PerObjectData* data,
								   float screenSize,
								   bool reverseAreas ) const
{
	if( !GetDisplay() || g_brokenMacOSNvidiaDrivers )
	{
		return;
	}

	auto geometryResource = GetGeometryResource();
	if( !geometryResource || !geometryResource->IsGood() )
	{
		return;
	}

	auto instanceGeometryResource = GetInstanceGeometryResource();
	if( !instanceGeometryResource || !instanceGeometryResource->IsInstanceDataReady() )
	{
		return;
	}

	bool rebuild = m_vertexDeclaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION || m_instanceDeclaration != instanceGeometryResource->GetInstanceBufferVertexDeclaration( m_instanceMeshIndex );
	if( rebuild )
	{
		CreateVertexDeclaration();
		if( m_vertexDeclaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			return;
		}
	}

	auto lod = geometryResource->GetMeshLod( m_meshIndex, screenSize );
	if( !lod || !lod->m_allocationsValid )
	{
		return;
	}

	auto instanceData = instanceGeometryResource->GetInstanceData( m_instanceMeshIndex, screenSize );
	if( instanceData.count == 0 )
	{
		return;
	}

	for( auto it = areas->begin(); it != areas->end(); ++it )
	{
		Tr2MeshArea* area = *it;

		if( !area->GetDisplay() )
		{
			continue;
		}

		auto shadMat = area->GetMaterialInterface();
		if( !shadMat )
		{
			continue;
		}

		auto primCount = GetPrimitiveCount( *lod, std::max( 0, area->GetIndex() ), std::max( 0, area->GetCount() ) );
		if( !primCount )
		{
			continue;
		}
		auto reversed = area->GetReversed();
		if( reverseAreas )
		{
			reversed = !reversed;
		}
		if( reversed && !lod->m_reversedIndicesValid )
		{
			continue;
		}
		auto& lodArea = lod->m_areas[area->GetIndex()];

		Tr2RenderBatch batch;
		batch.SetMaterial( shadMat );
		batch.SetPerObjectData( data );
		batch.SetGeometry( m_vertexDeclaration, lod->m_vertexAllocation, lod->m_indexAllocation );
		batch.SetStreamSource( 1, instanceData.buffer, instanceData.stride );

		auto& indices = reversed ? lod->m_reversedIndexAllocation : lod->m_indexAllocation;
		uint32_t startIndex;
		if( reversed )
		{
			startIndex = indices.GetStartIndex() + lod->m_primitiveCount * 3 - lodArea.m_firstIndex - primCount * 3;
		}
		else
		{
			startIndex = indices.GetStartIndex() + lodArea.m_firstIndex;
		}

		batch.SetDrawIndexedInstanced(
			primCount * 3,
			instanceData.count,
			startIndex,
			lod->m_vertexAllocation.GetOffset() / lod->m_vertexAllocation.GetStride(),
			instanceData.offset / instanceData.stride );

		batches->Commit( batch );
	}
}

CcpMath::AxisAlignedBox Tr2InstancedMesh::GetBounds( const Matrix* boneTransforms, const int32_t* meshBindingIndices, size_t boneCount, const Tr2MorphTargetAnimationData* morphTargets, size_t morphTargetsCount ) const
{
	CcpMath::AxisAlignedBox aabb;
	if( m_boundsMethod == STATIC )
	{
		aabb.m_min = m_minBounds;
		aabb.m_max = m_maxBounds;
		return aabb;
	}
	else
	{
		auto instanceGeometryResource = GetInstanceGeometryResource();
		if( !instanceGeometryResource )
		{
			return aabb;
		}
		aabb = instanceGeometryResource->GetInstanceBufferBoundingBox( m_instanceMeshIndex );
		if( !aabb )
		{
			return aabb;
		}

		auto instanceSize = m_maxInstanceSize;
		if( m_boundsMethod == DYNAMIC_SCALED )
		{
			CcpMath::AxisAlignedBox instance = GetInstanceBounds();
			if( instance )
			{
				float radius = 0;
				instance.EnumerateVertices( [&radius]( const Vector3& vtx ) {
					radius = std::max( radius, LengthSq( vtx ) );
				} );
				instanceSize *= sqrt( radius );
			}
		}
		aabb.Grow( instanceSize );
		return aabb;
	}
}

CcpMath::AxisAlignedBox Tr2InstancedMesh::GetInstanceBounds() const
{
	auto geometryResource = GetGeometryResource();
	CcpMath::AxisAlignedBox instance;
	if( geometryResource && geometryResource->GetBoundingBox( m_meshIndex, instance.m_min, instance.m_max ) )
	{
		return instance;
	}
	return CcpMath::AxisAlignedBox();
}


CcpMath::Sphere Tr2InstancedMesh::GetInstanceBoundsClosestToPoint( const Vector3& point ) const
{
	float instanceSize = m_maxInstanceSize;
	switch( m_boundsMethod )
	{
	case Tr2InstancedMesh::DYNAMIC:
		break;
	case Tr2InstancedMesh::DYNAMIC_SCALED: {
		float radius = 0;
		GetInstanceBounds().EnumerateVertices( [&radius]( const Vector3& vtx ) {
			radius = std::max( radius, LengthSq( vtx ) );
		} );
		instanceSize *= sqrt( radius );
		break;
	}
	default:
		return {};
	}
	auto outerBounds = GetBounds();
	outerBounds.Grow( -instanceSize );

	Vector3 closestPoint = ClosestPointToBoundingBox( outerBounds.m_min, outerBounds.m_max, point );

	return CcpMath::Sphere( closestPoint, instanceSize );
}

CcpMath::AxisAlignedBox Tr2InstancedMesh::GetAreaBounds( unsigned int, const Matrix* boneTransforms ) const
{
	return GetBounds( boneTransforms );
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the bounding box for this mesh.
// Arguments:
//   min - Min bounds in local space
//   max - Max bounds in local space
// --------------------------------------------------------------------------------------
void Tr2InstancedMesh::SetBoundingBox( const Vector3& min, const Vector3& max )
{
	m_minBounds = min;
	m_maxBounds = max;
}

// --------------------------------------------------------------------------------------
// Description:
//   Overrides Tr2Mesh method. Reports if the mesh geometry is still loading.
// Return value:
//   true If geometry or instance geometry is still loading
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2InstancedMesh::IsLoading() const
{
	return Tr2Mesh::IsLoading() && GetInstanceGeometryResource() && !GetInstanceGeometryResource()->IsInstanceDataReady();
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns vertex declaration elements for a given mesh in geometry resource.
// Arguments:
//   geometryRes - Geometry resource
//   meshIndex - Mesh index in geometry resource
//   elements - (out) Resulting array of vertex declaration elements; caller must allocate
//              enough space for it
//   count - (out) Number of elements in returned vertex declaration array
// Return value:
//   true On success
//   false Otherwise
// --------------------------------------------------------------------------------------
static bool GetMeshVertexDeclaration( TriGeometryRes* geometryRes, int meshIndex, Tr2VertexDefinition& vd )
{
	if( geometryRes == nullptr || !geometryRes->IsGood() )
	{
		return false;
	}
	TriGeometryResMeshData* meshData = geometryRes->GetMeshData( meshIndex );
	if( meshData == nullptr )
	{
		return false;
	}
	return Tr2EffectStateManager::GetVertexDeclarationElements( meshData->m_vertexDeclarationHandle, vd );
}

// --------------------------------------------------------------------------------------
// Description:
//   Combines two vertex declarations into a single multi-stream declaration. Usage indexes
//   for instanced declaration are shifted by 8, so shader would expect POSITION8, etc.
//   for instance data.
// Arguments:
//   meshVD - Vertex definition of the base mesh
//   instanceVD - Vertex definition of the instance data
// Return value:
//   Handle for combined vertex definition (renderContext version) or Tr2EffectStateManager::
//   UNINITIALIZED_DECLARATION on failure
// --------------------------------------------------------------------------------------
static unsigned int MergeVertexDeclarations( const Tr2VertexDefinition& meshVD,
											 const Tr2VertexDefinition& instanceVD )
{
	const unsigned usageOffset = 8;

	if( meshVD.m_items.empty() || instanceVD.m_items.empty() )
	{
		return Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	}

	Tr2VertexDefinition merged;
	merged = meshVD;
	merged.m_items.insert( merged.m_items.end(), instanceVD.m_items.begin(), instanceVD.m_items.end() );

	for( size_t i = 0; i != instanceVD.m_items.size(); ++i )
	{
		merged.m_items[i + meshVD.m_items.size()].m_stream = 1;
		merged.m_items[i + meshVD.m_items.size()].m_instanceStepRate = 1;
		merged.m_items[i + meshVD.m_items.size()].m_usageIndex += usageOffset;
	}

	return Tr2EffectStateManager::GetVertexDeclarationHandle( merged );
}

// --------------------------------------------------------------------------------------
// Description:
//   Creates a combined vertex declaration for base mesh and instance data.
// --------------------------------------------------------------------------------------
void Tr2InstancedMesh::CreateVertexDeclaration() const
{
	m_vertexDeclaration = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_instanceDeclaration = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;

	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return;
	}

	if( !GetInstanceGeometryResource() || !GetInstanceGeometryResource()->IsInstanceDataReady() )
	{
		return;
	}

	const unsigned declaration = GetInstanceGeometryResource()->GetInstanceBufferVertexDeclaration( m_instanceMeshIndex );
	m_instanceDeclaration = declaration;

	if( declaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return;
	}
	Tr2VertexDefinition instanceVD;
	if( !Tr2EffectStateManager::GetVertexDeclarationElements( declaration, instanceVD ) )
	{
		return;
	}
	Tr2VertexDefinition meshVD;
	if( GetMeshVertexDeclaration( GetGeometryResource(), m_meshIndex, meshVD ) )
	{
		m_vertexDeclaration = MergeVertexDeclarations( meshVD, instanceVD );
	}
}

unsigned int Tr2InstancedMesh::GetVertexDeclaration() const
{
	return m_vertexDeclaration;
}

void Tr2InstancedMesh::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Instance Mesh Bounds" );
}

void Tr2InstancedMesh::RenderDebugInfo( const Matrix& worldTransform, ITr2DebugRenderer2& renderer, const Matrix* boneTransforms, const int32_t* meshBindingIndices, size_t boneCount, const Tr2MorphTargetAnimationData* morphTargets, size_t morphTargetsCount )
{
	if( renderer.HasOption( this, "Instance Mesh Bounds" ) )
	{
		if( m_boundsMethod == STATIC )
		{
			renderer.DrawBox( this, worldTransform, m_minBounds, m_maxBounds, Tr2DebugRenderer::Wireframe, Tr2DebugColor( 0xffaa8800, 0x22aa8800 ) );
		}
		else
		{
			auto parentBounds = GetBounds();
			if( parentBounds )
			{
				if( m_instanceGeometryResource )
				{
					auto sphere = GetInstanceBoundsClosestToPoint( TransformCoord( Tr2Renderer::GetViewPosition(), Inverse( worldTransform ) ) );
					if( sphere && !parentBounds.IsPointInside( Tr2Renderer::GetViewPosition() ) )
					{
						auto aabb = CcpMath::AxisAlignedBox( sphere );
						renderer.DrawBox( this, worldTransform, aabb.m_min, aabb.m_max, Tr2DebugRenderer::Wireframe, Tr2DebugColor( 0xff008888, 0x22008888 ) );
					}
				}
				renderer.DrawBox( this, worldTransform, parentBounds.m_min, parentBounds.m_max, Tr2DebugRenderer::Wireframe, Tr2DebugColor( 0xff888888, 0x22888888 ) );
			}
		}
	}
}

void Tr2InstancedMesh::SetDynamicBounds( float maxInstanceSize )
{
	m_boundsMethod = DYNAMIC;
	m_maxInstanceSize = maxInstanceSize;
}

void Tr2InstancedMesh::SetDynamicScaledBounds( float maxScale )
{
	m_boundsMethod = DYNAMIC_SCALED;
	m_maxInstanceSize = maxScale;
}