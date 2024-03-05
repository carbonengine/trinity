
#include "StdAfx.h"
#include "EveChildBehaviorSystem.h"
#include "TriRenderBatch.h"
#include "Tr2Mesh.h"
#include "Tr2QuadRenderer.h"
#include "Resources/TriGeometryRes.h"
#include "Behaviors/BehaviorGroupBooster.h"

namespace
{
	class EveChildBehaviorSystemPerObjectData : public Tr2PerObjectData
	{
	public:
		virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
		{
			FillAndSetConstants( *buffers[Tr2RenderContextEnum::VERTEX_SHADER],
								 m_vsData,
								 sizeof( *m_vsData ),
								 Tr2RenderContextEnum::VERTEX_SHADER,
								 Tr2Renderer::GetPerObjectVSStartRegister(),
								 renderContext );
			FillAndSetConstants( *buffers[Tr2RenderContextEnum::PIXEL_SHADER],
				m_psData, sizeof( *m_psData ),
				Tr2RenderContextEnum::PIXEL_SHADER,
				Tr2Renderer::GetPerObjectPSStartRegister(),
				renderContext );
		}
		EveSpaceObjectPSData* m_psData;
		EveSpaceObjectVSData* m_vsData;
	};
}

// --------------------------------------------------------------------------------------
// Description
//   Render batch specialization for EveChildContainer-batches
// See Also
//   TriRenderBatch, ITr2GeometryBatch
// --------------------------------------------------------------------------------------
class TriBehaviorSystemInstancingBatch : public TriGeometryBatch
{
public:

	void SetGeometryResource( TriGeometryRes* val )
	{
		m_geometryResource = val;
	}

	// Set the group
	void SetGroup( BehaviorGroup* val )
	{
		m_group = val;
	}

	// Set the geometry provider
	void SetBehaviorSystemReference( EveChildBehaviorSystem* val )
	{
		m_geom = val;
	}

	void SetRenderType( EveChildBehaviorSystem::RenderType renderType )
	{
		m_renderType = renderType;
	}

	// Forward the SubmitGeometry call to the geometry provider
	void SubmitGeometry( Tr2RenderContext& renderContext )
	{
		if ( m_geom )
		{
			if( !m_group->m_display )
			{
				return;
			}
			auto booster = m_group->GetBooster();
			switch( m_renderType )
			{
			case EveChildBehaviorSystem::RENDER_SHIP:
				if( m_group->GetMesh()->GetDisplay() )
				{
					m_geom->Draw( this, renderContext, static_cast< unsigned int >( m_group->GetSize() ),
						m_group->GetVertexDeclarationHandle(), m_group->GetGroupIndexIndicator(), m_renderType );
				}				
				break;
			case EveChildBehaviorSystem::RENDER_BOOSTER:
				
				if( booster != nullptr && booster->GetDisplay() )
				{
					m_geom->Draw( this, renderContext, static_cast< unsigned int >( m_group->GetSize() ),
						booster->GetVertexDeclaration(), m_group->GetGroupIndexIndicator(),
						m_renderType );
				}
				break;
			default:
				break;
			}
		}
	}

	// Gets the batch type name for PIX debugging
	virtual const std::string& GetBatchTypeName( void ) const
	{
		static const std::string name = "TriGroupInstancingBatch";
		return name;
	}

	BehaviorGroupBoosterPtr GetBooster() {
		if( m_group ) {
			return m_group->GetBooster();
		}
		return nullptr;
	}

private:
	EveChildBehaviorSystemPtr m_geom;
	BehaviorGroupPtr m_group;
	EveChildBehaviorSystem::RenderType m_renderType;
};


EveChildBehaviorSystem::EveChildBehaviorSystem( IRoot* lockobj ) :
	PARENTLOCK( m_behaviorGroups ),
	PARENTLOCK( m_splineTunnels ),
	m_stride( 12 * sizeof( float ) ),
	m_vertexCount( 1 ),
	m_display( true ),
	m_behaviorGroupLoaded( false ),
	m_behaviorGroupLoadedForTunnel( false )
{
	m_behaviorGroups.SetNotify( this );
	m_splineTunnels.SetNotify( this );
	PrepareResources();
	// init per-object data with default values
	memset( &m_vsData, 0, sizeof( EveSpaceObjectVSData ) );
	memset( &m_psData, 0, sizeof( EveSpaceObjectPSData ) );
}

