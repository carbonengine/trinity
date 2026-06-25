// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "EveChildBehaviorSystem.h"
#include "TriRenderBatch.h"
#include "Tr2Mesh.h"
#include "Tr2QuadRenderer.h"
#include "Resources/TriGeometryRes.h"
#include "Behaviors/BehaviorGroupBooster.h"
#include "Shader/Tr2Effect.h"

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
							 m_psData,
							 sizeof( *m_psData ),
							 Tr2RenderContextEnum::PIXEL_SHADER,
							 Tr2Renderer::GetPerObjectPSStartRegister(),
							 renderContext );
	}

	void ApplyConstantBuffers( Tr2IndirectDrawBufferWriter& writer, Tr2RenderContext& renderContext ) const
	{
		writer.SetPerObjectData( Tr2RenderContextEnum::VERTEX_SHADER, m_vsData, sizeof( *m_vsData ) );
		writer.SetPerObjectData( Tr2RenderContextEnum::PIXEL_SHADER, m_psData, sizeof( *m_psData ) );
	}

	EveSpaceObjectPSData* m_psData;
	EveSpaceObjectVSData* m_vsData;
};
}


EveChildBehaviorSystem::EveChildBehaviorSystem( IRoot* lockobj ) :
	PARENTLOCK( m_behaviorGroups ),
	PARENTLOCK( m_splineTunnels ),
	m_shipStride( 24 * sizeof( float ) ),
	m_boosterStride( 12 * sizeof( float ) ),
	m_instanceCount( 1 ),
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
	if( m_staticTransform )
	{
		RebuildLocalTransform();
	}

	ChangeBufferInstanceCount();

	return true;
}

bool EveChildBehaviorSystem::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_display ) )
	{
		ReRegister();
	}
	return true;
}

void EveChildBehaviorSystem::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList )
{
	if( theList == &m_behaviorGroups )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( BehaviorGroupPtr handler = BlueCastPtr( value ) )
			{
				std::function<void( void )> f = std::bind( &EveChildBehaviorSystem::ChangeBufferInstanceCount, this );
				handler->SetVertexFunctionReferance( f );
				handler->InitializeGeometryResource();
			}
			break;
		case BELIST_REMOVED:
			if( BehaviorGroupPtr handler = BlueCastPtr( value ) )
			{
				ChangeBufferInstanceCount();
			}
			break;
		case BELIST_LOADFINISHED:
			if( BehaviorGroupPtr handler = BlueCastPtr( value ) )
			{
				std::function<void( void )> f = std::bind( &EveChildBehaviorSystem::ChangeBufferInstanceCount, this );
				handler->SetVertexFunctionReferance( f );
				handler->InitializeGeometryResource();
			}
			else
				m_behaviorGroupLoaded = false; //this is for when this file is loaded but the groups have yet to be loaded
			break;
		default:
			break;
		}
	}
	if( theList == &m_splineTunnels )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( SplineTunnelGroupPtr handler = BlueCastPtr( value ) )
			{
				std::function<void( void )> f = std::bind( &EveChildBehaviorSystem::UpdateTunnelRegistry, this );
				handler->SetSystemTunnelFunctionReferenceAndColor( f, 0xffffff00 );
			}
			break;
		case BELIST_REMOVED:
			if( SplineTunnelGroupPtr handler = BlueCastPtr( value ) )
			{
				std::function<void( void )> f = std::bind( &EveChildBehaviorSystem::UpdateTunnelRegistry, this );
				handler->SetSystemTunnelFunctionReferenceAndColor( f, 0xffffff00 );
			}
			break;
		case BELIST_LOADFINISHED:
			if( SplineTunnelGroupPtr handler = BlueCastPtr( value ) )
			{
				std::function<void( void )> f = std::bind( &EveChildBehaviorSystem::UpdateTunnelRegistry, this );
				handler->SetSystemTunnelFunctionReferenceAndColor( f, 0xffffff00 );
			}
			else
				m_behaviorGroupLoadedForTunnel = false; //this is for when this file is loaded but the groups have yet to be loaded
			break;
		default:
			break;
		}
	}
	if( theList == &m_behaviorGroups && ( event & BELIST_LOADING ) == 0 )
	{
		if( IsInRegistry() )
		{
			switch( event & BELIST_EVENTMASK )
			{
			case BELIST_INSERTED:
				if( EveEntityPtr entity = BlueCastPtr( value ) )
				{
					entity->Register( GetComponentRegistry() );
				}
				break;
			case BELIST_REMOVED:
				if( EveEntityPtr entity = BlueCastPtr( value ) )
				{
					entity->UnRegister( GetComponentRegistry() );
				}
				break;
			case BELIST_UNLOADSTART:
				for( ssize_t i = 0; i < theList->GetSize(); ++i )
				{
					if( EveEntityPtr entity = BlueCastPtr( theList->GetAt( i ) ) )
					{
						entity->UnRegister( GetComponentRegistry() );
					}
				}
				break;
			default:
				break;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Tr2DeviceResource
bool EveChildBehaviorSystem::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	// Start with a fresh buffer
	m_shipInstanceBuffer = Tr2BufferAL();
	m_boosterInstanceBuffer = Tr2BufferAL();
	ChangeBufferInstanceCount();
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
	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		size += ( *it )->GetSize();
	}
	return unsigned( size );
}

// A Simple function to handle cases where a behavior system is loaded along with all it's children from a single file
// The onListNotify takes care of all other cases (like when just a new behavior group is loaded)
void EveChildBehaviorSystem::PassInVertexesToBehaviorGroups()
{
	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		std::function<void( void )> f = std::bind( &EveChildBehaviorSystem::ChangeBufferInstanceCount, this );
		( *it )->SetVertexFunctionReferance( f );
		( *it )->InitializeGeometryResource();
	}
	m_behaviorGroupLoaded = true;
}

