////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//
#include "StdAfx.h"
#include "EveOccluder.h"

#include "Shader/Tr2Effect.h"

#include "EveUpdateContext.h"
#include "EveTransform.h"
#include "include/TriMath.h"

#include "Tr2GpuBuffer.h"


Tr2OcclusionBuffer::Tr2OcclusionBuffer()
{
	m_management.CreateInstance();
	m_management->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/Lensflares/OccluderManagement.fx" );

	m_buffer.CreateInstance();
	GlobalStore().RegisterVariable( "FlareOcclusionBuffer", m_buffer );
}

Tr2OcclusionBuffer::Offset Tr2OcclusionBuffer::AllocateOffset()
{
	if( m_free.empty() )
	{
		ResizeBuffer();
	}
	if( m_free.empty() )
	{
		return {};
	}
	auto offset = m_free.back();
	m_free.pop_back();
	auto handle = Offset( new uint32_t( offset ), &DestroyOffset );
	m_clear.push_back( offset );
	return handle;
}

void Tr2OcclusionBuffer::ProcessBuffer( Tr2RenderContext& renderContext )
{
	bool success = true;
	for( auto clear : m_clear )
	{
		GlobalStore().RegisterVariable( "OcclusionBufferOffset", *reinterpret_cast<float*>( &clear ) );
		success = Tr2Renderer::RunComputeShader( m_management, BlueSharedString( "Clear" ), 1, 1, 1, renderContext ) && success;
	}
	if( success )
	{
		m_clear.clear();
	}
	if( m_size > 0 )
	{
		Tr2Renderer::RunComputeShader( m_management, BlueSharedString( "CopyCounters" ), m_size / ELEMENT_SIZE, 1, 1, renderContext );
	}
}

Tr2OcclusionBuffer& Tr2OcclusionBuffer::GetInstance()
{
	static Tr2OcclusionBuffer buffer;
	return buffer;
}

uint32_t Tr2OcclusionBuffer::GetOccluderOffset( const Offset& offset, uint32_t index )
{
	return offset ? *offset + 5 + index * 2 : 0;
}

void Tr2OcclusionBuffer::DestroyOffset( uint32_t* offset )
{
	GetInstance().m_free.push_back( *offset );
	delete offset;
}

bool Tr2OcclusionBuffer::OnPrepareResources()
{
	return true;
}

void Tr2OcclusionBuffer::ReleaseResources( TriStorage s )
{
	if( ( s & TRISTORAGE_MANAGEDMEMORY ) != 0 )
	{
		m_clear.clear();
		for( uint32_t i = 0; i < m_size; i += ELEMENT_SIZE )
	{
			m_clear.push_back( i );
	}
	}
}

void Tr2OcclusionBuffer::ResizeBuffer()
{
	auto oldSize = m_size;
	if( m_size )
	{
		m_size *= 2;
	}
	else
	{
		const uint32_t INITIAL_SIZE = 4;
		m_size = INITIAL_SIZE * ELEMENT_SIZE;
	}
	Tr2BufferAL old;
	if( auto buffer = m_buffer->GetGpuBuffer( 0 ) )
	{
		old = *buffer;
	}
	m_buffer->Create( m_size, Tr2RenderContextEnum::PIXEL_FORMAT_R32_UINT, Tr2GpuBuffer::GPU_WRITABLE );
	
	if( old.IsValid() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		renderContext.CopySubBuffer( *m_buffer->GetGpuBuffer( 0 ), 0, old, 0, old.GetSize() );
	}

	for( auto i = oldSize; i < m_size; i += ELEMENT_SIZE )
	{
		m_free.push_back( i );
	}
}


// --------------------------------------------------------------------------------
// Description:
//   Initialize data members, create a batch accumulator, load a shader and
//   register this class as a device resource, so we can handle device resets
//   for the dx objects
// --------------------------------------------------------------------------------
EveOccluder::EveOccluder( IRoot* lockobj ) :
	PARENTLOCK( m_sprites ),
	m_display( true )
{
	// create batch accumulator for rendering occlusion sprites
	TriPoolAllocator* allocator = Tr2Renderer::GetPoolAllocator();
	m_batches = std::make_unique<TriRenderBatchAccumulator<EffectKeyGenerator>>( allocator );
}

// --------------------------------------------------------------------------------
// Description:
//   Performs two render calls with different states: with and without depth-test
//   to get the visibility. WARNING: a query might not return a value immediatly,
//   if it still "runs", don't do anything and wait until it finishes
// Arguments:
//   renderContext - current render context
//   frustum - the current view frustum of the current frame
//   transform - parent transform of the lensflare to turn this into a 2d sprite
// --------------------------------------------------------------------------------
void EveOccluder::RunQuery( Tr2RenderContext& renderContext, const TriFrustum& frustum, const Matrix& transform, uint32_t bufferOffset, float fogWeight )
{
	// visible?
	if( !m_display )
	{
		return;
	}

	GlobalStore().RegisterVariable( "OcclusionBufferOffset", *reinterpret_cast<float*>( &bufferOffset ) );
	GlobalStore().RegisterVariable( "OcclusionFogWeight", fogWeight );

	// prepare batches for rendering
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );
	// collect renderables from sprite, so we can pass the parent transform and EveTransform handles
	// all of the view update for us
	std::vector<ITr2Renderable*> renderables;
	EveUpdateContext dummyContext;
	for( auto& sprite : m_sprites )
	{
		sprite->UpdateSyncronous( dummyContext );
		sprite->UpdateVisibility( frustum, transform );
		sprite->GetRenderables( renderables, nullptr );
	}
	// collect batches from renderables (only from decal area, nothing else is important for
	// occlusion queries)
	for( auto& renderable : renderables )
	{
		Tr2PerObjectData* objectData = renderable->GetPerObjectData( m_batches.get() );
		renderable->GetBatches( m_batches.get(), TRIBATCHTYPE_OPAQUE, objectData );
	}
	m_batches->Finalize();

	renderContext.RenderBatches( m_batches.get() );
	// cleanup
	m_batches->Clear();
}