EveChildBehaviorSystem::~EveChildBehaviorSystem()
{
}

bool EveChildBehaviorSystem::Initialize()
{
	if ( m_staticTransform )
	{
		RebuildLocalTransform();
	}

	ChangeBufferVertexCount();

	return true;
}

void EveChildBehaviorSystem::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList )
{
	if ( theList == &m_behaviorGroups )
	{
		switch ( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if ( BehaviorGroupPtr handler = BlueCastPtr( value ) )
			{
				std::function<void( void )> f = std::bind( &EveChildBehaviorSystem::ChangeBufferVertexCount, this );
				handler->SetVertexFunctionReferance( f );
				handler->InitializeGeometryResource();
			}
			break;
		case BELIST_REMOVED:
			if ( BehaviorGroupPtr handler = BlueCastPtr( value ) )
			{
				ChangeBufferVertexCount();
			}
			break;
		case BELIST_LOADFINISHED:
			if ( BehaviorGroupPtr handler = BlueCastPtr( value ) )
			{
				std::function<void( void )> f = std::bind( &EveChildBehaviorSystem::ChangeBufferVertexCount, this );
				handler->SetVertexFunctionReferance( f );
				handler->InitializeGeometryResource();
			}
			else m_behaviorGroupLoaded = false; //this is for when this file is loaded but the groups have yet to be loaded
			break;
		default:
			break;
		}
	}
	if (theList == &m_splineTunnels)
	{
		switch ( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if (SplineTunnelGroupPtr handler = BlueCastPtr( value ))
			{
				std::function<void( void )> f = std::bind( &EveChildBehaviorSystem::UpdateTunnelRegistry, this );
				handler->SetSystemTunnelFunctionReferenceAndColor( f, 0xffffff00 );
			}
			break;
		case BELIST_REMOVED:
			if (SplineTunnelGroupPtr handler = BlueCastPtr( value ))
			{
				std::function<void( void )> f = std::bind( &EveChildBehaviorSystem::UpdateTunnelRegistry, this );
				handler->SetSystemTunnelFunctionReferenceAndColor( f, 0xffffff00 );
			}
			break;
		case BELIST_LOADFINISHED:
			if (SplineTunnelGroupPtr handler = BlueCastPtr( value ))
			{
				std::function<void( void )> f = std::bind( &EveChildBehaviorSystem::UpdateTunnelRegistry, this );
				handler->SetSystemTunnelFunctionReferenceAndColor( f, 0xffffff00 );
			}
			else m_behaviorGroupLoadedForTunnel = false; //this is for when this file is loaded but the groups have yet to be loaded
			break;
		default:
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Tr2DeviceResource
bool EveChildBehaviorSystem::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	
	// Start with a fresh buffer
	m_vertexBuffer = Tr2BufferAL();
	ChangeBufferVertexCount();
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface. Returns number of instance buffers.
// Return value:
//   1 always
// --------------------------------------------------------------------------------------
unsigned int EveChildBehaviorSystem::GetInstanceBufferCount() const
{
	return 1;
}


// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface. Returns number of instances in the instance 
//   buffer.
// Arguments:
//   bufferIndex - (unused) instance buffer index
// Return value:
//   Number of instances
// --------------------------------------------------------------------------------------
unsigned int EveChildBehaviorSystem::GetInstanceBufferVertexCount( unsigned int bufferIndex ) const
{
	size_t size = 0;
	for ( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		size += (*it)->GetSize();
	}
	return unsigned( size );
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface. Returns vertex buffer with instance data.
// Arguments:
//   bufferIndex - (unused) instance buffer index
//   buffer - (out) vertex buffer containing instance data (can be null)
//   stride - (out) vertex stride for the vertex buffer
// --------------------------------------------------------------------------------------
void EveChildBehaviorSystem::GetVertexBuffer( unsigned int bufferIndex, Tr2BufferAL& buffer, unsigned& stride )
{
	buffer = m_vertexBuffer;
	stride = m_stride;
}

// A Simple function to handle cases where a behavior system is loaded along with all it's children from a single file
// The onListNotify takes care of all other cases (like when just a new behavior group is loaded)
void EveChildBehaviorSystem::PassInVertexesToBehaviorGroups()
{
	for ( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		std::function<void( void )> f = std::bind( &EveChildBehaviorSystem::ChangeBufferVertexCount, this );
		(*it)->SetVertexFunctionReferance( f );
		(*it)->InitializeGeometryResource();
	}
	m_behaviorGroupLoaded = true;
}

// A Simple function to handle cases where a behavior system is loaded along with all it's children from a single file
// The onListNotify takes care of all other cases (like when just a new behavior group is loaded)
void EveChildBehaviorSystem::PassInTunnelFunctionsToBehaviorGroups()
{
	for (auto it = begin( m_splineTunnels ); it != end( m_splineTunnels ); ++it)
	{
		std::function<void( void )> f = std::bind( &EveChildBehaviorSystem::UpdateTunnelRegistry, this );
		(*it)->SetSystemTunnelFunctionReferenceAndColor( f, 0xffffff00 );
	}
	m_behaviorGroupLoadedForTunnel = true;
}

/////////////////////////////////////////////////////////////////////////////////////
// EveChildMesh
void EveChildBehaviorSystem::UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{

	// might be a better way to get these initialized but Iinitialize doesn't work
	// since these need to be called after children are initialized so basically a single frame later...
	if( !m_behaviorGroupLoaded )
	{
		PassInVertexesToBehaviorGroups();
	}
	if( !m_behaviorGroupLoadedForTunnel )
	{
		PassInTunnelFunctionsToBehaviorGroups();
	}

	for ( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		(*it)->CreateVertexDeclaration();
	}

	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		( *it )->UpdateSyncronous( updateContext, params );
	}

	UpdateAgents( updateContext.GetDeltaT() );
}

/////////////////////////////////////////////////////////////////////////////////////
// EveChildBehaviorSystem
void EveChildBehaviorSystem::UpdateAgents( const float dt )
{
	for ( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		( *it )->UpdateAgents( dt, *this );
	}
}

void EveChildBehaviorSystem::UpdateBuffer( Tr2RenderContext& renderContext )
{
	m_offsets.clear();
	m_offsets.push_back( 0 );
	uint8_t *data;
	Matrix WT = EveChildTransform::m_worldTransform;
	CR_RETURN( m_vertexBuffer.MapForWriting( data, renderContext ) );
	uint32_t I = 0;
	uint32_t totalShipsSoFar = 0;
	for ( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		(*it)->GetInfoForBuffer( data, WT );
		(*it)->SetGroupIndexIndicator( I );
		data += (*it)->GetCount() * 2 * m_stride;
		I++;
		totalShipsSoFar += (*it)->GetCount();
		m_offsets.push_back( totalShipsSoFar * 2 * m_stride );
	}
	m_vertexBuffer.UnmapForWriting( renderContext );
}

void EveChildBehaviorSystem::GetGroupBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType,
	const Tr2PerObjectData* perObjectData, Tr2MeshPtr mesh, BehaviorGroup* group )
{
	if ( mesh == nullptr )
	{
		return;
	}

	if ( mesh->GetGeometryResource() == nullptr )
	{
		return;
	}

	if ( !(mesh->GetGeometryResource()->IsGood()) )
	{
		return;
	}

	if ( mesh->GetGeometryResource()->GetMeshCount() < 1 )
	{
		return;
	}

	auto areaList = mesh->GetAreas( batchType );
	for ( auto srcMeshArea = areaList->begin(); srcMeshArea != areaList->end(); ++srcMeshArea )
	{
		auto a = *srcMeshArea;

		TriBehaviorSystemInstancingBatch* batch = batches->Allocate<TriBehaviorSystemInstancingBatch>();

		if ( nullptr == batch )
		{
			continue;
		}

		batch->SetPerObjectData( perObjectData );
		batch->SetShaderMaterial( a->GetMaterialInterface() );
		batch->SetMeshParameters( mesh->GetMeshIndex(), a->GetIndex(), a->GetCount(), a->IsReversed() );
		batch->SetGeometryResource( mesh->GetGeometryResource() );
		batch->SetBehaviorSystemReference( this );
		batch->SetRenderType( RENDER_SHIP );
		batch->SetGroup( group );
		batches->Commit( batch );
	}
}

void EveChildBehaviorSystem::GetGroupBoosterBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, 
	const Tr2PerObjectData* perObjectData, BehaviorGroup* group )
{
	if( batchType != TRIBATCHTYPE_ADDITIVE )
	{
		return;
	}
	
	TriBehaviorSystemInstancingBatch* batch = batches->Allocate<TriBehaviorSystemInstancingBatch>();

	if( batch == nullptr )
	{
		return;
	}

	auto booster = group->GetBooster();
	if( booster == nullptr )
	{
		return;
	}
	auto effect = booster->GetEffect();
	if( effect == nullptr )
	{
		return;
	}

	batch->SetPerObjectData( perObjectData );
	batch->SetShaderMaterial( (Tr2Material*) effect.p );
	batch->SetBehaviorSystemReference( this );
	batch->SetRenderType( RENDER_BOOSTER );
	batch->SetGroup( group );
	batches->Commit( batch );
}

void EveChildBehaviorSystem::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType,
	const Tr2PerObjectData* perObjectData, Tr2RenderReason reason  )
{
	if ( !m_display )
	{
		return;
	}

	if ( !m_vertexBuffer.IsValid() )
	{
		return;
	}

	// If all groups are not visble -> do not render
	bool isAnyGroupVisible = std::any_of( m_behaviorGroups.begin(),
										  m_behaviorGroups.end(),
										  []( BehaviorGroup* group )
										  { return group->IsGroupVisible() == true; } );
	if ( !isAnyGroupVisible )
	{
		return;
	}

	for ( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		auto group = *it;

		//same is 1 if you are far away and should only see sprites and 0 for only ships ( ]0-1[ -> both ) 
		const float same = group->AllTheSame();

		if ( same != 1 )
		{
			GetGroupBatches( batches, batchType, perObjectData, group->GetMesh(), group );
			GetGroupBoosterBatches( batches, batchType, perObjectData, group );
		}
	}
}


