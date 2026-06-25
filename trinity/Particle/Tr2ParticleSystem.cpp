// Copyright © 2011 CCP ehf.

#include "StdAfx.h"
#include "Tr2ParticleSystem.h"
#include "TriDevice.h"
#include "ITr2GenericEmitter.h"
#include "ITr2ParticleForce.h"
#include "ITr2GenericParticleConstraint.h"
#include "TbbStub.h"
#include "TriSettingsRegistrar.h"
#include "Tr2VertexDefinitionUtilities.h"
#include "TriFrustum.h"

using namespace Tr2RenderContextEnum;

int Tr2ParticleSystem::s_totalParticleCount = 0;

unsigned g_maxParticleCountCap = 10000u;
TRI_REGISTER_SETTING( "maxParticleCountCap", g_maxParticleCountCap );
static unsigned s_maxParticleCount = 1024 * 512;
TRI_REGISTER_SETTING( "cpuMaxParticleCount", s_maxParticleCount );

CCP_STATS_DECLARE( statTotalParticleCount, "Trinity/totalParticleCount", false, CST_COUNTER_HIGH, "Tr2ParticleSystem particles allocated (not necessarily drawn/updated)" );
CCP_STATS_DECLARE( statAliveParticleCount, "Trinity/aliveParticleCount", true, CST_COUNTER_HIGH, "Tr2ParticleSystem particles alive (not necessarily drawn/updated)" );
CCP_STATS_DECLARE( statUpdatedParticleCount, "Trinity/updatedParticleCount", true, CST_COUNTER_HIGH, "Tr2ParticleSystem particles updated last frame" );

