////////////////////////////////////////////////////////////
//
//    Created:   November 2011
//    Copyright: CCP 2011
//

#include "StdAfx.h"
#include "Tr2InstancedMesh.h"
#include "Resources/TriGeometryRes.h"
#include "Tr2Renderer.h"


CCP_STATS_DECLARED_ELSEWHERE( primitiveCount );
CCP_STATS_DECLARED_ELSEWHERE( vertexCount );
CCP_STATS_DECLARED_ELSEWHERE( sceneDrawcallCount );

CCP_STATS_DECLARE( instancesRendered, "Trinity/instancesRendered", true, CST_COUNTER_HIGH, "Number of instances rendered with Tr2InstancedMesh" );

#if CCP_STATS_ENABLED
#	define CCP_STATS_INC_PTR( name ) g_ccpStatistics_##name->Inc()
// Allow external code (such as shadow cubemap) to change the counter used for counting draw calls,
// so we get more refined results than just one scene-wide "you had this many drawcalls in the _entire_ frame".
extern CcpStaticStatisticsEntry * g_ccpStatistics_drawcallCount;
#else
#define CCP_STATS_INC_PTR( name )
#endif

// --------------------------------------------------------------------------------------
// Description:
//   Render batch for Tr2InstancedMesh.
// See Also:
//   Tr2InstancedMesh, TriRenderBatch
// --------------------------------------------------------------------------------------
class Tr2InstancedMesh::Batch: public TriRenderBatch
{
public:
	Batch()
		:m_mesh( nullptr ),
		m_areaIndex( 0 ),
		m_areaCount( 0 ),
		m_reversed( false )
	{
	}

	// --------------------------------------------------------------------------------------
	// Description:
	//   Assigns Tr2InstancedMesh to the batch.
	// Arguments:
	//   mesh - Instanced mesh that will be rendered with this batch
	// --------------------------------------------------------------------------------------
	void SetMesh( Tr2InstancedMesh* mesh )
	{
		m_mesh = mesh;
	}

	// --------------------------------------------------------------------------------------
	// Description:
	//   Assigns area data to the batch.
	// Arguments:
	//   areaIx - Area start index
	//   areaCount - Number of geometry areas to render
	//   reversed - If reversed index buffer is required
	// --------------------------------------------------------------------------------------
	void SetMeshParameters( unsigned int areaIx, 
							unsigned int areaCount,
							bool reversed = false )
	{
		m_areaIndex = areaIx;
		m_areaCount = areaCount;
		m_reversed = reversed;
	}

	// --------------------------------------------------------------------------------------
	// Description:
	//   Submit instanced geometry to the device.
	// --------------------------------------------------------------------------------------
    virtual void SubmitGeometry( Tr2RenderContext& renderContext )
	{
		m_mesh->RenderAreas( m_areaIndex, m_areaCount, m_reversed, renderContext );
	}

	// --------------------------------------------------------------------------------------
	// Description:
	//   Gets the batch type name for PIX debugging.
	// Return value:
	//   Batch type name
	// --------------------------------------------------------------------------------------
	virtual const std::string& GetBatchTypeName( void ) const
	{ 
		static const std::string name = "Tr2InstancedMesh::Batch";
		return name; 
	}
private:
	// Instanced mesh to render
	Tr2InstancedMesh* m_mesh;
	// Area start index
    unsigned int m_areaIndex;
	// Number of geometry areas to render
    unsigned int m_areaCount;
	// If reversed index buffer is required
	bool m_reversed;
};

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InstancedMesh default constructor
// --------------------------------------------------------------------------------------
Tr2InstancedMesh::Tr2InstancedMesh( IRoot* lockobj )
	:Tr2Mesh( lockobj ),
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
	else if( IsMatch( value, m_instanceCount ) )
	{
		CreateVertexDeclaration();
		RebuildIndirectBuffers();
	}

	return Tr2Mesh::OnModified( value );
}