bool EveChildBehaviorSystem::HasTransparentBatches()
{
	bool isTrue = false;
	for ( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		auto mesh = (*it)->GetMesh();
		if ( m_display && mesh )
		{
			if ( !(mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT )->empty()) )
			{
				isTrue = true;
			}
		}
	}

	return isTrue;
}

// --------------------------------------------------------------------------------
// Description:
//   No transparency, no sorting
// --------------------------------------------------------------------------------
float EveChildBehaviorSystem::GetSortValue()
{
	return 0.f;
}

// --------------------------------------------------------------------------------
// Description:
//   The perobject data
// --------------------------------------------------------------------------------
Tr2PerObjectData* EveChildBehaviorSystem::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	EveChildBehaviorSystemPerObjectData* perObjectData = accumulator->Allocate<EveChildBehaviorSystemPerObjectData>();

	if ( !perObjectData )
	{
		return nullptr;
	}

	perObjectData->m_vsData = &m_vsData;
	perObjectData->m_psData = &m_psData;
	return perObjectData;
}

// --------------------------------------------------------------------------------
// Description:
//   Setup instanced rendering and call DIP
// --------------------------------------------------------------------------------
void EveChildBehaviorSystem::Draw( TriBehaviorSystemInstancingBatch* batch, Tr2RenderContext& renderContext, int count,
	unsigned int vertexDecl, int groupIndex, RenderType renderType )
{
	if( !m_behaviorGroupLoaded )
	{
		return;
	}
	switch( renderType )
	{
	case RENDER_SHIP:
		DrawMeshes( batch, renderContext, count, vertexDecl, groupIndex );
		break;
	case RENDER_BOOSTER:
		DrawBoosters( batch, renderContext, count, vertexDecl, groupIndex );
		break;
	default:
		break;
	}
}

