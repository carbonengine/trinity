////////////////////////////////////////////////////////////
//
//    Created:   October 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "Tr2RuntimeInstanceData.h"
#include "Utilities/BoundingBox.h"
#include "Particle/Tr2ParticleSystem.h"

using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------------
// Description:
//   Tr2RuntimeInstanceData default constructor
// --------------------------------------------------------------------------------------
Tr2RuntimeInstanceData::Tr2RuntimeInstanceData( IRoot* lockobj )
	:m_count( 0 ),
	m_stride( 0 ),
	m_vertexDeclaration( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_updateBoundingBox( true ),
	m_aabbMin( 0.f, 0.f, 0.f ),
	m_aabbMax( 0.f, 0.f, 0.f )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2RuntimeInstanceData destructor
// --------------------------------------------------------------------------------------
Tr2RuntimeInstanceData::~Tr2RuntimeInstanceData()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from ITriDeviceResource interface. Invalidates vertex declaration.
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::ReleaseResources( TriStorage s )
{
	m_vertexDeclaration = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	if( m_vb.GetMemoryClass() & s )
	{
		m_vb.Destroy();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from ITriDeviceResource interface. Recreates instance data vertex buffer
//   and vertex declaration if needed.
// Return value:
//   true If GPU resources are created successfully
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2RuntimeInstanceData::OnPrepareResources()
{
	if( m_data && m_stride && m_count )
	{
		if( !m_vb.IsValid() )
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			if( FAILED( m_vb.Create( m_stride * m_count, USAGE_CPU_WRITE | USAGE_SHADER_RESOURCE, m_data.get(), renderContext ) ) )
			{
				return false;
			}
		}
	}
	if( m_vertexDeclaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION && !m_layout.m_items.empty() )
	{
		m_vertexDeclaration = Tr2EffectStateManager::GetVertexDeclarationHandle( m_layout );
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface. Query if instance data is available for 
//   rendering.
// Return value:
//   true If instance data is available
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2RuntimeInstanceData::IsInstanceDataReady() const
{
	return m_vertexDeclaration != Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface. Returns number of instance buffers.
// Return value:
//   1 always
// --------------------------------------------------------------------------------------
unsigned int Tr2RuntimeInstanceData::GetInstanceBufferCount() const
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
unsigned int Tr2RuntimeInstanceData::GetInstanceBufferVertexDeclaration( unsigned int bufferIndex ) const
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
//   Nmber of instances
// --------------------------------------------------------------------------------------
unsigned int Tr2RuntimeInstanceData::GetInstanceBufferVertexCount( unsigned int bufferIndex ) const
{
	return m_count;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2InstanceData interface. Returns vertex buffer with instance data.
// Arguments:
//   bufferIndex - (unused) instance buffer index
//   buffer - (out) vertex buffer containing instance data (can be null)
//   stride - (out) vertex stride for the vertex buffer
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::GetVertexBuffer( unsigned int bufferIndex, Tr2VertexBufferAL*& buffer, unsigned& stride )
{
	buffer = &m_vb;
	stride = m_stride;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GpuBuffer interface. Returns GPU buffer with particle data.
// Arguments:
//   bufferIndex - instance buffer index
// Return Value:
//   GPU buffer containing instance data
// --------------------------------------------------------------------------------------
Tr2GpuBufferAL* Tr2RuntimeInstanceData::GetGpuBuffer( unsigned bufferIndex )
{
	if( !m_uavBuffer.IsValid() && m_vb.IsValid() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		CR( m_uavBuffer.CreateVbView( m_vb, false, renderContext ) );
		{
			return nullptr;
		}
	}

	return &m_uavBuffer;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericEmitter interface. Does nothing as this emitter only emits 
//   particles on demand.
// Arguments:
//   time - (unused) current time
//   parentTransform - (unused) partent transform
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::Update( Be::Time time )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericEmitter interface. Does nothing as this emitter only emits 
//   particles on demand.
// Arguments:
//   position - (unused) parent position
//   velocity - (unused) parent velocity
//   rateModifier - (unused) emit rate modifier
//   parentTransform - (unused) partent transform
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::SpawnParticles( const Vector3* position, 
											 const Vector3* velocity, 
											 float rateModifier )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericEmitter interface. Does nothing as this emitter only emits 
//   particles on demand.
// Arguments:
//   positionStart - (unused) parent position at the start of the frame
//   positionEnd - (unused) parent position at the end of the frame
//   velocityStart - (unused) parent velocity at the start of the frame
//   velocityStart - (unused) parent velocity at the end of the frame
//   deltaTime - (unused) frame time
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::SpawnParticles( const Vector3 *positionStart, 
											 const Vector3 *positionEnd,
											 const Vector3 *velocityStart, 
											 const Vector3 *velocityEnd,
											 float deltaTime )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericEmitter. Notify the emitter that it's spawn functions are going 
//   to be called in multi-threaded scenario (during particle system update). Since this
//   "emitter" does not emit with SpawnParticles methods we don't need to do anything here.
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::SetThreadSafeFlag()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns number of instances.
// Return value:
//   Nmber of instances
// --------------------------------------------------------------------------------------
unsigned Tr2RuntimeInstanceData::GetCount() const
{
	return m_count;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets vertex layout for instance data. This invalidates all data.
// Arguments:
//   layout - new vertex layout
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::SetLayout( const Tr2VertexDefinition& layout )
{
	m_data.release();
	m_vb.Destroy();
	m_uavBuffer.Destroy();
	m_stride = 0;
	for( auto it = layout.m_items.begin(); it != layout.m_items.end(); ++it )
	{
		if( it->m_stream != 0 )
		{
			CCP_LOGERR( "Tr2PythonInstanceData: vertex layout needs to reference stream 0 only" );
			return;
		}
		m_stride = std::max( m_stride, it->m_offset + layout.GetDataTypeSizeInBytes( it->m_dataType ) );
	}
	m_layout = layout;
	m_vertexDeclaration = Tr2EffectStateManager::GetVertexDeclarationHandle( m_layout );
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns current vertex layout for instance data. 
// Return value:
//   Current vertex layout
// --------------------------------------------------------------------------------------
const Tr2VertexDefinition& Tr2RuntimeInstanceData::GetLayout() const
{
	return m_layout;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns current instance data (read-only). 
// Return value:
//   Current instance data
// --------------------------------------------------------------------------------------
const void* Tr2RuntimeInstanceData::GetData() const
{
	return m_data.get();
}

// --------------------------------------------------------------------------------------
// Description:
//   Allocates memory for instance data (if passed instance count is different from 
//   previous one) and returns it.
// Arguments:
//   count - Number of instances
// Return value:
//   Instance data buffer
// --------------------------------------------------------------------------------------
void* Tr2RuntimeInstanceData::GetData( unsigned count )
{
	if( count != m_count )
	{
		m_count = count;
		if( m_stride && m_count )
		{
			m_data.reset( new char[m_stride * m_count] );
			m_vb.Destroy();
			m_uavBuffer.Destroy();
		}
		else
		{
			m_data.reset();
			return nullptr;
		}
	}

	if( !m_data )
	{
		return nullptr;
	}
	
	return m_data.get();
}

// --------------------------------------------------------------------------------------
// Description:
//   Updates GPU vertsion of instance data. Should be called after data is modified.
// Arguments:
//   count - Number of instances
// Return value:
//   Instance data buffer
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::UpdateData()
{
	if( !m_data )
	{
		return;
	}
	if( m_vb.IsValid() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		char* data;
		if( SUCCEEDED( m_vb.Lock( data, Tr2RenderContextEnum::LOCK_WRITEONLY, renderContext ) ) )
		{
			memcpy( data, m_data.get(), m_count * m_stride );
			m_vb.Unlock( renderContext );
		}
	}
	if( m_uavBuffer.IsValid() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		char* data;
		if( SUCCEEDED( m_uavBuffer.Lock( 0, 0, (void**)&data, Tr2RenderContextEnum::LOCK_WRITEONLY, renderContext ) ) )
		{
			memcpy( data, m_data.get(), m_count * m_stride );
			m_uavBuffer.Unlock( renderContext );
		}
	}

	if( !m_vb.IsValid() && !m_uavBuffer.IsValid() )
	{
		PrepareResources();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Updates bounding box (if m_updateBoundingBox flag is set) after data is modified.
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::UpdateBoundingBox()
{
	if( m_updateBoundingBox )
	{
		unsigned positionOffset = -1;
		for( auto it = m_layout.m_items.begin(); it != m_layout.m_items.end(); ++it )
		{
			if( it->m_usage == Tr2VertexDefinition::POSITION && 
				it->m_usageIndex == 0 && 
				Tr2VertexDefinition::GetDataTypeSizeInMembers( it->m_dataType ) >= 3 )
			{
				positionOffset = it->m_offset;
				break;
			}
		}
		BoundingBoxInitialize( m_aabbMin, m_aabbMax );
		if( positionOffset != -1 )
		{
			for( unsigned i = 0; i < m_count; ++i )
			{
				const Vector3* pos = 
					reinterpret_cast<Vector3*>( m_data.get() + positionOffset + m_stride * i );
				BoundingBoxUpdate( m_aabbMin, m_aabbMax, *pos );
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Destroys instance data.
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::DestroyData()
{
	m_vb.Destroy();
	m_uavBuffer.Destroy();
	m_data.reset();
	m_count = 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns per-instance data size.
// Return value:
//   Per-instance data size
// --------------------------------------------------------------------------------------
unsigned Tr2RuntimeInstanceData::GetStride() const
{
	return m_stride;
}

namespace
{
// --------------------------------------------------------------------------------------
// Description:
//   Mapping between particle element declaration and D3D vertex declaration in particle
//   data vertex buffer.
// --------------------------------------------------------------------------------------
struct DeclarationMapping
{
	// Element offset in vertex buffer
	unsigned inOffset;
	// Buffer type for particle element
	Tr2ParticleElementData::BufferType buffer;
	// Element offset in the particle buffer
	unsigned offset;
	// Length/dimension of particle element
	unsigned length;
	// Is the element in vertex buffer type FLOAT16 (needs convertion to 32 bit float)
	bool isFloat16;
};
}

// --------------------------------------------------------------------------------------
// Description:
//   Spawns particles into the particle system.
// --------------------------------------------------------------------------------------
void Tr2RuntimeInstanceData::Spawn()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_particleSystem && m_particleSystem->IsValid() && 
		!m_layout.m_items.empty() && m_data )
	{
		// Validate geometry vertex declaration against particle system element declaration
		const Tr2ParticleElementDataMap &particleElements = m_particleSystem->GetElementDeclaration();
		std::vector<DeclarationMapping> geometryDeclarationMap;
		for( auto i = particleElements.begin(); i != particleElements.end(); ++i )
		{
			bool found = false;
			for( size_t j = 0; j != m_layout.m_items.size(); ++j )
			{
				unsigned usageIndex = 0;
				if( i->first.m_type == Tr2ParticleElementDeclarationName::CUSTOM )
				{
					usageIndex = i->second.m_usageIndex;
				}
				if( m_layout.m_items[j].m_usage == i->first.GetD3DUsage() && m_layout.m_items[j].m_usageIndex == usageIndex )
				{
					DeclarationMapping mapping;
					mapping.inOffset = m_layout.m_items[j].m_offset;
					mapping.buffer = i->second.m_bufferType;
					mapping.offset = i->second.m_offset;
					mapping.length = i->second.m_dimension;
					
					if( m_layout.m_items[j].m_dataType >= m_layout.FLOAT32_1 + int( i->second.m_dimension ) - 1 && 
						m_layout.m_items[j].m_dataType <= m_layout.FLOAT32_4 )
					{
						mapping.isFloat16 = false;
					}
					else if( ( m_layout.m_items[j].m_dataType == m_layout.FLOAT16_2 && i->second.m_dimension <= 2 ) || 
							 m_layout.m_items[j].m_dataType == m_layout.FLOAT16_4 )
					{
						CCP_LOGWARN( 
							"Particle elements \"%s\" has FLOAT16 type in Tr2RuntimeInstanceData \"%s\", this degrades emitter performance", 
							i->first.GetName().c_str(), 
							m_name.c_str() );
						mapping.isFloat16 = true;
					}
					else
					{
						CCP_LOGERR( "Incompatible type for particle elements \"%s\" in Tr2RuntimeInstanceData \"%s\"", 
									i->first.GetName().c_str(), 
									m_name.c_str() );
						return;
					}
					geometryDeclarationMap.push_back( mapping );
					found = true;
					break;
				}
			}
			if( !found )
			{
				CCP_LOGERR( "No data for particle elements \"%s\" in Tr2RuntimeInstanceData \"%s\"", 
							i->first.GetName().c_str(), 
							m_name.c_str() );
				return;
			}
		}

		m_particleSystem->ClearParticles();

		// Spawn particles
		char* data = m_data.get();
		for( unsigned i = 0; i < m_count; ++i )
		{
			float* particle[Tr2ParticleElementData::COUNT];
			if( m_particleSystem->InsertParticle( particle ) )
			{
				for( auto j = geometryDeclarationMap.begin(); j != geometryDeclarationMap.end(); ++j )
				{
					if( j->isFloat16 )
					{
						std::copy( 
							static_cast<D3DXFLOAT16*>( static_cast<void*>( data + j->inOffset ) ), 
							static_cast<D3DXFLOAT16*>( static_cast<void*>( data + j->inOffset ) ) + j->length, 
							particle[j->buffer] + j->offset );
					}
					else
					{
						std::copy( 
							static_cast<float*>( static_cast<void*>( data + j->inOffset ) ), 
							static_cast<float*>( static_cast<void*>( data + j->inOffset ) ) + j->length, 
							particle[j->buffer] + j->offset );
					}
				}
				m_particleSystem->DoneInsertingParticle();
			}
			data += m_stride;
		}
	}
}