CcpAtomic<uint32_t> s_aliveParticleCount( 0 );
CcpAtomic<uint32_t> s_updatedParticleCount( 0 );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2ParticleSystem default constructor
// --------------------------------------------------------------------------------------
Tr2ParticleSystem::Tr2ParticleSystem( IRoot* lockobj ) :
	PARENTLOCK( m_elements ),
	PARENTLOCK( m_forces ),
	PARENTLOCK( m_constraints ),
	m_declaration( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_declarationHash( 0 ),
	m_indexes( nullptr ),
	m_maxParticleCount( 0 ),
	m_originalMaxParticles( 0 ),
	m_aliveCount( 0 ),
	m_bufferDirty( true ),
	m_insertDeleteMutex( nullptr ),
	m_lastUpdate( 0 ),
	m_applyAging( true ),
	m_updateSimulation( true ),
	m_applyForce( true ),
	m_requiresSorting( false ),
	m_sortingAllowed( true ),
	m_isGlobal( false ),
	m_isValid( false ),
	m_AabbMin( 0.0f, 0.0f, 0.0f ),
	m_AabbMax( 0.0f, 0.0f, 0.0f ),
	m_updatePeriod( 1 ),
	m_updatePeriodClock( 0 ),
	m_peakAliveCount( 0 ),
	m_useSimTimeRebase( false ),
	m_isUsingSimTimeRebase( false ),
	m_shouldSortVisible( true ),
	m_previousDataOutdated( true ),
	m_worldTransform( IdentityMatrix() )
{
	for( unsigned i = 0; i < Tr2ParticleElementData::COUNT; ++i )
	{
		m_vertexSizes[i] = 0;
		m_buffers[i] = nullptr;
	}
	for( unsigned i = 0; i < Tr2ParticleElementDeclarationName::CUSTOM; ++i )
	{
		m_semanticElements[i].m_offset = -1;
	}

	m_sortingReferencePoint = (XMVECTOR*)CCP_ALIGNED_MALLOC(
		"Tr2ParticleSystem::m_sortingReferencePoint",
		sizeof( XMVECTOR ),
		16 );

	GetAllSystems().insert( this );

	m_constraints.SetNotify( this );
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2ParticleSystem destructor
// --------------------------------------------------------------------------------------
Tr2ParticleSystem::~Tr2ParticleSystem()
{
	GetAllSystems().erase( this );
	DestroyBuffers();
	CCP_ALIGNED_FREE( m_sortingReferencePoint );
	if( m_isUsingSimTimeRebase )
	{
		BeOS->UnregisterForSimTimeRebase( this );
	}
	CCP_DELETE m_insertDeleteMutex;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IInitialize interface. Builds internal particle declaration and creates
//   D3D resources.
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2ParticleSystem::Initialize()
{
	UpdateElementDeclaration();
	OnPrepareResources();
	if( m_useSimTimeRebase )
	{
		BeOS->RegisterForSimTimeRebase( this );
		m_isUsingSimTimeRebase = true;
	}
	if( m_emissionOnDeathEmitter )
	{
		m_emissionOnDeathEmitter->SetThreadSafeFlag();
	}
	if( m_emissionWhileAliveEmitter )
	{
		m_emissionWhileAliveEmitter->SetThreadSafeFlag();
	}

	m_originalMaxParticles = m_maxParticleCount;

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements INotify interface.  Allows the system to respond to parameter changes
//   generated in Python. Monitors changes to maximum nuber of particles (to resize
//   buffers), requiresSorting flag and useSimTimeRebase flag.
// Arguments:
//   value - The Blue-exposed parameter that changed
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2ParticleSystem::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_requiresSorting ) )
	{
		CCP_DELETE[] m_indexes;
		if( m_requiresSorting && m_maxParticleCount )
		{
			m_indexes = CCP_NEW( "Tr2ParticleSystem::m_indexes" ) unsigned[m_maxParticleCount];
		}
		else
		{
			m_indexes = nullptr;
		}
	}
	else if( IsMatch( value, m_useSimTimeRebase ) )
	{
		if( m_useSimTimeRebase )
		{
			BeOS->RegisterForSimTimeRebase( this );
			m_isUsingSimTimeRebase = true;
		}
		else
		{
			BeOS->UnregisterForSimTimeRebase( this );
			m_isUsingSimTimeRebase = false;
		}
	}
	else if( IsMatch( value, m_emissionOnDeathEmitter ) && m_emissionOnDeathEmitter )
	{
		m_emissionOnDeathEmitter->SetThreadSafeFlag();
	}
	else if( IsMatch( value, m_emissionWhileAliveEmitter ) && m_emissionWhileAliveEmitter )
	{
		m_emissionWhileAliveEmitter->SetThreadSafeFlag();
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for setting maxParticleCount of the particle system. This is used
//   to setup a Blue property. If there are live particles the particle system buffer
//   will be rebuilt and all particles cleared.
// Arguments:
//   maxParticleCount - The new maxParticleCount value
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::SetMaxParticleCount( unsigned maxParticleCount )
{
	maxParticleCount = std::min( g_maxParticleCountCap, maxParticleCount );
	DestroyBuffers();
	m_maxParticleCount = maxParticleCount;
	BuildBuffers();
}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function for getting the particle system's maxParticleCount. This is used
//   to setup a Blue property.
// Return Value:
//   The particle system's maxParticleCount.
// --------------------------------------------------------------------------------------
unsigned Tr2ParticleSystem::GetMaxParticleCount() const
{
	return m_maxParticleCount;
}


// --------------------------------------------------------------------------------------
// Description:
//   Implements IListNotify interface. Binds added particle constraints to the system.
// Arguments:
//   event - List event type
//   key - First element index (unused)
//   key2 - Second element index (unused)
//   value - Element value
//   theList - The list being modified
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::OnListModified(
	long event,
	ssize_t key,
	ssize_t key2,
	IRoot* value,
	const IList* theList )
{
	if( theList == &m_constraints )
	{
		// Bind added constaint to this system
		if( ( event & BELIST_EVENTMASK ) == BELIST_INSERTED )
		{
			if( value )
			{
				ITr2GenericParticleConstraint* constraint = NULL;
				if( value->QueryInterface( BlueInterfaceIID<ITr2GenericParticleConstraint>(), (void**)&constraint ) )
				{
					constraint->Bind( this );
					constraint->Unlock();
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource method. Releases vertex buffer and declaration.
// Arguments:
//   s - Type of video memory to release
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::ReleaseResources( TriStorage s )
{
	m_declaration = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_vertexBuffer = Tr2BufferAL();
	m_bufferDirty = true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource method. Recreates vertex buffer and declaration.
// --------------------------------------------------------------------------------------
bool Tr2ParticleSystem::OnPrepareResources()
{
	RebuildDeclaration();
	CreateVertexBuffer();
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Create a vertex buffer for particle data if needed.
// Return value:
//   true If the buffer was created or is not needed
//   false On error
// --------------------------------------------------------------------------------------
bool Tr2ParticleSystem::CreateVertexBuffer()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( m_maxParticleCount > 0 && m_vertexSizes[Tr2ParticleElementData::GPU] > 0 )
	{
		CR_RETURN_VAL( m_vertexBuffer.Create(
						   m_vertexSizes[Tr2ParticleElementData::GPU] * sizeof( float ),
						   m_maxParticleCount,
						   Tr2GpuUsage::VERTEX_BUFFER,
						   Tr2CpuUsage::WRITE_OFTEN,
						   nullptr,
						   renderContext ),
					   false );
		m_bufferDirty = true;
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface. Query if instance data is ready for rendering.
// Return Value:
//   true If vertex declaration is initialized
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2ParticleSystem::IsInstanceDataReady() const
{
	return m_declaration != Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
}

ITr2InstanceData::InstanceData Tr2ParticleSystem::GetInstanceData( unsigned int, float ) const
{
	return { m_vertexBuffer, 0, uint32_t( m_vertexSizes[Tr2ParticleElementData::GPU] * sizeof( float ) ), m_aliveCount };
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface. Returns vertex declaration handle.
// Arguments:
//   bufferIndex - Instance buffer index (unused)
// Return Value:
//   Vertex declaration handle
// --------------------------------------------------------------------------------------
unsigned int Tr2ParticleSystem::GetInstanceBufferVertexDeclaration( unsigned int bufferIndex ) const
{
	return m_declaration;
}

CcpMath::AxisAlignedBox Tr2ParticleSystem::GetInstanceBufferBoundingBox( unsigned int bufferIndex ) const
{
	if( m_aliveCount > 0 )
	{
		return CcpMath::AxisAlignedBox( m_AabbMin, m_AabbMax );
	}
	return CcpMath::AxisAlignedBox();
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GpuBuffer interface. Returns GPU buffer with particle data.
// Arguments:
//   bufferIndex - instance buffer index
// Return Value:
//   GPU buffer containing instance data
// --------------------------------------------------------------------------------------
Tr2BufferAL* Tr2ParticleSystem::GetGpuBuffer( unsigned bufferIndex )
{
	return &m_vertexBuffer;
}

// --------------------------------------------------------------------------------------
// Description:
//   Clears CPU-side particle data buffers and all live particles.
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::DestroyBuffers()
{
	if( !m_buffers[0] )
	{
		return;
	}

	if( m_indexes )
	{
		CCP_DELETE[] m_indexes;
		m_indexes = nullptr;
	}

	if( m_buffers[0] )
	{
		for( unsigned i = 0; i < Tr2ParticleElementData::COUNT; ++i )
		{
			CCP_ALIGNED_FREE( m_buffers[i] );
			m_buffers[i] = nullptr;
		}
	}

	m_vertexBuffer = Tr2BufferAL();

	m_aliveCount = 0;
	s_updatedParticleCount = m_aliveCount;

	s_totalParticleCount -= m_maxParticleCount;
	CCP_STATS_SET( statTotalParticleCount, s_totalParticleCount );
}

// --------------------------------------------------------------------------------------
// Description:
//   Creates CPU-side particle data buffers.
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::BuildBuffers()
{
	if( !m_maxParticleCount )
	{
		return;
	}

	if( !m_isValid )
	{
		return;
	}

	int totalParticleCount = s_totalParticleCount + m_maxParticleCount;

	// only dynamic system are being capped!
	if( IsDynamic() )
	{
		if( totalParticleCount >= int( s_maxParticleCount ) )
		{
			m_maxParticleCount = 0;
			return;
		}
	}

	if( m_requiresSorting )
	{
		m_indexes = CCP_NEW( "Tr2ParticleSystem::m_indexes" ) unsigned[m_maxParticleCount];
	}

	for( unsigned i = 0; i < Tr2ParticleElementData::COUNT; ++i )
	{
		m_buffers[i] = (float*)CCP_ALIGNED_MALLOC(
			"Tr2ParticleSystem::m_buffers",
			sizeof( float ) * m_maxParticleCount * m_vertexSizes[i],
			16 );
	}
	if( m_vertexSizes[Tr2ParticleElementData::GPU] > 0 && Tr2Renderer::IsResourceCreationAllowed() )
	{
		CreateVertexBuffer();
	}
	s_totalParticleCount = totalParticleCount;
	CCP_STATS_SET( statTotalParticleCount, s_totalParticleCount );
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns particle element stream (start and stride) for a given element type.
// Arguments:
//   type - Particle element type
//   element - (out) Element data start (for the first particle)
//   stride - (out) Stride (spacing) between consecutive particle data
// Return Value:
//   true If element is found
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2ParticleSystem::GetElementStream( Tr2ParticleElementDeclarationName::Type type, float*& element, unsigned& stride )
{
	if( m_semanticElements[type].m_offset == -1 )
	{
		element = nullptr;
		stride = 0;
		return false;
	}
	element = m_buffers[m_semanticElements[type].m_bufferType] + m_semanticElements[type].m_offset;
	stride = m_vertexSizes[m_semanticElements[type].m_bufferType];
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Check if the system has requested particle element.
// Arguments:
//   type - Particle element type
// Return Value:
//   true If element is found in the system
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2ParticleSystem::HasElement( Tr2ParticleElementDeclarationName::Type type ) const
{
	return m_semanticElements[type].m_offset != -1;
}

// --------------------------------------------------------------------------------------
// Description:
//   Rebase internal time variables in case of sim time rebase.
// Arguments:
//   oldTime - Previous current sim time
//   newTime - The new sim time
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::OnSimClockRebase( Be::Time oldTime, Be::Time newTime )
{
	m_lastUpdate += newTime - oldTime;
}

// --------------------------------------------------------------------------------------
// Description:
//   Per-frame update method for the system. Updates particle system state.
// Arguments:
//   globalArguments - Child emitter update arguments
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::Update( const ITr2GenericEmitter::UpdateArguments& globalArguments )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	auto arguments = globalArguments;
	arguments.parentTransform = m_worldTransform;


	if( m_previousDataOutdated )
	{
		float* gpuBuffer = m_buffers[Tr2ParticleElementData::GPU];
		uint32_t gpuDataSize = m_vertexSizes[Tr2ParticleElementData::GPU];
		uint32_t gpuDataHalfSize = gpuDataSize >> 1;

		for( uint32_t i = 0; i < m_aliveCount; ++i )
		{
			//Save the current data so we have the previous frame's data for motion vectors
			//This kinda has to be done regardless of if we tick or not, to make sure it's always up to date for rendering...
			std::copy(
				gpuBuffer + i * gpuDataSize,
				gpuBuffer + i * gpuDataSize + gpuDataHalfSize,
				gpuBuffer + i * gpuDataSize + gpuDataHalfSize );
		}

		m_previousDataOutdated = false;
	}

	// if we're offscreen or very small, we can tick less frequently
	if( m_updatePeriod > 1 )
	{
		++m_updatePeriodClock;
		m_updatePeriodClock %= m_updatePeriod;
		if( m_updatePeriodClock != 0 )
		{
			return;
		}
	}

	if( m_lastUpdate == 0 )
	{
		m_lastUpdate = arguments.time;
	}
	float dt = std::min( TimeAsFloat( arguments.time - m_lastUpdate ), 1.0f / 3.0f );
	m_lastUpdate = arguments.time;

	//include considerable hysteresis in toggling sorting.
	//Things to consider:
	// frametime breakpoints as global constant/setting/per-system?
	// re-enable a limited number of systems per frame?
	if( dt > 0.035f && m_sortingAllowed )
	{
		m_sortingAllowed = false;
	}
	else if( !m_sortingAllowed && dt < 0.02f )
	{
		m_sortingAllowed = true;
	}

	UpdateSimulation( arguments, dt );
}

// --------------------------------------------------------------------------------------
// Description:
//   Updates per-particle data (age, positions, etc.), removes dead particles, emits new
//   particles with "emit during lifetime" and  "emit on death" emitters. This function
//   can be called asyncronously.
// Arguments:
//   arguments - Child emitter update arguments
//   dt - simulation time interval
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::UpdateSimulation( const ITr2GenericEmitter::UpdateArguments& arguments, float dt )
{
	// Update lifetime and remove dead particles
	if( m_applyAging && HasElement( Tr2ParticleElementDeclarationName::LIFETIME ) )
	{
		float* lifetime = nullptr;
		unsigned lifetimeStride;
		GetElementStream( Tr2ParticleElementDeclarationName::LIFETIME, lifetime, lifetimeStride );

		float* position = nullptr;
		unsigned positionStride;
		GetElementStream( Tr2ParticleElementDeclarationName::POSITION, position, positionStride );

		float* velocity = nullptr;
		unsigned velocityStride;
		GetElementStream( Tr2ParticleElementDeclarationName::VELOCITY, velocity, velocityStride );

		if( m_insertDeleteMutex )
		{
			m_insertDeleteMutex->Acquire();
		}

		for( unsigned i = 0; i < m_aliveCount; ++i )
		{
			lifetime[0] += dt / lifetime[1];
			if( lifetime[0] >= 1.0f )
			{
				if( m_emissionOnDeathEmitter )
				{
					m_emissionOnDeathEmitter->SpawnParticles( arguments,
															  reinterpret_cast<Vector3*>( position ),
															  reinterpret_cast<Vector3*>( velocity ) );
				}

				m_aliveCount -= 1;
				if( i < m_aliveCount )
				{
					for( unsigned j = 0; j < Tr2ParticleElementData::COUNT; ++j )
					{
						std::copy( m_buffers[j] + m_aliveCount * m_vertexSizes[j],
								   m_buffers[j] + ( m_aliveCount + 1 ) * m_vertexSizes[j],
								   m_buffers[j] + i * m_vertexSizes[j] );
					}
				}

				--i;
			}
			else
			{
				lifetime += lifetimeStride;
				if( position )
				{
					position += positionStride;
				}
				if( velocity )
				{
					velocity += velocityStride;
				}
			}
		}
		m_bufferDirty = true;

		if( m_insertDeleteMutex )
		{
			m_insertDeleteMutex->Release();
		}

		m_previousDataOutdated = true;
	}

	s_aliveParticleCount += m_aliveCount;

	// Calculate forces and update position
	if( m_updateSimulation &&
		HasElement( Tr2ParticleElementDeclarationName::POSITION ) &&
		HasElement( Tr2ParticleElementDeclarationName::VELOCITY ) )
	{
		bool hasForces = m_applyForce && !m_forces.empty();

		for( auto it = m_forces.begin(); it != m_forces.end(); ++it )
		{
			( *it )->Update( dt );
		}

		float* positionStart = nullptr;
		unsigned positionStride;
		GetElementStream( Tr2ParticleElementDeclarationName::POSITION, positionStart, positionStride );

		float* velocityStart = nullptr;
		unsigned velocityStride;
		GetElementStream( Tr2ParticleElementDeclarationName::VELOCITY, velocityStart, velocityStride );

		float* massStart = nullptr;
		unsigned massStride;
		GetElementStream( Tr2ParticleElementDeclarationName::MASS, massStart, massStride );

		Tr2ParallelFor(
			Tr2BlockedRange<size_t>( 0, m_aliveCount, 100 ),
			[=]( const Tr2BlockedRange<size_t>& range ) -> void {
				float* position = positionStart + range.begin() * positionStride;
				float* velocity = velocityStart + range.begin() * velocityStride;
				float* mass = nullptr;
				if( massStart )
				{
					mass = massStart + range.begin() * massStride;
				}
				for( auto i = range.begin(); i < range.end(); ++i )
				{
					XMVECTOR particlePosition = XMLoadFloat4A( reinterpret_cast<XMFLOAT4A*>( position ) );
					XMVECTOR particleVelocity = XMLoadFloat4A( reinterpret_cast<XMFLOAT4A*>( velocity ) );
					if( hasForces )
					{
						float particleMass;
						if( mass )
						{
							particleMass = *mass;
							mass += massStride;
						}
						else
						{
							particleMass = 1.0f;
						}
						XMVECTOR force = XMVectorReplicate( 0.0f );
						for( auto it = m_forces.begin(); it != m_forces.end(); ++it )
						{
							force = XMVectorAdd( ( *it )->GetForce( particlePosition, particleVelocity, dt, particleMass ), force );
						}
						if( mass )
						{
							force = XMVectorScale( force, 1.f / particleMass );
						}
						XMStoreFloat4A( reinterpret_cast<XMFLOAT4A*>( velocity ),
										XMVectorAdd( particleVelocity, XMVectorScale( force, dt ) ) );
					}

					XMStoreFloat4A(
						reinterpret_cast<XMFLOAT4A*>( position ),
						XMVectorAdd(
							XMLoadFloat4A( reinterpret_cast<XMFLOAT4A*>( position ) ),
							XMVectorScale( XMLoadFloat4A( reinterpret_cast<XMFLOAT4A*>( velocity ) ), dt ) ) );


					if( m_emissionWhileAliveEmitter != nullptr )
					{
						m_emissionWhileAliveEmitter->SpawnParticles(
							arguments,
							reinterpret_cast<Vector3*>( &particlePosition ),
							reinterpret_cast<Vector3*>( position ),
							reinterpret_cast<Vector3*>( &particleVelocity ),
							reinterpret_cast<Vector3*>( velocity ),
							dt );
					}

					position += positionStride;
					velocity += velocityStride;
				}
			} );

		m_bufferDirty = true;
		s_updatedParticleCount = m_aliveCount;

		m_previousDataOutdated = true;
	}
	// Update contraints
	if( m_updateSimulation && !m_constraints.empty() )
	{
		for( auto constraint = m_constraints.begin(); constraint != m_constraints.end(); ++constraint )
		{
			( *constraint )->ApplyConstraint( arguments, m_buffers, m_vertexSizes, m_aliveCount, dt );
		}
		m_bufferDirty = true;
		m_previousDataOutdated = true;
	}

	// Update bounding box
	if( m_bufferDirty )
	{
		if( m_aliveCount > 0 && HasElement( Tr2ParticleElementDeclarationName::POSITION ) )
		{
			float* position = nullptr;
			unsigned positionStride;
			GetElementStream( Tr2ParticleElementDeclarationName::POSITION, position, positionStride );

			XMVECTOR aabbMin = XMVectorReplicate( FLT_MAX );
			XMVECTOR aabbMax = XMVectorReplicate( -FLT_MAX );
			for( unsigned i = 0; i < m_aliveCount; ++i )
			{
				aabbMin = XMVectorMin( aabbMin, XMLoadFloat4A( reinterpret_cast<XMFLOAT4A*>( position ) ) );
				aabbMax = XMVectorMax( aabbMax, XMLoadFloat4A( reinterpret_cast<XMFLOAT4A*>( position ) ) );
				position += positionStride;
			}

			m_AabbMin = aabbMin;
			m_AabbMax = aabbMax;
		}
		else
		{
			m_AabbMin = Vector3( 0.f, 0.f, 0.f );
			m_AabbMax = Vector3( 0.f, 0.f, 0.f );
		}
	}

	// Emit particles with m_emissionWhileAliveEmitter
	// If the particles have both position and velocity streams, this will occur during update
	// and provide additional information to the m_emissionWhileAliveEmitter object
	// (but only if m_updateSimulation is true)
	const bool positionAndVelocity = HasElement( Tr2ParticleElementDeclarationName::POSITION ) &&
		HasElement( Tr2ParticleElementDeclarationName::VELOCITY );
	if( m_emissionWhileAliveEmitter && !( positionAndVelocity && m_updateSimulation ) )
	{
		float* positionStart = nullptr;
		unsigned positionStride;
		GetElementStream( Tr2ParticleElementDeclarationName::POSITION, positionStart, positionStride );

		float* velocityStart = nullptr;
		unsigned velocityStride;
		GetElementStream( Tr2ParticleElementDeclarationName::VELOCITY, velocityStart, velocityStride );

		Tr2ParallelFor(
			Tr2BlockedRange<size_t>( 0, m_aliveCount, 100 ),
			[=]( const Tr2BlockedRange<size_t>& range ) -> void {
				float* position = nullptr;
				if( positionStart )
				{
					position = positionStart + range.begin() * positionStride;
				}
				float* velocity = nullptr;
				if( velocityStart )
				{
					velocity = velocityStart + range.begin() * velocityStride;
				}
				for( auto i = range.begin(); i < range.end(); ++i )
				{
					if( m_emissionWhileAliveEmitter != nullptr )
					{
						m_emissionWhileAliveEmitter->SpawnParticles(
							arguments,
							reinterpret_cast<Vector3*>( position ),
							reinterpret_cast<Vector3*>( velocity ),
							dt );
					}
					position += positionStride;
					if( velocity )
					{
						velocity += velocityStride;
					}
				}
			} );

		m_previousDataOutdated = true;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Compares two particles for depth-sorting.
// Arguments:
//   particle1 - First particle index
//   particle2 - Second particle index
// Return Value:
//   true If particle2 is closer to camera
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2ParticleSystem::CompareParticles( unsigned particle1, unsigned particle2 ) const
{
	XMVECTOR referencePosition = *m_sortingReferencePoint;

	const float* buffer = m_buffers[m_semanticElements[Tr2ParticleElementDeclarationName::POSITION].m_bufferType];

	unsigned offset = m_semanticElements[Tr2ParticleElementDeclarationName::POSITION].m_offset;
	unsigned vertexSize = m_vertexSizes[m_semanticElements[Tr2ParticleElementDeclarationName::POSITION].m_bufferType];

	XMVECTOR position1 = XMLoadFloat4A( reinterpret_cast<const XMFLOAT4A*>( buffer + offset + vertexSize * particle1 ) );
	position1 = XMVectorSubtract( position1, referencePosition );
	XMVECTOR position2 = XMLoadFloat4A( reinterpret_cast<const XMFLOAT4A*>( buffer + offset + vertexSize * particle2 ) );
	position2 = XMVectorSubtract( position2, referencePosition );
	XMVECTOR distance1 = XMVector3LengthSq( position1 );
	XMVECTOR distance2 = XMVector3LengthSq( position2 );

	return XMVector3Less( distance2, distance1 ) != 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Per-frame update method for all created Tr2ParticleSystem objects. Calls Update
//   method of each system asyncronously.
// Arguments:
//   arguments - Child emitter update arguments
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::UpdateAllSystems( const ITr2GenericEmitter::UpdateArguments& arguments )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	static Be::Time previousUpdateTime = -1;

	s_aliveParticleCount = 0;
	s_updatedParticleCount = 0;

	if( previousUpdateTime != -1 && previousUpdateTime == arguments.time )
	{
		return;
	}

	std::set<Tr2ParticleSystem*>& allSystems = GetAllSystems();

	Tr2ParallelDo( allSystems.begin(),
				   allSystems.end(),
				   [=]( Tr2ParticleSystem* system ) { system->Update( arguments ); } );

	CCP_STATS_SET( statAliveParticleCount, s_aliveParticleCount );
	CCP_STATS_SET( statUpdatedParticleCount, s_updatedParticleCount );
}

// --------------------------------------------------------------------------------------
// Description:
//   Maintains a set of all created Tr2ParticleSystem objects.
// Return Value:
//   Set of all created Tr2ParticleSystem objects
// --------------------------------------------------------------------------------------
std::set<Tr2ParticleSystem*>& Tr2ParticleSystem::GetAllSystems()
{
	static std::set<Tr2ParticleSystem*> s_allSystems;
	return s_allSystems;
}

// --------------------------------------------------------------------------------------
// Description:
//   Rebuilds D3D vertex declaration.
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::RebuildDeclaration()
{
	Tr2VertexDefinition vd;

	for( auto it = m_elementMap.begin(); it != m_elementMap.end(); ++it )
	{
		//Skip things we don't want to upload to the GPU
		if( it->second.m_bufferType != Tr2ParticleElementData::GPU )
		{
			continue;
		}

		Tr2VertexDefinition::Item item;
		item.m_stream = 0;
		item.m_offset = it->second.m_offset * sizeof( float );
		item.m_dataType = static_cast<Tr2VertexDefinition::DataType>( vd.DT_FLOAT32 + ( ( it->second.m_dimension - 1 ) << vd.DT_SIZE_OFFSET ) );
		item.m_usage = it->first.GetD3DUsage();

		item.m_usageIndex = it->first.m_type == Tr2ParticleElementDeclarationName::CUSTOM ? it->second.m_usageIndex : 0;

		vd.m_items.push_back( item );
		vd.m_nextOffset[0] = std::max( vd.m_nextOffset[0], item.m_offset + vd.GetDataTypeSizeInBytes( item.m_dataType ) );

		/*
		Since we need to upload both the current and previous frame's data,
		we duplicate the vertex attributes with an offset. This allows the shader to access
		both the current data and the previous data simultaneously.

		Since CUSTOM vertex attributes can't be updated, we only need to duplicate the non-CUSTOM
		attributes. However, for simplicity, all the previous frame particle GPU data is tracked,
		so this could be added in the future if necessary.
		*/

		if( it->first.m_type != Tr2ParticleElementDeclarationName::CUSTOM )
		{
			item.m_usageIndex = 1;
			item.m_offset += m_vertexSizes[Tr2ParticleElementData::GPU] * sizeof( float ) / 2;

			vd.m_items.push_back( item );
			vd.m_nextOffset[0] = std::max( vd.m_nextOffset[0], item.m_offset + vd.GetDataTypeSizeInBytes( item.m_dataType ) );
		}
	}

	m_declaration = Tr2EffectStateManager::GetVertexDeclarationHandle( vd );
}

// --------------------------------------------------------------------------------------
// Description:
//   Rearranges particle element declaration so that POSITION and VELOCITY offsets are
//   16-byte aligned (for faster loading into SSE registers).
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::EnsureAligned()
{
	// Move POSITION and VELOCITY elements into the beginning of their buffers. If they
	// are in the same buffer, make the following arrangement:
	// POSITION pad VELOCITY all_other_stuff
	// Here pad is 4-byte padding to align VELOCITY.
	Tr2ParticleElementDeclarationName::Type position = Tr2ParticleElementDeclarationName::POSITION;
	Tr2ParticleElementDeclarationName::Type velocity = Tr2ParticleElementDeclarationName::VELOCITY;
	if( m_semanticElements[position].m_offset != -1 )
	{
		ShiftOffsets(
			m_semanticElements[position].m_bufferType,
			m_semanticElements[position].m_offset,
			-int( m_semanticElements[position].m_dimension ) );
		ShiftOffsets(
			m_semanticElements[position].m_bufferType,
			0,
			4 );
		m_semanticElements[position].m_offset = 0;
		m_elementMap[position].m_offset = 0;
		// We added padding, so increase vertex size
		m_vertexSizes[m_semanticElements[position].m_bufferType]++;
	}
	if( m_semanticElements[velocity].m_offset != -1 )
	{
		ShiftOffsets(
			m_semanticElements[velocity].m_bufferType,
			m_semanticElements[velocity].m_offset,
			-int( m_semanticElements[velocity].m_dimension ) );
		if( m_semanticElements[position].m_offset != -1 &&
			m_semanticElements[velocity].m_bufferType == m_semanticElements[position].m_bufferType )
		{
			ShiftOffsets(
				m_semanticElements[velocity].m_bufferType,
				4,
				4 );
			m_semanticElements[velocity].m_offset = 4;
			m_elementMap[velocity].m_offset = 4;
		}
		else
		{
			ShiftOffsets(
				m_semanticElements[velocity].m_bufferType,
				0,
				4 );
			m_semanticElements[velocity].m_offset = 0;
			m_elementMap[velocity].m_offset = 0;
		}
		// We added padding, so increase vertex size
		m_vertexSizes[m_semanticElements[velocity].m_bufferType]++;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Shifts particle element offsets in a given buffer (used in EnsureAligned method).
//   All offsets larger than "start" are shifted by value "shift".
// Arguments:
//   bufferType - Buffer type where offsets need to be shifted
//   start - Offset start
//   shift - Offset shift
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::ShiftOffsets( Tr2ParticleElementData::BufferType bufferType,
									  unsigned start,
									  int shift )
{
	for( auto it = m_elementMap.begin(); it != m_elementMap.end(); ++it )
	{
		if( it->second.m_bufferType == bufferType &&
			it->second.m_offset != -1 &&
			it->second.m_offset >= start )
		{
			it->second.m_offset += shift;
			if( it->first.m_type != Tr2ParticleElementDeclarationName::CUSTOM )
			{
				m_semanticElements[it->first.m_type].m_offset = it->second.m_offset;
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Called before particle system is used for rendering. Optionally sorts particles
//   and updates vertex buffer if needed.
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::UpdateViewDependentData( const TriFrustum* frustum, const Matrix& worldTransform )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_shouldSortVisible = false;

	m_worldTransform = worldTransform;

	if( !m_bufferDirty && !m_requiresSorting )
	{
		return;
	}

	if( !m_vertexBuffer.IsValid() )
	{
		return;
	}

	m_shouldSortVisible = true;
	m_updatePeriod = 1;
	if( frustum )
	{
		Vector3 minB, maxB;
		//If we were about to sort, first check that this system is visible and large enough to warrant sorting
		if( GetBoundingBox( minB, maxB ) )
		{
			Vector3 centre( ( maxB + minB ) * 0.5f );
			const Vector3 extent( ( maxB - minB ) * 0.5f );

			//use the contained sphere rather than circumscribing sphere, just to be a little conservative
			float radius = std::max( std::abs( extent.x ), std::max( std::abs( extent.y ), std::abs( extent.z ) ) );
			radius *= Length( m_worldTransform.GetX() );

			centre = Vector3( XMVector3TransformCoord( centre, m_worldTransform ) );
			const Vector4 boundingSphere = Vector4( centre.x, centre.y, centre.z, radius );

			if( !frustum->IsSphereVisible( &boundingSphere ) )
			{
				m_shouldSortVisible = false;
				// update every fourth frame
				m_updatePeriod = 4;
			}
		}
	}
}

void Tr2ParticleSystem::SortParticles()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( !m_bufferDirty && !m_requiresSorting )
	{
		return;
	}

	if( !m_vertexBuffer.IsValid() )
	{
		return;
	}

	XMMATRIX invWorldTransform = m_worldTransform;
	XMVECTOR determinant;
	invWorldTransform = XMMatrixInverse( &determinant, invWorldTransform );

	const float sortingPointEpsilon = 0.001f;
	XMVECTOR sortingReferencePoint = XMVector3TransformCoord( Tr2Renderer::GetViewPosition(), invWorldTransform );
	XMVECTOR lengthSq = XMVector3LengthSq( XMVectorSubtract( sortingReferencePoint, *m_sortingReferencePoint ) );
	if( !m_bufferDirty && XMVectorGetX( lengthSq ) < sortingPointEpsilon )
	{
		return;
	}
	*m_sortingReferencePoint = sortingReferencePoint;

	if( m_aliveCount > 0 )
	{
		if( m_shouldSortVisible && m_sortingAllowed && m_requiresSorting && HasElement( Tr2ParticleElementDeclarationName::POSITION ) )
		{
			for( unsigned i = 0; i < m_aliveCount; ++i )
			{
				m_indexes[i] = i;
			}

			Tr2ParallelSort(
				m_indexes,
				m_indexes + m_aliveCount,
				[=]( unsigned particle1, unsigned particle2 ) -> bool {
					return CompareParticles( particle1, particle2 );
				} );

			if( m_vertexBuffer.IsValid() )
			{
				float* data;
				CR_RETURN( m_vertexBuffer.MapForWriting( data, renderContext ) );
				ON_BLOCK_EXIT( [&] { m_vertexBuffer.UnmapForWriting( renderContext ); } );
				for( unsigned i = 0; i < m_aliveCount; ++i )
				{
					std::copy(
						m_buffers[Tr2ParticleElementData::GPU] + m_indexes[i] * m_vertexSizes[Tr2ParticleElementData::GPU],
						m_buffers[Tr2ParticleElementData::GPU] + ( m_indexes[i] + 1 ) * m_vertexSizes[Tr2ParticleElementData::GPU],
						data + i * m_vertexSizes[Tr2ParticleElementData::GPU] );
				}
			}
		}
		else if( m_vertexBuffer.IsValid() )
		{
			float* data;
			CR_RETURN( m_vertexBuffer.MapForWriting( data, renderContext ) );
			ON_BLOCK_EXIT( [&] { m_vertexBuffer.UnmapForWriting( renderContext ); } );
			std::copy(
				m_buffers[Tr2ParticleElementData::GPU],
				m_buffers[Tr2ParticleElementData::GPU] + m_aliveCount * m_vertexSizes[Tr2ParticleElementData::GPU],
				data );
		}
	}
	m_bufferDirty = false;
	m_shouldSortVisible = false;
}

void Tr2ParticleSystem::UpdateTransform( const Matrix& worldTransform )
{
	m_worldTransform = worldTransform;
}

// --------------------------------------------------------------------------------------
// Description:
//   Rebuilds internal particle element declaration based on m_elements list.
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::UpdateElementDeclaration()
{
	m_isValid = false;
	DestroyBuffers();
	m_declaration = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_elementMap.clear();
	for( unsigned i = 0; i < Tr2ParticleElementDeclarationName::CUSTOM; ++i )
	{
		m_semanticElements[i].m_offset = -1;
	}
	for( unsigned i = 0; i < Tr2ParticleElementData::COUNT; ++i )
	{
		CCP_ALIGNED_FREE( m_buffers[i] );
		m_buffers[i] = nullptr;
		m_vertexSizes[i] = 0;
	}

	m_aliveCount = 0;

	if( m_elements.empty() )
	{
		return;
	}

	// Check for duplicate semantics and usage indexes
	bool semantics[Tr2ParticleElementDeclarationName::CUSTOM];
	static const unsigned USAGE_INDEX_COUNT = 8;
	bool usages[USAGE_INDEX_COUNT];
	std::fill( std::begin( semantics ), std::end( semantics ), false );
	std::fill( std::begin( usages ), std::end( usages ), false );
	for( auto it = m_elements.begin(); it != m_elements.end(); ++it )
	{
		if( ( *it )->m_name.m_type != Tr2ParticleElementDeclarationName::CUSTOM )
		{
			if( semantics[( *it )->m_name.m_type] )
			{
				CCP_LOGERR( "Duplicate semantics %s in a particle system", ( *it )->GetName().c_str() );
				return;
			}
			semantics[( *it )->m_name.m_type] = true;
		}
		else if( ( *it )->m_usedByGPU )
		{
			if( ( *it )->m_usageIndex >= USAGE_INDEX_COUNT )
			{
				CCP_LOGERR(
					"Particle declaration element %s usage index %u is out of range (needs to be less than %u)",
					( *it )->GetName().c_str(),
					( *it )->m_usageIndex,
					USAGE_INDEX_COUNT );
				return;
			}
			if( usages[( *it )->m_usageIndex] )
			{
				CCP_LOGERR( "Duplicate usage index %u for particle declaration element %s", ( *it )->m_usageIndex, ( *it )->GetName().c_str() );
				return;
			}
			usages[( *it )->m_usageIndex] = true;
		}
	}

	// Update internal per-element data
	unsigned offsets[Tr2ParticleElementData::COUNT];
	std::fill( std::begin( offsets ), std::end( offsets ), 0 );
	for( auto it = m_elements.begin(); it != m_elements.end(); ++it )
	{
		Tr2ParticleElementData data;
		data.m_bufferType = ( *it )->m_usedByGPU ? Tr2ParticleElementData::GPU : Tr2ParticleElementData::CPU;
		data.m_dimension = ( *it )->GetSize();
		data.m_offset = offsets[data.m_bufferType];
		data.m_usageIndex = ( *it )->m_usageIndex;

		m_elementMap[( *it )->m_name] = data;
		if( ( *it )->m_name.m_type != Tr2ParticleElementDeclarationName::CUSTOM )
		{
			m_semanticElements[( *it )->m_name.m_type] = data;
		}

		offsets[data.m_bufferType] += data.m_dimension;
	}

	for( unsigned i = 0; i < Tr2ParticleElementData::COUNT; ++i )
	{
		m_vertexSizes[i] = offsets[i];
	}

	// Make buffers compatible with SSE
	EnsureAligned();
	for( unsigned i = 0; i < Tr2ParticleElementData::COUNT; ++i )
	{
		if( m_vertexSizes[i] % 4 != 0 )
		{
			m_vertexSizes[i] += 4 - m_vertexSizes[i] % 4;
		}
	}

	m_vertexSizes[Tr2ParticleElementData::GPU] *= 2; //Reserve space for previous frame data

	m_declarationHash++;

	m_isValid = true;
	BuildBuffers();

	OnPrepareResources();

	for( auto it = m_constraints.begin(); it != m_constraints.end(); ++it )
	{
		( *it )->Bind( this );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns internal built particle element declaration. Can be used by emitters and
//   constraints to bind to individual particle elements.
// Return Value:
//   Internal built particle element declaration
// --------------------------------------------------------------------------------------
const Tr2ParticleElementDataMap& Tr2ParticleSystem::GetElementDeclaration() const
{
	return m_elementMap;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns internal built particle element declaration hash (changes with every call to
//   UpdateElementDeclaration). Can be used by emitters to check if their binding is out
//   of date.
// Return Value:
//   Internal built particle element declaration hash
// --------------------------------------------------------------------------------------
unsigned Tr2ParticleSystem::GetElementDeclarationHash() const
{
	return m_declarationHash;
}

// --------------------------------------------------------------------------------------
// Description:
//   Inserts a new particle to the system.
// Arguments:
//   paticle - (out) New particle data: array ofTr2ParticleElementData::COUNT of pointers
//		to particle buffers.
// Return Value:
//   true If particle was inserted
//   false If the system is full
// --------------------------------------------------------------------------------------
bool Tr2ParticleSystem::InsertParticle( float** particle )
{
	if( m_insertDeleteMutex )
	{
		m_insertDeleteMutex->Acquire();
	}

	if( m_aliveCount < m_maxParticleCount )
	{
		for( unsigned i = 0; i < Tr2ParticleElementData::COUNT; ++i )
		{
			particle[i] = m_buffers[i] + m_aliveCount * m_vertexSizes[i];
		}
		m_aliveCount++;
		m_peakAliveCount = std::max( m_peakAliveCount, m_aliveCount );
		m_bufferDirty = true;
		return true;
	}
	if( m_insertDeleteMutex )
	{
		m_insertDeleteMutex->Release();
	}
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Should be called by emitters after sucessfull call to InsertParticle when all new
//   particle data is filled. Used for syncronization.
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::DoneInsertingParticle()
{
	if( m_insertDeleteMutex )
	{
		m_insertDeleteMutex->Release();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Query if particle system was successfully rebuilt with UpdateElementDeclaration
//   method.
// Return Value:
//   true If last UpdateElementDeclaration was successful.
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2ParticleSystem::IsValid() const
{
	return m_isValid;
}

// --------------------------------------------------------------------------------------
// Description:
//   Query if particle system is a dynamic system as opposite to a static system
// Return Value:
//   true if dynamic
//   false if static
// --------------------------------------------------------------------------------------
bool Tr2ParticleSystem::IsDynamic() const
{
	return m_updateSimulation;
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes all particles from the system.
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::ClearParticles()
{
	m_aliveCount = 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Saves current particle data into a CMF file.
// Arguments:
//   resPath - Resource path of the CMF file to save particle data to.
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::SaveToCMF( const char* resPath ) const
{
	if( !m_isValid )
	{
		return;
	}

	std::wstring resPathW = (const wchar_t*)CA2W( resPath );
	std::wstring filename = BePaths->ResolvePathForWritingW( resPathW );

#if WITH_GRANNY
	if( filename.size() >= 4 && filename.compare( filename.size() - 4, 4, L".gr2" ) == 0 )
	{
		return SaveToGranny( resPath );
	}
#endif

	cmf::MemoryAllocator allocator;
	cmf::BufferManager bufferAllocator( allocator );
	cmf::Span<cmf::VertexElement> cmfVertexDecl = allocator.AllocateSpan<cmf::VertexElement>( m_elementMap.size() );

	unsigned vertexSize = 0;
	std::map<Tr2ParticleElementDeclarationName, unsigned> offsets;

	Tr2VertexDefinition vd;
	for( auto i = m_elementMap.begin(); i != m_elementMap.end(); ++i )
	{
		static const unsigned dimensions[] = {
			Tr2VertexDefinition::DT_SIZE_1,
			Tr2VertexDefinition::DT_SIZE_2,
			Tr2VertexDefinition::DT_SIZE_3,
			Tr2VertexDefinition::DT_SIZE_4,
		};
		auto& item = vd.Add( static_cast<Tr2VertexDefinition::DataType>( vd.FLOAT32_1 | dimensions[i->second.m_dimension - 1] ), i->first.GetD3DUsage(), i->second.m_usageIndex, 0 );
		item.m_offset = vertexSize;

		offsets[i->first] = vertexSize;

		vertexSize += i->second.m_dimension;
	}
	if( !ConvertVertexDeclToCMF( vd, cmfVertexDecl, (uint32_t)cmfVertexDecl.size() ) )
	{
		return;
	}

	unsigned totalSize = 0;
	for( unsigned i = 0; i < Tr2ParticleElementData::COUNT; ++i )
	{
		totalSize += m_vertexSizes[i];
	}
	std::vector<float> vertices;
	vertices.resize( totalSize * m_aliveCount );
	if( m_aliveCount )
	{
		float* vertex = vertices.data();
		for( unsigned i = 0; i < m_aliveCount; ++i )
		{
			for( auto j = m_elementMap.begin(); j != m_elementMap.end(); ++j )
			{
				float* element = m_buffers[j->second.m_bufferType] +
					i * m_vertexSizes[j->second.m_bufferType] +
					j->second.m_offset;
				std::copy( element, element + j->second.m_dimension, vertex + offsets[j->first] );
			}
			vertex += vertexSize;
		}
	}

	uint32_t vertexBufferStride = vertexSize * sizeof( float );
	uint32_t vertexBufferSize = vertexBufferStride * m_aliveCount;

	cmf::Data cmfData;
	cmfData.meshes = allocator.AllocateSpan<cmf::Mesh>( 1 );

	cmf::Mesh& cmfMesh = cmfData.meshes[0] = {};
	cmfMesh.name = allocator.AllocateString( m_name );
	cmfMesh.decl = cmfVertexDecl;
	cmfMesh.lods = allocator.AllocateSpan<cmf::MeshLod>( 1 );
	cmfMesh.topology = cmf::MeshTopology::PointList;

	cmf::MeshLod& cmfMeshLod = cmfMesh.lods[0] = {};
	cmfMeshLod.vb = bufferAllocator.AddBuffer( vertices.data(), vertexBufferSize, vertexBufferStride );

	auto file = cmf::BuildFile( cmfData, bufferAllocator );

	FILE* outFile;
	auto err = fopen_s( &outFile, CW2A( filename.c_str() ), "wb" );

	if( err != 0 )
	{
		CCP_LOGERR( "Failed to open output file: %ls\n", filename.c_str() );
		return;
	}
	if( fwrite( file.data(), 1, file.size(), outFile ) != file.size() )
	{
		CCP_LOGERR( "Failed to write output file: %ls\n", filename.c_str() );
		fclose( outFile );
		return;
	}
	fclose( outFile );
}

#if WITH_GRANNY
// --------------------------------------------------------------------------------------
// Description:
//   Saves current particle data into a granny file.
// Arguments:
//   resPath - Resource path of the granny file to save particle data to.
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::SaveToGranny( const char* resPath ) const
{
	if( !m_isValid )
	{
		return;
	}

	std::wstring resPathW = (const wchar_t*)CA2W( resPath );
	std::wstring filename = BePaths->ResolvePathForWritingW( resPathW );

	if( filename.size() >= 4 && filename.compare( filename.size() - 4, 4, L".cmf" ) == 0 )
	{
		return SaveToCMF( resPath );
	}

	granny_data_type_definition* definition = CCP_NEW( "Tr2ParticleSystem::SaveToGranny" ) granny_data_type_definition[m_elementMap.size() + 1];
	unsigned index = 0;
	unsigned vertexSize = 0;
	std::map<Tr2ParticleElementDeclarationName, unsigned> offsets;
	for( auto i = m_elementMap.begin(); i != m_elementMap.end(); ++i )
	{
		Tr2VertexDefinition vd;
		static const unsigned dimensions[] = {
			Tr2VertexDefinition::DT_SIZE_1,
			Tr2VertexDefinition::DT_SIZE_2,
			Tr2VertexDefinition::DT_SIZE_3,
			Tr2VertexDefinition::DT_SIZE_4,
		};
		auto& item = vd.Add( static_cast<Tr2VertexDefinition::DataType>( vd.FLOAT32_1 | dimensions[i->second.m_dimension - 1] ), i->first.GetD3DUsage(), i->second.m_usageIndex, 0 );
		item.m_offset = vertexSize;

		if( !ConvertVertexDeclToGranny( vd, definition + index, 1 ) )
		{
			return;
		}

		offsets[i->first] = vertexSize;

		++index;
		vertexSize += i->second.m_dimension;
	}

	granny_vertex_data* vertexData = CCP_NEW( "Tr2ParticleSystem::SaveToGranny" ) granny_vertex_data;
	memset( vertexData, 0, sizeof( granny_vertex_data ) );
	vertexData->VertexType = definition;
	vertexData->VertexComponentNames = CCP_NEW( "Tr2ParticleSystem::SaveToGranny" ) const char * [m_elementMap.size()];
	for( unsigned i = 0; i < m_elementMap.size(); ++i )
	{
		vertexData->VertexComponentNames[i] = definition[i].Name;
	}
	vertexData->VertexComponentNameCount = (granny_int32)m_elementMap.size();

	unsigned totalSize = 0;
	for( unsigned i = 0; i < Tr2ParticleElementData::COUNT; ++i )
	{
		totalSize += m_vertexSizes[i];
	}
	if( m_aliveCount )
	{
		float* vertices = CCP_NEW( "Tr2ParticleSystem::SaveToGranny" ) float[totalSize * m_aliveCount];
		float* vertex = vertices;
		for( unsigned i = 0; i < m_aliveCount; ++i )
		{
			for( auto j = m_elementMap.begin(); j != m_elementMap.end(); ++j )
			{
				float* element = m_buffers[j->second.m_bufferType] +
					i * m_vertexSizes[j->second.m_bufferType] +
					j->second.m_offset;
				std::copy( element, element + j->second.m_dimension, vertex + offsets[j->first] );
			}
			vertex += vertexSize;
		}

		vertexData->Vertices = reinterpret_cast<granny_uint8*>( vertices );
	}
	vertexData->VertexCount = m_aliveCount;

	// We don't need index buffer, but TriGeometryRes will complain/crash if tries
	// to load granny without index buffer.
	granny_tri_topology* topology = CCP_NEW( "Tr2ParticleSystem::SaveToGranny" ) granny_tri_topology;
	memset( topology, 0, sizeof( granny_tri_topology ) );
	topology->Index16Count = 3;
	topology->Indices16 = CCP_NEW( "Tr2ParticleSystem::SaveToGranny" ) granny_uint16[3];
	topology->Indices16[0] = 0;
	topology->Indices16[1] = 0;
	topology->Indices16[2] = 0;

	granny_mesh* mesh = CCP_NEW( "Tr2ParticleSystem::SaveToGranny" ) granny_mesh;
	memset( mesh, 0, sizeof( granny_mesh ) );
	mesh->Name = m_name.c_str();
	mesh->PrimaryVertexData = vertexData;
	mesh->PrimaryTopology = topology;

	granny_file_info info;
	memset( &info, 0, sizeof( granny_file_info ) );

	info.MeshCount = 1;
	info.Meshes = &mesh;
	info.VertexDataCount = 1;
	info.VertexDatas = &vertexData;
	info.TriTopologyCount = 1;
	info.TriTopologies = &topology;

	granny_file_builder* builder = GrannyBeginFile(
		1,
		GrannyCurrentGRNStandardTag,
		GrannyGRNFileMV_ThisPlatform,
		GrannyGetTemporaryDirectory(),
		"prefix2" );
	granny_file_data_tree_writer* writer = GrannyBeginFileDataTreeWriting( GrannyFileInfoType, &info, 0, 0 );

	GrannyWriteDataTreeToFileBuilder( writer, builder );
	GrannyEndFileDataTreeWriting( writer );

	GrannyEndFile( builder, CW2A( filename.c_str() ) );

	CCP_DELETE mesh;
	CCP_DELETE[] topology->Indices16;
	CCP_DELETE topology;
	CCP_DELETE[] vertexData->Vertices;
	CCP_DELETE[] vertexData->VertexComponentNames;
	CCP_DELETE vertexData;
	CCP_DELETE[] definition;
}
#endif

// --------------------------------------------------------------------------------------
// Description:
//   Returns local bounding box for this system.
// Arguments:
//   minBounds - (out) Min bounds of local bounding box.
//   maxBounds - (out) Max bounds of local bounding box.
// Return value:
//   true If the system has valid bounding box
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2ParticleSystem::GetBoundingBox( Vector3& minBounds, Vector3& maxBounds ) const
{
	if( m_aliveCount > 0 )
	{
		minBounds = m_AabbMin;
		maxBounds = m_AabbMax;
		return true;
	}
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Rebinds all system constraints.
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::RebindConstraints()
{
	for( auto it = m_constraints.begin(); it != m_constraints.end(); ++it )
	{
		( *it )->Bind( this );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns size of one element in GPU buffer in bytes.
// Return value:
//   Size of one element in GPU buffer in bytes
// --------------------------------------------------------------------------------------
size_t Tr2ParticleSystem::GetGpuStride() const
{
	return m_vertexSizes[Tr2ParticleElementData::GPU] * sizeof( float );
}

// --------------------------------------------------------------------------------------
// Description:
//   Notifies particle system that it needs additional syncronization when inserting
//   particles. Normally called by emitters that are identified to be emitting particles
//   during particle system updates.
// --------------------------------------------------------------------------------------
void Tr2ParticleSystem::SetThreadSafeFlag()
{
	if( !m_insertDeleteMutex )
	{
		m_insertDeleteMutex = CCP_NEW( "Tr2ParticleSystem::m_insertDeleteMutex" ) CcpMutex( "Tr2ParticleSystem", "m_mutex", 1000 );
	}
}

void Tr2ParticleSystem::GetDebugOptions( Tr2DebugRendererOptions& options ) const
{
	options.insert( "Particle Systems" );
}

void Tr2ParticleSystem::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldTransform ) const
{
	if( !renderer.HasOption( this, "Particle Systems" ) )
	{
		return;
	}
	auto aabb = CcpMath::AxisAlignedBox( m_AabbMin, m_AabbMax );
	for( auto& force : m_forces )
	{
		force->RenderDebugInfo( renderer, worldTransform, aabb );
	}
	for( auto& force : m_constraints )
	{
		force->RenderDebugInfo( renderer, worldTransform, aabb );
	}
}