void EveChildBehaviorSystem::DrawMeshes( TriBehaviorSystemInstancingBatch* batch, Tr2RenderContext& renderContext, int count,
	unsigned int vertexDecl, int groupIndex )
{
	auto geometry = batch->GetGeometryResource();

	if( geometry == nullptr ) { return; }
	if( !( geometry->IsGood() ) ) { return; }
	if( geometry->GetMeshCount() < 1 ) { return; }
	if( !m_vertexBuffer.IsValid() ) { return; }

	const TriGeometryResMeshData* meshData = geometry->GetMeshData( 0 );
	auto areaIx = batch->GetAreaIndex();
	auto areaCount = batch->GetAreaCount();

	if( areaIx >= meshData->m_areas.size() )
	{
		return;
	}

	if( areaIx + areaCount > meshData->m_areas.size() )
	{
		areaCount = static_cast< unsigned int >( meshData->m_areas.size() ) - areaIx;
	}

	const TriGeometryResAreaData& area = meshData->m_areas[areaIx];

	unsigned int primCount = area.m_primitiveCount;
	for( unsigned int i = 1; i < areaCount; ++i )
	{
		const TriGeometryResAreaData& curArea = meshData->m_areas[areaIx + i];
		primCount += curArea.m_primitiveCount;
	}
	
	renderContext.m_esm.ApplyVertexDeclaration( vertexDecl );
	renderContext.m_esm.ApplyIndexBuffer( meshData->m_indexBuffer );
	// Stream 0: "geometry": here: our ship geometry
	renderContext.m_esm.ApplyStreamSource( 0, meshData->m_vertexBuffer, 0, meshData->m_bytesPerVertex );
	// Stream 1: instance", here: instance index
	renderContext.m_esm.ApplyStreamSource( 1, m_vertexBuffer, m_offsets[groupIndex], 2 * m_stride );

	renderContext.SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES );
	renderContext.DrawIndexedInstanced( meshData->m_vertexCount, area.m_firstIndex, primCount, count );
}