// A Simple function to handle cases where a behavior system is loaded along with all it's children from a single file
// The onListNotify takes care of all other cases (like when just a new behavior group is loaded)
void EveChildBehaviorSystem::PassInTunnelFunctionsToBehaviorGroups()
{
	for( auto it = begin( m_splineTunnels ); it != end( m_splineTunnels ); ++it )
	{
		std::function<void( void )> f = std::bind( &EveChildBehaviorSystem::UpdateTunnelRegistry, this );
		( *it )->SetSystemTunnelFunctionReferenceAndColor( f, 0xffffff00 );
	}
	m_behaviorGroupLoadedForTunnel = true;
}

/////////////////////////////////////////////////////////////////////////////////////
// EveChildMesh
void EveChildBehaviorSystem::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
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

	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		( *it )->CreateVertexDeclaration();
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
	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		( *it )->UpdateAgents( dt, *this );
	}
}

void EveChildBehaviorSystem::UpdateBuffer( Tr2RenderContext& renderContext )
{
	m_startInstanceValues.clear();

	uint8_t* shipData;
	uint8_t* boosterData;


	CR_RETURN( m_shipInstanceBuffer.MapForWriting( shipData, renderContext ) );
	ON_BLOCK_EXIT( [&] { m_shipInstanceBuffer.UnmapForWriting( renderContext ); } );

	CR_RETURN( m_boosterInstanceBuffer.MapForWriting( boosterData, renderContext ) );
	ON_BLOCK_EXIT( [&] { m_boosterInstanceBuffer.UnmapForWriting( renderContext ); } );

	Matrix WT = EveChildTransform::m_worldTransform;

	uint32_t totalShipsSoFar = 0;
	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		int count = ( *it )->GetCount();

		//get ship instance data
		( *it )->GetShipInfoForBuffer( shipData, WT );
		shipData += count * m_shipStride;

		//get booster instance data
		( *it )->GetBoosterInfoForBuffer( boosterData, WT );
		boosterData += count * m_boosterStride;

		//save base instance offset for this group
		( *it )->SetGroupIndexIndicator( static_cast<uint32_t>( m_startInstanceValues.size() ) );
		m_startInstanceValues.push_back( totalShipsSoFar );
		totalShipsSoFar += count;
	}
}

