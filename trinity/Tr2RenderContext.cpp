////////////////////////////////////////////////////////////
//
//    Created:   July 2013
//    Copyright: CCP 2013
//

#include "StdAfx.h"
#include "Tr2RenderContext.h"
#include "Tr2VariableStore.h"
#include "Tr2RenderTarget.h"
#include "Tr2PerObjectData.h"
#include "TriRenderBatch.h"
#include "Shader/Tr2Shader.h"
#include "TbbStub.h"

CCP_STATS_DECLARE( batchCount, "Trinity/batchCount", true, CST_COUNTER_HIGH, "Batches rendered per frame");

Tr2RenderContextBase::Tr2RenderContextBase( Tr2RenderContext& renderContext )
	:m_esm( renderContext )
{
	m_esm.Initialize();
#if !TRINITY_PLATFORM_HAS_PRIMARY_CONTEXT
	m_backBuffer.CreateInstance();
	m_backBuffer->SetName( "backbuffer" );
#endif
	m_objectIdVariable		= GlobalStore().RegisterVariable( "objectId",	0.0f );
	m_areaIdVariable		= GlobalStore().RegisterVariable( "areaId",		0.0f );
}

using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------------
// Description:
//   Called by Tr2RenderContextAL when a primary or secondary context is created. 
//   Initializes Tr2EffectStateManager instance.
// Arguments:
//   renderContext - AL render context created
// --------------------------------------------------------------------------------------
void Tr2RenderContextBase::OnContextCreated( Tr2PrimaryRenderContextAL& renderContext )
{
#if !TRINITY_PLATFORM_HAS_PRIMARY_CONTEXT
	m_backBuffer->Attach( renderContext.GetDefaultBackBuffer(), this );
#endif
}

#if !TRINITY_PLATFORM_HAS_PRIMARY_CONTEXT
// --------------------------------------------------------------------------------------
// Description:
//   Returns back buffer render target as Blue-exposed Tr2RenderTarget. Exposed to 
//   script.
// Return Value:
//   Back buffer render target 
// --------------------------------------------------------------------------------------
Tr2RenderTargetPtr Tr2RenderContextBase::GetBackBuffer()
{
	return m_backBuffer;
}
#endif


Tr2RenderContext::Tr2RenderContext()
	:Tr2RenderContextBase( *this ),
	m_parallelContextMutex( "Tr2RenderContext", "m_parallelContextMutex" ),
	m_parallelContextSemaphore( 0, 1024 )
{
#if !TRINITY_PLATFORM_HAS_PRIMARY_CONTEXT
	m_events = this;
#endif
}

void Tr2RenderContext::PrepareParallelContext( uint32_t index, Tr2RenderContext& context )
{
#if TRINITY_PLATFORM_SUPPORTS_PARALLEL_CONTEXTS
	ForkContext( &context, index );
	for( uint32_t i = 0; i != CBUFFER_COUNT; ++i )
	{
		Tr2ConstantBufferAL* buffer = GetConstantBuffer( i );
		if( buffer && buffer->IsValid() )
		{
			void* data = nullptr;
			buffer->Lock( &data, *this );
			context.GetConstantBuffer( i )->Create( buffer->GetSize(), Tr2ConstantUsageAL::REUSABLE, data, *this );
			buffer->Unlock( *this );
		}
	}
	context.m_esm.AssignFrom( m_esm );
#endif
}

uint32_t Tr2RenderContext::BeginParallelEncoding( uint32_t count )
{
#if TRINITY_PLATFORM_SUPPORTS_PARALLEL_CONTEXTS
	CCP_STATS_ZONE( __FUNCTION__ );
	
	CCP_ASSERT( m_parallelContextsPool.empty() );

	uint32_t available = 0;
	{
		CCP_STATS_ZONE( "Tr2RenderContextAL::BeginParallelEncoding" );
		available = std::min( count, Tr2RenderContextAL::BeginParallelEncoding( count ) );
	}
	if( available )
	{
		m_esm.SetupContextResources();
		
		for( size_t i = m_parallelContexts.size(); i < available; ++i )
		{
			Tr2RenderContextPtr context;
			context.CreateInstance();
			m_parallelContexts.push_back( context );
		}
		
		for( uint32_t i = 0; i < available; ++i )
		{
			PrepareParallelContext( i, *m_parallelContexts[i] );
			m_parallelContextsPool.push_back( m_parallelContexts[i] );
			m_parallelContextSemaphore.Signal();
		}
	}
	return available;
#else
	return 0;
#endif
}