void EveChildBehaviorSystem::DrawBoosters( TriBehaviorSystemInstancingBatch* batch, Tr2RenderContext& renderContext, int count,
	unsigned int vertexDecl, int groupIndex )
{
	auto boosters = batch->GetBooster();
	
	if( boosters == nullptr )
	{
		return;
	}
	boosters->Draw( renderContext, &m_vertexBuffer, m_offsets[groupIndex] + m_stride, 2 * m_stride, count );
}

/////////////////////////////////////////////////////////////////////////////////////
// ITr2DebugRenderable
void EveChildBehaviorSystem::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "SplineTunnels" );

	for ( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		(*it)->GetDebugOptions( options );
	}
}

void EveChildBehaviorSystem::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	for ( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		(*it)->RenderDebugInfo( renderer, EveChildTransform::m_worldTransform );
	}

	if (renderer.HasOption( this, "SplineTunnels" ))
	{
		for (auto it = begin( m_splineTunnels ); it != end( m_splineTunnels ); ++it)
		{
			(*it)->RenderDebugInfo( renderer, EveChildTransform::m_worldTransform );
		}
	}
}

const std::vector<SplineTunnel>* EveChildBehaviorSystem::GetTunnels() const
{
	return &m_tunnels;
}

SplineTunnelGroupVector* EveChildBehaviorSystem::GetSplineTunnels()
{
	return &m_splineTunnels;
}

void EveChildBehaviorSystem::UpdateTunnelRegistry()
{
	m_tunnels.clear();
	int id = 0;
	for (auto it = begin( m_splineTunnels ); it != end( m_splineTunnels ); ++it)
	{
		auto group = (*it)->GetTunnels();
		for (auto tunnel = begin( *group ); tunnel != end( *group ); ++tunnel)
		{
			(*tunnel).tunnelID = id;
			id++;
			m_tunnels.push_back(*tunnel);
		}
	}

	for ( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		( *it )->InitializeGeometryResource();
	}
}