void EveChildBehaviorSystem::GetGroupBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2MeshPtr mesh, BehaviorGroup* group )
{
	if( !group->m_display )
	{
		return;
	}
	if( mesh == nullptr )
	{
		return;
	}
	auto geometry = mesh->GetGeometryResource();
	if( !geometry || !geometry->IsGood() )
	{
		return;
	}

	TriGeometryResLodData* lod = geometry->GetMeshLod( mesh->GetMeshIndex(), 0 );
	if( !lod || !lod->m_allocationsValid )
	{
		return;
	}

	auto areaList = mesh->GetAreas( batchType );
	for( auto& area : *areaList )
	{
		Tr2RenderBatch batch = CreateGeometryBatch( lod, area, perObjectData );
		batch.SetVertexDeclaration( group->GetVertexDeclarationHandle() );
		batch.SetStreamSource( 1, m_shipInstanceBuffer, m_shipStride );
		batch.m_startInstanceLocation = m_startInstanceValues[group->GetGroupIndexIndicator()];
		batch.m_instanceCount = uint32_t( group->GetSize() );
		batches->Commit( batch );
	}
}

void EveChildBehaviorSystem::GetGroupBoosterBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, BehaviorGroup* group )
{
	if( batchType != TRIBATCHTYPE_ADDITIVE )
	{
		return;
	}

	auto booster = group->GetBooster();
	if( booster == nullptr || !booster->GetDisplay() )
	{
		return;
	}

	auto batch = group->GetBooster()->GetBatch(
		&m_boosterInstanceBuffer,
		m_startInstanceValues[group->GetGroupIndexIndicator()],
		m_boosterStride,
		uint32_t( group->GetSize() ) );
	batch.SetPerObjectData( perObjectData );
	batches->Commit( batch );
}

void EveChildBehaviorSystem::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( !m_display )
	{
		return;
	}

	if( !m_shipInstanceBuffer.IsValid() || !m_boosterInstanceBuffer.IsValid() )
	{
		return;
	}

	// If all groups are not visble -> do not render
	bool isAnyGroupVisible = std::any_of( m_behaviorGroups.begin(),
										  m_behaviorGroups.end(),
										  []( BehaviorGroup* group ) { return group->IsGroupVisible() == true; } );
	if( !isAnyGroupVisible )
	{
		return;
	}

	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		auto group = *it;

		//same is 1 if you are far away and should only see sprites and 0 for only ships ( ]0-1[ -> both )
		const float same = group->AllTheSame();

		if( same != 1 )
		{
			GetGroupBatches( batches, batchType, perObjectData, group->GetMesh(), group );
			GetGroupBoosterBatches( batches, batchType, perObjectData, group );
		}
	}
}


bool EveChildBehaviorSystem::HasTransparentBatches()
{
	bool hasTransparentBatches = false;
	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		auto mesh = ( *it )->GetMesh();
		if( m_display && mesh )
		{
			if( !( mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT )->empty() ) )
			{
				hasTransparentBatches = true;
				break;
			}
		}
	}

	return hasTransparentBatches;
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

	if( !perObjectData )
	{
		return nullptr;
	}

	perObjectData->m_vsData = &m_vsData;
	perObjectData->m_psData = &m_psData;
	return perObjectData;
}

/////////////////////////////////////////////////////////////////////////////////////
// ITr2DebugRenderable
void EveChildBehaviorSystem::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "SplineTunnels" );

	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		( *it )->GetDebugOptions( options );
	}
}

void EveChildBehaviorSystem::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		( *it )->RenderDebugInfo( renderer, EveChildTransform::m_worldTransform );
	}

	if( renderer.HasOption( this, "SplineTunnels" ) )
	{
		for( auto it = begin( m_splineTunnels ); it != end( m_splineTunnels ); ++it )
		{
			( *it )->RenderDebugInfo( renderer, EveChildTransform::m_worldTransform );
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
	for( auto it = begin( m_splineTunnels ); it != end( m_splineTunnels ); ++it )
	{
		auto group = ( *it )->GetTunnels();
		for( auto tunnel = begin( *group ); tunnel != end( *group ); ++tunnel )
		{
			( *tunnel ).tunnelID = id;
			id++;
			m_tunnels.push_back( *tunnel );
		}
	}

	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		( *it )->InitializeGeometryResource();
	}
}