Tr2RenderContext* Tr2RenderContext::Fork()
{
	while( true )
	{
		m_parallelContextSemaphore.Wait();
		
		CcpAutoMutex lock( m_parallelContextMutex );
		
		if( m_parallelContextsPool.empty() )
		{
			continue;
		}
		
		auto ctx = m_parallelContextsPool.back();
		m_parallelContextsPool.pop_back();
		return ctx.Detach();
	}
}

void Tr2RenderContext::Join( Tr2RenderContext* context )
{
	CcpAutoMutex lock( m_parallelContextMutex );
	Tr2RenderContextPtr ctx;
	ctx.Attach( context );
	m_parallelContextsPool.push_back( ctx );
	m_parallelContextSemaphore.Signal();
}

void Tr2RenderContext::EndParallelEncoding()
{
#if TRINITY_PLATFORM_SUPPORTS_PARALLEL_CONTEXTS
	CCP_STATS_ZONE( __FUNCTION__ );
	{
		CCP_STATS_ZONE( "Tr2RenderContextAL::EndParallelEncoding" );
		Tr2RenderContextAL::EndParallelEncoding();
	}
	
	// Reset semaphore to 0
	for( size_t i = 0; i < m_parallelContextsPool.size(); ++i )
	{
		m_parallelContextSemaphore.Wait();
	}
	m_parallelContextsPool.clear();
#endif
}


#if TRINITY_PLATFORM_HAS_PRIMARY_CONTEXT
Tr2PrimaryRenderContext::Tr2PrimaryRenderContext()
	:Tr2RenderContextBase( *reinterpret_cast<Tr2RenderContext*>( this ) )
{
	m_backBuffer.CreateInstance();
	m_backBuffer->SetName( "backbuffer" );

	m_events = this;
}

void Tr2PrimaryRenderContext::OnContextCreated( Tr2PrimaryRenderContextAL& renderContext )
{
	m_backBuffer->Attach( GetDefaultBackBuffer(), this );
	Tr2RenderContextBase::OnContextCreated( renderContext );
}

Tr2PrimaryRenderContext::operator Tr2RenderContext&()
{
	return *reinterpret_cast<Tr2RenderContext*>( this );
}

Tr2RenderTargetPtr Tr2PrimaryRenderContext::GetBackBuffer()
{
	return m_backBuffer;
}

#endif


namespace {
#if TRINITY_PLATFORM_HAS_PRIMARY_CONTEXT
	Tr2PrimaryRenderContextPtr	s_mainThreadRenderContext;
#else
	Tr2RenderContextPtr s_mainThreadRenderContext;
#endif
}
	
void Tr2RenderContext::DestroyMainThreadRenderContext()
{
	if( s_mainThreadRenderContext )
	{
		s_mainThreadRenderContext->Destroy();
		s_mainThreadRenderContext.Unlock();
		Tr2RenderContextAL::SetPrimaryRenderContext( nullptr );
	}
}

Tr2PrimaryRenderContext& Tr2RenderContext_GetMainThreadRenderContext()
{
	if( !s_mainThreadRenderContext )
	{
		s_mainThreadRenderContext.CreateInstance();
		Tr2RenderContextAL::SetPrimaryRenderContext( s_mainThreadRenderContext );
	}

	return *s_mainThreadRenderContext;
}

TriVariable* Tr2RenderContextBase::GetObjectIdVariable( void )
{
	return m_objectIdVariable;
}

namespace
{
const char* s_passNames[] = {
	"Pass #1",
	"Pass #2",
	"Pass #3",
	"Pass #4",
	"Pass #5",
	"Pass #6",
	"Pass #7",
	"Pass #8",
	"Pass #...",
};

const char* GetPassName( size_t passIndex )
{
	return s_passNames[std::min( passIndex, sizeof( s_passNames ) / sizeof( s_passNames[0] ) )];
}

}