// --------------------------------------------------------------------------------------
// Description:
//   Rebuilds (either clears or creates) indirect parameter buffers for all areas of the
//   mesh.
// --------------------------------------------------------------------------------------
void Tr2InstancedMesh::RebuildIndirectBuffers()
{
	if( !m_instanceCount )
	{
		m_indirectParams.clear();
	}
	else
	{
		for( int type = 0; type < TRIBATCHTYPE_COUNT_OF_BATCH_TYPES; ++type )
		{
			Tr2MeshAreaVector* areas = GetAreas( TriBatchType( type ) );
			if( areas )
			{
				for( auto it = areas->begin(); it != areas->end(); ++it )
				{
					AreaKey key;
					key.index = ( *it )->GetIndex();
					key.count = ( *it )->GetCount();
					key.reversed = ( *it )->GetReversed();
					GetIndirectBuffer( key );
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets or create an indirect parameters buffer for given area properties. The instance
//   count field of the buffer is left with 0 and needs to be filled later with the 
//   actual count.
// Arguments:
//   key - Area parameters
// Return Value:
//   Indirect parameters buffer or NULL on error
// --------------------------------------------------------------------------------------
Tr2BufferAL Tr2InstancedMesh::GetIndirectBuffer( const AreaKey& key )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	auto found = m_indirectParams.find( key );
	if( found != m_indirectParams.end() )
	{
		if( found->second.IsValid() )
		{
			return found->second;
		}
		else
		{
			m_indirectParams.erase( found );
		}
	}

	unsigned areaIx = key.index;
	unsigned areaCount = key.count;

	if( !m_geometryResource || !m_geometryResource->IsGood() )
	{
		return Tr2BufferAL();
	}

	if( m_meshIndex >= int( m_geometryResource->GetMeshCount() ) )
	{
		return Tr2BufferAL();
	}

	TriGeometryResMeshData* pMesh = m_geometryResource->GetMeshData( m_meshIndex );
	if( !pMesh )
	{
		return Tr2BufferAL();
	}

	if( areaIx >= pMesh->m_areas.size() )
	{
		return Tr2BufferAL();
	}

	if( areaIx + areaCount > pMesh->m_areas.size() )
	{
		areaCount = (unsigned int)pMesh->m_areas.size() - areaIx;
	}
	
	const TriGeometryResAreaData& area = pMesh->m_areas[areaIx];

	unsigned int primCount = area.m_primitiveCount;
	for( unsigned int i = 1; i < areaCount; ++i )
	{
		const TriGeometryResAreaData& curArea = pMesh->m_areas[areaIx + i];
		primCount += curArea.m_primitiveCount;
	}

	uint32_t data[5];
	data[0] = primCount * 3;
	data[1] = 154;
	data[3] = 0;
	data[4] = 0;

	if( key.reversed )
	{
		data[2] = pMesh->m_primitiveCount * 3 - area.m_firstIndex - primCount * 3;
	}
	else
	{
		data[2] = area.m_firstIndex;
	}

	Tr2BufferAL buffer;
	if( FAILED( buffer.Create( Tr2RenderContextEnum::PIXEL_FORMAT_R32_UINT, 5, Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::DRAW_INDIRECT_ARGS, Tr2CpuUsage::NONE, data, renderContext ) ) )
	{
		return Tr2BufferAL();
	}
	m_indirectParams[key] = buffer;
	return buffer;
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
					ITr2MeshBatchCallback* callback ) const
{
	if( !GetDisplay() )
	{
		return;
	}
	if( m_instanceCount )
	{
		if( !m_instanceCount->GetGpuBuffer( 0 ) )
		{
			return;
		}
	}
	else
	{
		auto instanceGeometryResource = GetInstanceGeometryResource();
		if( !instanceGeometryResource || !instanceGeometryResource->GetInstanceBufferVertexCount( m_instanceMeshIndex ) )
		{
			return;
		}
	}
	bool rebuild = m_vertexDeclaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	if( !rebuild )
	{
		if( !m_instanceCount && GetInstanceGeometryResource() && GetInstanceGeometryResource()->IsInstanceDataReady() )
		{
			rebuild = m_instanceDeclaration != GetInstanceGeometryResource()->GetInstanceBufferVertexDeclaration( m_instanceMeshIndex );
		}
	}

	if( rebuild )
	{
		CreateVertexDeclaration();
		if( m_vertexDeclaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			return;
		}
	}

	for( auto it = areas->begin(); it != areas->end(); ++it )
	{
		Tr2MeshArea* area = *it;
		auto shadMat = area->GetMaterialInterface();

		if( !area->GetDisplay())
		{
			continue;
		}

		if( !shadMat )
		{
			continue;
		}

		Batch* batch = batches->Allocate<Batch>();
		// Note that this can fail if the accumulator can't add more batches!
		if( batch )
		{
			batch->SetPerObjectData( data );
			batch->SetMesh( const_cast<Tr2InstancedMesh*>( this ) );
			batch->SetMeshParameters( area->GetIndex(), area->GetCount(), area->GetReversed() );
			batch->SetShaderMaterial( area->GetMaterialInterface() );

			if( callback )
			{
				if( !callback->ProcessBatch( area, batch ) )
				{
					continue;
				}
			}

			batches->Commit( batch );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Calculates mesh sorting value using explicit mesh bounds.
// Arguments:
//   worldTransform - Local to world space transform
// Return value:
//   Sorting value (based on distance from camera) for the mesh
// --------------------------------------------------------------------------------------
float Tr2InstancedMesh::CalcMeshSortValue( const Matrix& worldTransform )
{
	Vector3 center = TransformCoord( m_boundingSphere.GetXYZ(), worldTransform );
	Vector3	d = center - Tr2Renderer::GetViewPosition();
    float distSq = LengthSq( d );

    return distSq;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns bounding box for this mesh.
// Arguments:
//   min - (out) Min bounds in local space
//   max - (out) Max bounds in local space
// Return value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2InstancedMesh::GetBoundingBox( Vector3& min, Vector3& max ) const
{
	if( m_boundsMethod == STATIC )
	{
		min = m_minBounds;
		max = m_maxBounds;
		return true;
	}
	else
	{
		if( !m_instanceGeometryResource )
		{
			return false;
		}
		if( !m_instanceGeometryResource->GetInstanceBufferBoundingBox( m_instanceMeshIndex, min, max ) )
		{
			return false;
		}
		Vector3 margin( m_maxInstanceSize, m_maxInstanceSize, m_maxInstanceSize );
		min -= margin;
		max += margin;
		return true;
	}
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
//   Returns area bounding box (equalt to explicit bounding box) for this mesh.
// Arguments:
//   areaIx - area index
//   min - (out) Min bounds in local space
//   max - (out) Max bounds in local space
// Return value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2InstancedMesh::GetAreaBoundingBox( unsigned int, Vector3& min, Vector3& max ) const
{
	return GetBoundingBox( min, max );
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns bounding sphere (based on explicit bounding box) for this mesh.
// Arguments:
//   sphere - (out) Bounding space
// Return value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2InstancedMesh::GetBoundingSphere( Vector4& sphere )
{
	Vector3 min, max;
	if( !GetBoundingBox( min, max ) )
	{
		return false;
	}
	Vector3 center = ( min + max ) * 0.5f;
	Vector3 extent( min - max );
	sphere = Vector4( center.x, center.y, center.z, Length( extent ) * 0.5f );
	return true;
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
//   Renders specified geometry areas.
// Arguments:
//   areaIx - Area start index
//   areaCount - Number of areas to render
//   reversed - If reversed render order is required
// --------------------------------------------------------------------------------------
void Tr2InstancedMesh::RenderAreas( unsigned int areaIx, 
									unsigned int areaCount, 
									bool reversed, 
									Tr2RenderContext& renderContext )
{
	if( m_instanceCount )
	{
		if( !m_geometryResource || !m_geometryResource->IsGood() )
		{
			return;
		}

		if( m_meshIndex >= int( m_geometryResource->GetMeshCount() ) )
		{
			return;
		}

		TriGeometryResMeshData* pMesh = m_geometryResource->GetMeshData( m_meshIndex );
		if( !pMesh )
		{
			return;
		}

		auto source = m_instanceCount->GetGpuBuffer( 0 );
		if( !source || !source->IsValid() )
		{
			return;
		}

		AreaKey key;
		key.index = areaIx;
		key.count = areaCount;
		key.reversed = reversed;
		auto params = GetIndirectBuffer( key );
		if( !params.IsValid() )
		{
			return;
		}
		renderContext.CopySubBuffer( params, 4, *source, 0, 4 );

		renderContext.m_esm.ApplyVertexDeclaration( pMesh->m_vertexDeclaration );
		renderContext.m_esm.ApplyStreamSource( 0, pMesh->m_vertexBuffer, 0, pMesh->m_bytesPerVertex );
		if( reversed )
		{
			m_geometryResource->ReverseIndexBuffer( m_meshIndex, renderContext );
			renderContext.m_esm.ApplyIndexBuffer( pMesh->m_reversedIndexBuffer );
		}
		else
		{
			renderContext.m_esm.ApplyIndexBuffer( pMesh->m_indexBuffer );
		}

		renderContext.SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES );
		renderContext.DrawIndexedInstancedIndirect( params, 0 );
	}
	else
	{
		auto instanceGeometryResource = GetInstanceGeometryResource();

		if( m_vertexDeclaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			return;
		}

		if( !m_geometryResource || !m_geometryResource->IsGood() )
		{
			return;
		}

		if( !instanceGeometryResource || !instanceGeometryResource->IsInstanceDataReady() )
		{
			return;
		}

		if( m_meshIndex >= int( m_geometryResource->GetMeshCount() ) )
		{
			return;
		}

		if( m_instanceMeshIndex >= int( instanceGeometryResource->GetInstanceBufferCount() ) )
		{
			return;
		}

		TriGeometryResMeshData* pMesh = m_geometryResource->GetMeshData( m_meshIndex );
		if( !pMesh )
		{
			return;
		}

		if( areaIx >= pMesh->m_areas.size() )
		{
			return;
		}

		if( areaIx + areaCount > pMesh->m_areas.size() )
		{
			areaCount = (unsigned int)pMesh->m_areas.size() - areaIx;
		}
	
		const TriGeometryResAreaData& area = pMesh->m_areas[areaIx];

		unsigned int primCount = area.m_primitiveCount;
		for( unsigned int i = 1; i < areaCount; ++i )
		{
			const TriGeometryResAreaData& curArea = pMesh->m_areas[areaIx + i];
			primCount += curArea.m_primitiveCount;
		}

		const unsigned instanceCount = instanceGeometryResource->GetInstanceBufferVertexCount( m_instanceMeshIndex );

		if( primCount && instanceCount )
		{
			renderContext.m_esm.ApplyVertexDeclaration( m_vertexDeclaration );
			renderContext.m_esm.ApplyStreamSource( 0, pMesh->m_vertexBuffer, 0, pMesh->m_bytesPerVertex );
			Tr2BufferAL vb;
			unsigned stride;
			instanceGeometryResource->GetVertexBuffer( m_instanceMeshIndex, vb, stride );
			renderContext.m_esm.ApplyStreamSource( 1, vb, 0, stride );
			if( reversed )
			{
				m_geometryResource->ReverseIndexBuffer( m_meshIndex, renderContext );
				renderContext.m_esm.ApplyIndexBuffer( pMesh->m_reversedIndexBuffer );
			}
			else
			{
				renderContext.m_esm.ApplyIndexBuffer( pMesh->m_indexBuffer );
			}

			renderContext.SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES );
			if( reversed )
			{
				renderContext.DrawIndexedInstanced( pMesh->m_vertexCount, pMesh->m_primitiveCount * 3 - area.m_firstIndex - primCount * 3, primCount, instanceCount );
			}
			else
			{
				renderContext.DrawIndexedInstanced( pMesh->m_vertexCount, area.m_firstIndex, primCount, instanceCount );
			}
			CCP_STATS_ADD( instancesRendered, instanceCount );
		}
	}
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
	return Tr2EffectStateManager::GetVertexDeclarationElements( meshData->m_vertexDeclaration, vd );
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
	
	if( m_instanceCount )
	{
		if( m_geometryResource && m_geometryResource->IsGood() && m_geometryResource->GetMeshData( m_meshIndex ) )
		{
			m_vertexDeclaration = m_geometryResource->GetMeshData( m_meshIndex )->m_vertexDeclaration;
		}
	}
	else
	{
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
		if( GetMeshVertexDeclaration( m_geometryResource, m_meshIndex, meshVD ) )
		{
			m_vertexDeclaration = MergeVertexDeclarations( meshVD, instanceVD );
		}
	}
}

void Tr2InstancedMesh::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Instance Mesh Bounds" );
}

void Tr2InstancedMesh::RenderDebugInfo( const Matrix& worldTransform, ITr2DebugRenderer2& renderer )
{
	if( renderer.HasOption( this, "Instance Mesh Bounds" ) )
	{
		if( m_boundsMethod == STATIC )
		{
			renderer.DrawBox( this, worldTransform, m_minBounds, m_maxBounds, Tr2DebugRenderer::Wireframe, Tr2DebugColor( 0xffaa8800, 0x22aa8800 ) );
		}
		else
		{
			Vector3 min, max;
			if( m_instanceGeometryResource && m_instanceGeometryResource->GetInstanceBufferBoundingBox( m_instanceMeshIndex, min, max ) )
			{
				renderer.DrawBox( this, worldTransform, min, max, Tr2DebugRenderer::Wireframe, Tr2DebugColor( 0xff008888, 0x22008888 ) );
				Vector3 margin( m_maxInstanceSize, m_maxInstanceSize, m_maxInstanceSize );
				min -= margin;
				max += margin;
				renderer.DrawBox( this, worldTransform, min, max, Tr2DebugRenderer::Wireframe, Tr2DebugColor( 0xff888888, 0x22888888 ) );
			}
		}
	}
}
