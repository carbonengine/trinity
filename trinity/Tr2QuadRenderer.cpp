// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2QuadRenderer.h"
#include "TriRenderBatch.h"
#include <numeric>

using namespace Tr2RenderContextEnum;

CCP_STATS_DECLARE( instanceDataSize, "Trinity/Tr2QuadRenderer/instanceDataSize", true, CST_COUNTER_LOW, "Size of instance data per frame" );
CCP_STATS_DECLARE( instanceBufferSize, "Trinity/Tr2QuadRenderer/instanceBufferSize", true, CST_COUNTER_LOW, "Size of the instance buffer used by the pool" );

namespace
{

// Initial size of the instance buffer
const size_t BUFFER_INITIAL_SIZE = 4 * 1024 * 1024;

uint32_t Align( uint32_t offset, uint32_t alignment )
{
	return ( offset + alignment - 1 ) / alignment * alignment;
}

}

Tr2QuadRenderer::Tr2QuadRenderer( IRoot* ) :
	m_vertexBufferOffset( -1 ),
	m_buffer( "Tr2QuadRenderer::m_buffer", 1024, 128 ),
	m_effects( "Tr2QuadRenderer::m_effects" ),
	m_bufferSize( 0 ),
	m_lastInstanceDataSize( 0 ),
	m_bufferAlignment( 4 )
{
	// let the instance buffer grow to avoid buffer rewrites (helps AMD)
	m_vertexBuffer.SetSizeIncrement( 512 * 1024 );
}

Tr2QuadRenderer* Tr2QuadRenderer::Instance()
{
	static Tr2QuadRendererPtr instance;
	if( !instance )
	{
		instance.CreateInstance();
	}
	return instance;
}

// --------------------------------------------------------------------------------------
// Description:
//   Registers an effect with the renderer. After the effect is registered it can be
//   used to add and render sprites. If the effect with the same key is already registerd
//   the function does nothing.
// Arguments:
//   key - a unique key for the effect
//   batchType - render type the effect is going to be used with
//   instanceSize - size of instance data
//   quadCount - number of quads to render per instance
//   definition - vertex definition for instance data
//   effect - effect to use for rendering
// --------------------------------------------------------------------------------------
void Tr2QuadRenderer::RegisterEffect(
	EffectKey key,
	TriBatchType batchType,
	uint32_t instanceSize,
	uint32_t quadCount,
	const Tr2VertexDefinition& definition,
	Tr2Material* effect )
{
	auto found = m_effects.find( key );
	if( found != m_effects.end() )
	{
		return;
	}
	auto record = CCP_NEW( "Tr2QuadRenderer::EffectRecord" ) EffectRecord;
	record->effect = effect;
	record->batchType = batchType;
	record->instanceSize = instanceSize;
	record->count = 0;
	record->quadCount = quadCount;
	record->vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	record->definition = definition;
	m_effects[key] = record;
	m_bufferAlignment = std::lcm( m_bufferAlignment, instanceSize );
}

// --------------------------------------------------------------------------------------
// Description:
//   Needs to be called after all renderer batches were submitted and before new
//   instances are added.
// --------------------------------------------------------------------------------------
void Tr2QuadRenderer::DoneRendering( Tr2RenderContext& renderContext )
{
	m_vertexBuffer.DoneUsingData( renderContext );
	m_bufferSize = 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds quad instances for rendering. This function may be called from TBB threads.
// Arguments:
//   effectKey - a unique key for a previously registered effect
//   sprites - quad instance data
//   count - number of instances to add
// --------------------------------------------------------------------------------------
void Tr2QuadRenderer::AddQuads( EffectKey effectKey, const void* sprites, size_t count )
{
	auto found = m_effects.find( effectKey );
	if( found == m_effects.end() )
	{
		return;
	}
	auto record = found->second;
	auto& tls = record->combinable.local();
	auto size = count * record->instanceSize;
	if( tls.buffer.size() < tls.addedSize + size )
	{
		tls.buffer.resize(
			"Tr2InstancePool::buffer",
			std::max( tls.buffer.size() * 2, tls.buffer.size() + std::max( count, size_t( 512 ) ) * record->instanceSize ) );
	}
	memcpy( tls.buffer.get() + tls.addedSize, sprites, size );
	tls.addedSize += uint32_t( size );
	m_bufferSize += uint32_t( size );
}

// --------------------------------------------------------------------------------------
// Description:
//   Merges all per-thread buffers with added instance data into a single one.
// Return value:
//   Maximum number of quads per instance
// --------------------------------------------------------------------------------------
uint32_t Tr2QuadRenderer::MergeBuffers()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Add padding to accomodate alignment
	for( auto& jt : m_effects )
	{
		m_bufferSize += jt.second->instanceSize;
	}

	if( m_bufferSize > m_buffer.size() )
	{
		m_buffer.resize( "pool", m_bufferSize );
	}
	uint32_t offset = 0;
	uint32_t quadCount = 0;
	for( auto& jt : m_effects )
	{
		auto record = jt.second;
		offset = Align( offset, record->instanceSize );
		record->bufferOffset = offset;
		for( auto& it : record->combinable )
		{
			if( it.addedSize > 0 )
			{
				memcpy( m_buffer.get() + offset, it.buffer.get(), it.addedSize );
				offset += it.addedSize;
				it.addedSize = 0;
			}
		}
		record->count = ( offset - record->bufferOffset ) / record->instanceSize;
		if( record->count )
		{
			quadCount = std::max( quadCount, record->quadCount );
		}
	}

	return quadCount;
}