void Tr2RenderContextBase::RenderBatchesInOrder( ITriRenderBatchAccumulator* batches, const BlueSharedString& techniqueName )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	D3DPERF_EVENT( L"Tr2RenderContext::RenderBatchesInOrder" );


	const Tr2PerObjectData* curPerObjectData = nullptr;

	Tr2ConstantBufferAL*	perObjectConstantBuffers[CBUFFER_COUNT];
	for( uint32_t i = 0; i != CBUFFER_COUNT; ++i )
	{
		perObjectConstantBuffers[i] = &m_perObjectConstantBuffers[i];
	}

	for( TriRenderBatch* it = batches->GetFirstBatch(); it != nullptr; it = it->GetNext() )
	{
		auto material = it->GetShaderMaterialInterface();

		if( !material )
		{
			continue;
		}

		auto shader = it->GetShaderStateInterface();

		if( shader == 0 )
		{
			continue;
		}

		uint32_t technique;
		if( !shader->GetTechniqueIndex( techniqueName, technique ) )
		{
			continue;
		}

		const Tr2PerObjectData* perObjectData = it->GetPerObjectData();
		Tr2RenderContext* renderContext = reinterpret_cast<Tr2RenderContext*>( this );

		if( perObjectData && ( perObjectData != curPerObjectData ) )
		{
			D3DPERF_EVENT(L"RenderBatchesInOrder::Set per-object data to device");
			
			perObjectData->SetPerObjectDataToDevice( perObjectConstantBuffers, shader->GetShaderTypeMask( technique ), *renderContext );
			curPerObjectData = perObjectData;
		}

		uint32_t passCount = shader->GetPassCount( technique );

		for( uint32_t passIx = 0; passIx < passCount; ++passIx )
		{
			D3DPERF_EVENT1( L"Pass %i", passIx );
			shader->ApplyAllStateForPass( technique, passIx, *renderContext );
			material->ApplyMaterialDataForPass( technique, passIx, *renderContext );

			Tr2GpuProfiler::GetProfiler().Begin( material, GetPassName( passIx ), *renderContext );
			it->SubmitGeometry( *renderContext );
			Tr2GpuProfiler::GetProfiler().End( *renderContext );

			CCP_STATS_INC( batchCount );
		}
	}
}