void EveChildBehaviorSystem::ChangeBufferInstanceCount()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	size_t temp = 0;

	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		temp += ( *it )->GetSize();
	}

	unsigned int numAgents = static_cast<unsigned int>( temp );
	m_instanceCount = numAgents;

	// TODO: Review m_instanceCount
	// Prevent the vertex count from being 0 (it creates an exception and prevents us from debugging)
	if( m_instanceCount == 0 )
	{
		m_instanceCount++;
	}

	CR_RETURN( m_shipInstanceBuffer.Create(
		m_shipStride, // 12 * sizeof( float )
		m_instanceCount, // Number of instances
		Tr2GpuUsage::VERTEX_BUFFER, // VERTEX_BUFFER
		Tr2CpuUsage::WRITE_OFTEN, // WRITE_OFTEN
		nullptr,
		renderContext ) );

	CR_RETURN( m_boosterInstanceBuffer.Create(
		m_boosterStride, // 12 * sizeof( float )
		m_instanceCount, // Number of instances
		Tr2GpuUsage::VERTEX_BUFFER, // VERTEX_BUFFER
		Tr2CpuUsage::WRITE_OFTEN, // WRITE_OFTEN
		nullptr,
		renderContext ) );
}

// for validation and objects interacting with the shader attributes
std::vector<std::pair<int, int>> EveChildBehaviorSystem::GetVertexElementAddedThroughCode() const
{
	std::vector<std::pair<int, int>> out;
	out.emplace_back( std::pair<int, int>( Tr2VertexDefinition::TEXCOORD, 8 ) );
	out.emplace_back( std::pair<int, int>( Tr2VertexDefinition::TEXCOORD, 9 ) );
	out.emplace_back( std::pair<int, int>( Tr2VertexDefinition::TEXCOORD, 10 ) );
	out.emplace_back( std::pair<int, int>( Tr2VertexDefinition::TEXCOORD, 11 ) );
	out.emplace_back( std::pair<int, int>( Tr2VertexDefinition::TEXCOORD, 12 ) );
	out.emplace_back( std::pair<int, int>( Tr2VertexDefinition::TEXCOORD, 13 ) );
	return out;
}


/////////////////////////////////////////////////////////////////////////////////////
// IEveSpaceObjectChild
void EveChildBehaviorSystem::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	Matrix localToWorldTransform;

	if( nullptr != params.childParent )
	{
		params.childParent->GetLocalToWorldTransform( localToWorldTransform );
	}
	else if( nullptr != params.spaceObjectParent )
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

	m_psData.worldTransform = m_vsData.worldTransform;
	m_psData.worldTransformLast = m_vsData.worldTransformLast;
	m_psData.invWorldTransform = m_vsData.invWorldTransform;

	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		( *it )->UpdateAsyncronous( updateContext );
	}

	m_hasUpdated = true;
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
	if( !m_display || !m_hasUpdated )
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

void EveChildBehaviorSystem::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{
	if( !m_display )
	{
		return;
	}

	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		( *it )->UpdateVisibility( updateContext, m_worldTransform );
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

void EveChildBehaviorSystem::RegisterComponents()
{
	if( !m_display )
	{
		return;
	}

	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		if( EveEntityPtr entity = BlueCastPtr( *it ) )
		{
			entity->Register( GetComponentRegistry() );
		}
	}
}

void EveChildBehaviorSystem::UnRegisterComponents()
{
	for( auto it = begin( m_behaviorGroups ); it != end( m_behaviorGroups ); ++it )
	{
		if( EveEntityPtr entity = BlueCastPtr( *it ) )
		{
			entity->UnRegister( GetComponentRegistry() );
		}
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
