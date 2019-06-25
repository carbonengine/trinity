#include "StdAfx.h"
#include "EveChildBehaviorSystem.h"
#include "Eve/EveUpdateContext.h"
#include "include/TriMath.h"
#include "Behaviors/IBehavior.h"


EveChildBehaviorSystem::EveChildBehaviorSystem( IRoot* lockobj ) :
	m_vertexDeclaration( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_stride( 12 * sizeof( float ) ),
	m_vertexCount( 100 ),
	m_count( 0 ),
	m_maxVelocity( 100 ),
	m_maxForce( 50.f ),
	PARENTLOCK(m_behaviors)
{
	PrepareResources();
}

EveChildBehaviorSystem::~EveChildBehaviorSystem()
{
}

/////////////////////////////////////////////////////////////////////////////////////
// Tr2DeviceResource
bool EveChildBehaviorSystem::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	Tr2VertexDefinition def;
	def.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 0 );
	def.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 1 );
	def.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 2 );
	m_vertexDeclaration = renderContext.m_esm.GetVertexDeclarationHandle( def );

	auto hr = m_vertexBuffer.Create(
		m_stride,					// 12 * sizeof( float )
		m_vertexCount,				// Number of instances
		Tr2GpuUsage::VERTEX_BUFFER, // VERTEX_BUFFER
		Tr2CpuUsage::WRITE_OFTEN,	// WRITE_OFTEN
		nullptr,					// initialData
		renderContext );
	if( FAILED( hr ) )
	{
		return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// ITr2InstanceData

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface. Query if instance data is available for 
//   rendering.
// Return value:
//   true If instance data is available
//   false Otherwise
// --------------------------------------------------------------------------------------
bool EveChildBehaviorSystem::IsInstanceDataReady() const
{
	return m_vertexDeclaration != Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
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
//   Implements ITr2InstanceData interface. Returns vertex declaration handle for 
//   instance buffer.
// Arguments:
//   bufferIndex - (unused) instance buffer index
// Return value:
//   Vertex declaration handle
// --------------------------------------------------------------------------------------
unsigned int EveChildBehaviorSystem::GetInstanceBufferVertexDeclaration( unsigned int bufferIndex ) const
{
	return m_vertexDeclaration;
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
	return unsigned( m_agents.size() );
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

bool EveChildBehaviorSystem::GetInstanceBufferBoundingBox( unsigned int bufferIndex, Vector3& minBounds, Vector3& maxBounds ) const
{
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////
// EveChildMesh

//TODO: Remove this and instead use the new BehaviorGroup
void EveChildBehaviorSystem::UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	UpdateAgents( updateContext.GetDeltaT() );

	USE_MAIN_THREAD_RENDER_CONTEXT();
	UpdateBuffer( renderContext );

	EveChildMesh::UpdateSyncronous( updateContext, params );
}

/////////////////////////////////////////////////////////////////////////////////////
// EveChildBehaviorSystem
void EveChildBehaviorSystem::UpdateAgents( const float dt )
{
	//Calculate the behaviors
	for (auto agent = m_agents.begin(); agent != m_agents.end(); ++agent)
	{
		for (auto behavior = m_behaviors.begin(); behavior != m_behaviors.end(); ++behavior)
		{
			(*behavior)->CalculateBehavior( *agent, dt );
		}
	}
	//Move the agents based on the behaviors
	for (auto agent = m_agents.begin(); agent != m_agents.end(); ++agent)
	{
		agent->velocity = ClampLength( agent->velocity + agent->acceleration * dt, m_maxVelocity );
		//This made wandering behavior super weird where the agent would stop before turning
		//agent->acceleration = Vector3( 0, 0, 0 );
		agent->position += agent->velocity * dt;
		
		static const Vector3 zAxis( 0.f, 0.f, 1.f );
		TriQuaternionRotationArc( &agent->rotation, &zAxis, &agent->velocity );
	}
}

void EveChildBehaviorSystem::UpdateBuffer( Tr2RenderContext& renderContext )
{
	uint8_t *data;
	CR_RETURN( m_vertexBuffer.MapForWriting( data, renderContext ) );
	for (auto agent = m_agents.begin(); agent != m_agents.end(); ++agent)
	{
		Matrix m = Transpose( TransformationMatrix( Vector3( 10, 10, 10 ), agent->rotation, agent->position ) );
		memcpy( data, &m, 12 * sizeof( float ) );
		data += 12 * sizeof( float );
	}
	m_vertexBuffer.UnmapForWriting( renderContext );
}

void EveChildBehaviorSystem::AddAgent()
{
	DroneAgent agent;
	//This will probably be like a starting locator in near future, or m_worldPosition
	agent.position = Vector3(0, 0, 0);
	m_agents.push_back( agent );
	m_count++;
}

void EveChildBehaviorSystem::SetCount( int count )
{
	if (m_count < count)
	{
		int difference = count - m_count;
		for (int i = 0; i < difference; i++)
		{
			AddAgent();
		}
	}
}

//Also need to add a deleteAgent function


/////////////////////////////////////////////////////////////////////////////////////
// ITr2DebugRenderable
void EveChildBehaviorSystem::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Agents" );
	options.insert( "Behaviors" );
}

void EveChildBehaviorSystem::RenderDebugInfo( Tr2DebugRenderer& renderer )
{
	if (renderer.HasOption( this, "Agents" ))
	{
		for (auto agent = m_agents.begin(); agent != m_agents.end(); ++agent)
		{
			renderer.DrawSphere( this, TranslationMatrix( agent->position ) * EveChildTransform::m_worldTransform, 50, 6, Tr2DebugRenderer::Wireframe, 0xff555555 );
		}
	}

	if (renderer.HasOption( this, "Behaviors" ))
	{
		for (auto agent = m_agents.begin(); agent != m_agents.end(); ++agent)
		{
			m_behaviors[0]->RenderDebugInfo( renderer, agent->position );
		}
	}
}