void Tr2RenderContextBase::RenderBatchesSortedByEffectHelper( TriRenderBatch* batch, TriRenderBatch* endBatch, const BlueSharedString& techniqueName )
{
	Tr2RenderContext *renderContext = reinterpret_cast<Tr2RenderContext*>( this );

	Tr2EffectStateManager::RenderingMode currentMode = Tr2EffectStateManager::RM_ANY;
	const Tr2PerObjectData* curPerObjectData = nullptr;
	unsigned int curShaderTypeMask = 0;

	while( batch != endBatch )
	{
		Tr2ConstantBufferAL*	perObjectConstantBuffers[CBUFFER_COUNT];
		for( uint32_t i = 0; i != CBUFFER_COUNT; ++i )
		{
			perObjectConstantBuffers[i] = renderContext->GetConstantBuffer( i );
		}
		
		auto material = batch->GetShaderMaterialInterface();


		// If the batch doesn't have an effect, it is a view-modifier batch,
		// so we need to call SubmitGeometry (to make the view parameter change)
		// and move to the next batch
		if( !material )
		{
			D3DPERF_EVENT( CA2W(batch->GetBatchTypeName().c_str()) );
			Tr2GpuProfiler::GetProfiler().Begin( nullptr, "Batch Without Material", *renderContext );
			batch->SubmitGeometry( *renderContext );
			Tr2GpuProfiler::GetProfiler().End( *renderContext );
			batch = batch->GetNext();
			continue;
		}

		auto currentShader = batch->GetShaderStateInterface();

		if ( currentShader == 0)
		{
			batch = batch->GetNext();
			continue;
		}

		uint32_t technique;
		if( !currentShader->GetTechniqueIndex( techniqueName, technique ) )
		{
			batch = batch->GetNext();
			continue;
		}

		const auto currentShaderMask = currentShader->GetShaderTypeMask( technique );

		// Get the number of passes (must be at least 1 to SubmitGeometry)
		const uint32_t passCount = currentShader->GetPassCount( technique );
		if( passCount == 0)
		{
			batch = batch->GetNext();
			continue;
		}

		// Change the rendering mode , if needed
		const Tr2EffectStateManager::RenderingMode mode = batch->GetRenderingMode();
		if( mode != Tr2EffectStateManager::RM_ANY )
		{
			m_esm.ApplyStandardStates( mode );
			currentMode = mode;
		}

		TriRenderBatch* const startOfPass = batch;
		for( uint32_t passIx = 0; passIx < passCount; ++passIx )
		{
			{
				D3DPERF_EVENT1( L"Begin Pass %i", passIx );

				currentShader->ApplyAllStateForPass( technique, passIx, *renderContext );
			}

			batch = startOfPass;

			// Figure out in advance, before we actually start applying state and submitting geometry, how many
			// batches we will be able to batch based on rendering mode and shader sharing.
			TriRenderBatch *lastBatch = batch;
			while( lastBatch && lastBatch != endBatch )
			{
				const Tr2EffectStateManager::RenderingMode mode = lastBatch->GetRenderingMode();
				if( mode != Tr2EffectStateManager::RM_ANY && mode != currentMode )
				{
					break;
				}

				material = lastBatch->GetShaderMaterialInterface();
				if( material )
				{
					if( lastBatch->GetShaderStateInterface() != currentShader )
					{
						break;
					}
				}
				else
				{
					break;
				}

				lastBatch = lastBatch->GetNext();
			}

			while( batch && batch != lastBatch )
			{
				D3DPERF_EVENT( CA2W( batch->GetBatchTypeName().c_str() ) );

				// Set the data from the material, i.e constants and samplers for this pass
				batch->GetShaderMaterialInterface()->ApplyMaterialDataForPass( technique, passIx, *renderContext );

				// If the batch has per-object data, set it to the device
				const Tr2PerObjectData* const perObjectData = batch->GetPerObjectData();
				if( perObjectData && ( ( perObjectData != curPerObjectData ) || ( currentShaderMask != curShaderTypeMask ) ) )
				{
					D3DPERF_EVENT( L"Object - RenderBatchesSortedByEffect::SetPerObjectDataToDevice" );
					curPerObjectData = perObjectData;
					curShaderTypeMask = currentShaderMask;
					perObjectData->SetPerObjectDataToDevice( perObjectConstantBuffers, currentShaderMask, *renderContext );
				}

				CCP_STATS_INC( batchCount );

				// Submit the geometry for this batch
				Tr2GpuProfiler::GetProfiler().Begin( batch->GetShaderMaterialInterface(), GetPassName( passIx ), *renderContext );
				batch->SubmitGeometry( *renderContext );
				Tr2GpuProfiler::GetProfiler().End( *renderContext );

				batch = batch->GetNext();
			}
		}
	}
}

void Tr2RenderContextBase::RenderBatchesSortedByEffect( ITriRenderBatchAccumulator* batches, const BlueSharedString& techniqueName )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	D3DPERF_EVENT(L"Tr2EffectStateManager::RenderBatchesSortedByEffect");

	Tr2RenderContext* primaryContext = reinterpret_cast<Tr2RenderContext*>( this );