// --------------------------------------------------------------------------------------
// Description:
//   Updates instance data buffer.
// Arguments:
//   renderContext - current render context
// --------------------------------------------------------------------------------------
void Tr2QuadRenderer::UpdateInstanceBuffer( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_bufferSize && !m_vertexBuffer.IsValid() )
	{
		m_vertexBuffer.Create( BUFFER_INITIAL_SIZE );
	}

	if( FAILED( m_vertexBuffer.PutData( m_buffer.get(), m_bufferSize, m_bufferAlignment, m_vertexBufferOffset, renderContext ) ) )
	{
		m_vertexBufferOffset = -1;
		return;
	}
	else
	{
		CCP_STATS_ADD( instanceDataSize, m_bufferSize );
		CCP_STATS_ADD( instanceBufferSize, m_vertexBuffer.GetBufferSize() );
		m_lastInstanceDataSize = m_bufferSize;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Re-creates quad vertex and index buffers if needed.
// Arguments:
//   quadCount - maximum nuber of quads per instance for data added this frame
// --------------------------------------------------------------------------------------
void Tr2QuadRenderer::RecreateQuadBuffers( uint32_t quadCount )
{
	if( quadCount )
	{
		if( !m_quad.IsValid() || m_quad.GetSize() / sizeof( float ) < quadCount * 4 )
		{
			std::unique_ptr<float[]> quad( new float[quadCount * 4] );
			for( uint32_t i = 0; i < quadCount * 4; ++i )
			{
				quad[i] = float( i );
			}
			USE_MAIN_THREAD_RENDER_CONTEXT();
			m_quad.Create( sizeof( float ), 4 * quadCount, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, quad.get(), renderContext );
		}
		if( !m_quadIB.IsValid() || m_quadIB.GetDesc().count < quadCount * 6 )
		{
			uint16_t quad[] = { 0, 2, 1, 0, 3, 2 };
			std::unique_ptr<uint16_t[]> quadIB( new uint16_t[quadCount * 6] );
			for( uint32_t i = 0; i < quadCount; ++i )
			{
				for( uint32_t j = 0; j < 6; ++j )
				{
					quadIB[i * 6 + j] = uint16_t( i * 4 + quad[j] );
				}
			}
			USE_MAIN_THREAD_RENDER_CONTEXT();
			m_quadIB.Create( 2, quadCount * 6, Tr2GpuUsage::INDEX_BUFFER, Tr2CpuUsage::NONE, quadIB.get(), renderContext );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Needs to be called after all instance data was added to the renderer, but before
//   GetBatches call.
// Arguments:
//   renderContext - current render context
// --------------------------------------------------------------------------------------
void Tr2QuadRenderer::BeginRendering( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	uint32_t quadCount = MergeBuffers();
	UpdateInstanceBuffer( renderContext );
	RecreateQuadBuffers( quadCount );
	PrepareResources();
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource method. Destorys graphics resources.
// Arguments:
//   s - storage class for resources to destroy
// --------------------------------------------------------------------------------------
void Tr2QuadRenderer::ReleaseResources( TriStorage s )
{
	if( s & TRISTORAGE_MANAGEDMEMORY )
	{
		for( auto it = m_effects.begin(); it != m_effects.end(); ++it )
		{
			it->second->vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource method. Re-creates vertex declarations for registered
//   effects.
// Return value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2QuadRenderer::OnPrepareResources()
{
	for( auto it = m_effects.begin(); it != m_effects.end(); ++it )
	{
		if( it->second->vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			it->second->vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( it->second->definition );
		}
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds render batches for each effect with instance data.
// Arguments:
//   accumulator - batch accumulator
//   batchType - batch rendering mode
// --------------------------------------------------------------------------------------
void Tr2QuadRenderer::GetBatches( TriBatchType batchType, ITriRenderBatchAccumulator* accumulator )
{
	if( m_vertexBufferOffset == -1 || !m_quadIB.IsValid() )
	{
		return;
	}

	for( auto& it : m_effects )
	{
		auto& record = *it.second;
		if( record.count && record.batchType == batchType && record.vertexDeclHandle != Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			Tr2RenderBatch batch;
			batch.SetMaterial( record.effect );
			batch.SetGeometry( record.vertexDeclHandle, m_quad, 4, m_quadIB, m_quadIB.GetDesc().stride );
			batch.SetStreamSource( 1, m_vertexBuffer.GetBuffer(), record.instanceSize );
			batch.SetDrawIndexedInstanced( 6 * record.quadCount, record.count, 0, 0, ( m_vertexBufferOffset + record.bufferOffset ) / record.instanceSize );

			accumulator->Commit( batch );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the size of the instance buffer.
// --------------------------------------------------------------------------------------
uint32_t Tr2QuadRenderer::GetInstanceBufferSize() const
{
	return m_vertexBuffer.GetBufferSize();
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the size of the instance data added for rendering during last frame.
// --------------------------------------------------------------------------------------
uint32_t Tr2QuadRenderer::GetInstanceDataSize() const
{
	return m_lastInstanceDataSize;
}