void EveChildBehaviorSystem::ChangeBufferVertexCount()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	size_t temp = 0;

	for ( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		temp += (*it)->GetSize();
	}

	unsigned int numAgents = static_cast<unsigned int>(temp);
	m_vertexCount = numAgents;

	// TODO: Review m_vertexCount
	// Prevent the vertex count from being 0 (it creates an exception and prevents us from debugging)
	if ( m_vertexCount == 0 ) { m_vertexCount++; }

	auto b = m_vertexBuffer.Create(
		2 * m_stride,				// 12 * sizeof( float )
		m_vertexCount + 1,			// Number of instances
		Tr2GpuUsage::VERTEX_BUFFER, // VERTEX_BUFFER
		Tr2CpuUsage::WRITE_OFTEN,	// WRITE_OFTEN
		nullptr,
		renderContext );

}

// for validation and objects interacting with the shader attributes
std::vector<std::pair<int, int>> EveChildBehaviorSystem::GetVertexElementAddedThroughCode() const
{
	std::vector<std::pair<int, int>> out;
	out.emplace_back( std::pair<int, int>( Tr2VertexDefinition::TEXCOORD, 8 ) );
	out.emplace_back( std::pair<int, int>( Tr2VertexDefinition::TEXCOORD, 9 ) );
	out.emplace_back( std::pair<int, int>( Tr2VertexDefinition::TEXCOORD, 10 ) );
	return out;
}


/////////////////////////////////////////////////////////////////////////////////////
// IEveSpaceObjectChild
void EveChildBehaviorSystem::UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	Matrix localToWorldTransform;

	if ( nullptr != params.childParent )
	{
		params.childParent->GetLocalToWorldTransform( localToWorldTransform );
	}
	else if ( nullptr != params.spaceObjectParent )
	{
		params.spaceObjectParent->GetLocalToWorldTransform( localToWorldTransform );
		params.spaceObjectParent->GetPerObjectStructs( m_vsData, m_psData );
	}
	else
	{
		localToWorldTransform = params.localToWorldTransform;
	}
	m_vsData.worldTransformLast = Transpose( m_worldTransform );

	UpdateTransform( localToWorldTransform );

	m_vsData.worldTransform = Transpose( m_worldTransform );
	m_vsData.invWorldTransform = Inverse( m_worldTransform );

	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		( *it )->UpdateAsyncronous( updateContext );
	}
}

const char* EveChildBehaviorSystem::GetName() const
{
	return m_name;
}

void EveChildBehaviorSystem::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	EveChildTransform::Setup( scale, rotation, translation, lowestLodVisible );
}

void EveChildBehaviorSystem::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( !m_display )
	{
		return;
	}

	if( !m_behaviorGroupLoaded )
	{
		return;
	}

	renderables.push_back( this );

	USE_MAIN_THREAD_RENDER_CONTEXT();
	UpdateBuffer( renderContext );

	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		( *it )->GetRenderables( renderables );
	}
}

void EveChildBehaviorSystem::SetName( const char* name )
{
}

void EveChildBehaviorSystem::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod )
{
	if( !m_display )
	{
		return;
	}

	for ( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		( *it )->UpdateVisibility( frustum, m_worldTransform );
	}
}

bool EveChildBehaviorSystem::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	return true;
}

bool EveChildBehaviorSystem::GetInstanceBufferBoundingBox( unsigned int bufferIndex, Vector3& minBounds, Vector3& maxBounds ) const
{
	return false;
}

void EveChildBehaviorSystem::GetLocalToWorldTransform( Matrix& transform ) const
{
}

void EveChildBehaviorSystem::ChangeLOD( Tr2Lod lod )
{
}

void EveChildBehaviorSystem::GetLights( Tr2LightManager& lightManager ) const
{
	if( !m_display )
	{
		return;
	}

	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		( *it )->AddLights( lightManager, m_worldTransform );
	}
}

void EveChildBehaviorSystem::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer ) 
{
	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		( *it )->RegisterWithQuadRenderer( quadRenderer );
	}
}

void EveChildBehaviorSystem::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	if( !m_display )
	{
		return;
	}

	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		( *it )->AddQuadsToQuadRenderer( frustum, quadRenderer );
	}
}

Matrix EveChildBehaviorSystem::GetWorldTransform()
{
	return m_worldTransform;
}