#if TRINITY_PLATFORM_SUPPORTS_PARALLEL_CONTEXTS
	size_t batchCount = batches->GetBatchCount();
	extern bool g_useParallelEncoding;
	if( g_useParallelEncoding && batchCount > 256 && !Tr2GpuProfiler::GetProfiler().IsCapturing() )
	{
		const uint32_t coresCount = std::thread::hardware_concurrency();
		uint32_t batchesPerEncoder = std::max( 32u, uint32_t( ( batchCount + coresCount ) / coresCount ) );
		uint32_t taskCount = uint32_t( ( batchCount + batchesPerEncoder ) / batchesPerEncoder );
		
		std::vector<std::pair<int, TriRenderBatch*>> encodingTasks;
		encodingTasks.reserve( taskCount );
		
		TriRenderBatch* batch = batches->GetFirstBatch();
		int i = 0;
		while( batch )
		{
			if( batch->m_needsSyncronousSubmit )
			{
				batch->SyncronousSubmit( *primaryContext );
			}
			if( i == 0 )
			{
				encodingTasks.push_back( std::make_pair( encodingTasks.size(), batch ) );
			}
			++i %= batchesPerEncoder;
			batch = batch->GetNext();
		}

		if( primaryContext->BeginParallelEncoding( taskCount ) )
		{
			Tr2ParallelDo( encodingTasks.begin(), encodingTasks.end(), [&]( const std::pair<int, TriRenderBatch*>& task )
			{
#if __APPLE__
				@autoreleasepool
#endif
				{
					CCP_STATS_ZONE( "Parallel Encoding Task" );

					Tr2RenderContext* ctx = primaryContext->Fork();
					
					TriRenderBatch* beginBatch = task.second;
					TriRenderBatch* doneBatch = nullptr;
					if( task.first < encodingTasks.size() - 1 )
					{
						doneBatch = encodingTasks[task.first + 1].second;
					}
					
					ctx->RenderBatchesSortedByEffectHelper( beginBatch , doneBatch, techniqueName );
					
					primaryContext->Join( ctx );
				}
			} );
			primaryContext->EndParallelEncoding();
		}
		else
		{
			RenderBatchesSortedByEffectHelper( batches->GetFirstBatch(), nullptr, techniqueName );
		}
	}
	else
#endif
	{
		RenderBatchesSortedByEffectHelper( batches->GetFirstBatch(), nullptr, techniqueName );
	}
}

void Tr2RenderContextBase::RenderBatches( ITriRenderBatchAccumulator* batches, const BlueSharedString& techniqueName )
{
	if( batches->IsChainedByEffect() )
	{
		RenderBatchesSortedByEffect( batches, techniqueName );
	}
	else
	{
		RenderBatchesInOrder( batches, techniqueName );
	}
}

void Tr2RenderContextBase::RenderBatchesWithOverride( ITriRenderBatchAccumulator* batches, Tr2Material* overrideEffect, const BlueSharedString& techniqueName )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !overrideEffect )
	{
		RenderBatches( batches, techniqueName );
		return;
	}

	D3DPERF_EVENT( L"Tr2RenderContextBase::RenderBatchesWithOverride" );

	auto overrideShader = overrideEffect->GetShaderStateInterface();
	if( !overrideShader )
	{
		return;
	}

	Tr2ConstantBufferAL*	perObjectConstantBuffers[CBUFFER_COUNT];
	for( uint32_t i = 0; i != CBUFFER_COUNT; ++i )
	{
		perObjectConstantBuffers[i] = &m_perObjectConstantBuffers[i];
	}

	auto overrideMaterial =  overrideEffect;
	for( TriRenderBatch* it = batches->GetFirstBatch(); it != nullptr; it = it->GetNext() )
	{
		auto mode = it->RenderWithOverride();
		if( mode == TriRenderBatch::DO_NOT_RENDER_WITH_OVERRIDE )
		{
			continue;
		}

		auto materialForThisBatch = it->GetShaderMaterialInterface();
		auto shaderForThisBatch = it->GetShaderStateInterface();

		if( !shaderForThisBatch )
		{
			continue;
		}

		uint32_t technique;
		if( !shaderForThisBatch->GetTechniqueIndex( techniqueName, technique ) )
		{
			continue;
		}
		if( shaderForThisBatch->GetPassCount( technique ) == 0 )
		{
			continue;
		}

		// Setting render states appropriate to this batch (e.g. wireframe)
		m_esm.ApplyStandardStates( it->GetRenderingMode() );

		Tr2RenderContext *renderContext = reinterpret_cast<Tr2RenderContext*>( this );

		// Get the per-object data
		const Tr2PerObjectData* perObjectData = it->GetPerObjectData();

		uint32_t passCount = overrideShader->GetPassCount( 0 );

		for( uint32_t passIx = 0; passIx < passCount; ++passIx )
		{
			D3DPERF_EVENT1(L"Pass %i", passIx);

			shaderForThisBatch->ApplyShaderOverride( technique, 0, *overrideShader, passIx, *renderContext );

			materialForThisBatch->ApplyMaterialDataForPass( technique, 0, *renderContext );

			if( mode != TriRenderBatch::DO_NOT_USE_OVERRIDE_SHADERS )
			{
				overrideShader->ApplyRenderStates( 0, passIx, *renderContext );
			}
			else
			{
				shaderForThisBatch->ApplyRenderStates( technique, passIx, *renderContext );
			}


			// Apply per-object data
			//------------------------
			// NOTE: it's necessary to set per-object data *after* applying constants
			// and samplers from the effect, because the effect can stomp on the per-object
			// data in some cases.
			//
			// TODO: ensure that the per-object data can be set outside of the pass loop (optimization)
			// without the effect interfering with per-object parameters and data
			// <delder> 18-11-2009
			//
			if( perObjectData )
			{
				D3DPERF_EVENT(L"Tr2RenderContextBase::RenderBatchesWithOverride - SetPerObjectDataToDevice");
				perObjectData->SetPerObjectDataToDevice( perObjectConstantBuffers, overrideShader->GetShaderTypeMask( 0 ), *renderContext );
			}

			Tr2GpuProfiler::GetProfiler().Begin( materialForThisBatch, GetPassName( passIx ), *renderContext );
			it->SubmitGeometry( *renderContext );
			Tr2GpuProfiler::GetProfiler().End( *renderContext );

			CCP_STATS_INC( batchCount );
		}
	}
}

void Tr2RenderContextBase::RenderBatchesForPicking( ITriRenderBatchAccumulator* batches, const BlueSharedString& techniqueName )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	D3DPERF_EVENT(L"Tr2EffectStateManager::RenderBatchesForPicking");


	const Tr2PerObjectData* curPerObjectData = nullptr;

	Tr2ConstantBufferAL*	perObjectConstantBuffers[CBUFFER_COUNT];
	for( uint32_t i = 0; i != CBUFFER_COUNT; ++i )
	{
		perObjectConstantBuffers[i] = &m_perObjectConstantBuffers[i];
	}

	Tr2RenderContext *renderContext = reinterpret_cast<Tr2RenderContext*>( this );

	for( TriRenderBatch* it = batches->GetFirstBatch(); it != nullptr; it = it->GetNext() )
	{
		auto material = it->GetShaderMaterialInterface();

		if( !material )
		{
			continue;
		}

		auto shader = it->GetShaderStateInterface();

		if( !shader )
		{
			continue;
		}

		const Tr2PerObjectData* perObjectData = it->GetPerObjectData();

		uint32_t technique;
		if( !shader->GetTechniqueIndex( techniqueName, technique ) )
		{
			continue;
		}

		if( perObjectData )
		{
			if( perObjectData != curPerObjectData )
			{
				D3DPERF_EVENT(L"Set per-object data to device");
				perObjectData->SetPerObjectDataToDevice( perObjectConstantBuffers, shader->GetShaderTypeMask( technique ), *renderContext );
				curPerObjectData = perObjectData;
			}

			uint32_t id = perObjectData->GetUserData();
			if( m_objectIdVariable )
			{
				m_objectIdVariable->SetValue( (float)id );
			}

			uint32_t areaID = it->GetPickingData();
			if( m_areaIdVariable )
			{
				m_areaIdVariable->SetValue( (float)areaID );
			}
		}

		uint32_t passCount = shader->GetPassCount( technique );
		CCP_ASSERT( passCount == 1 );

		shader->ApplyAllStateForPass( technique, 0, *renderContext );
		material->ApplyMaterialDataForPass( technique, 0, *renderContext );
		Tr2GpuProfiler::GetProfiler().Begin( material, "", *renderContext );
		it->SubmitGeometry( *renderContext );
		Tr2GpuProfiler::GetProfiler().End( *renderContext );

		CCP_STATS_INC( batchCount );

	}
}
