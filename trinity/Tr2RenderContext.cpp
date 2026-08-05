// Copyright © 2013 CCP ehf.

#include "StdAfx.h"
#include "Tr2RenderContext.h"
#include "Tr2VariableStore.h"
#include "Tr2RenderTarget.h"
#include "Tr2PerObjectData.h"
#include "TriRenderBatch.h"
#include "Shader/Tr2Shader.h"
#include "TbbStub.h"
#include "Tr2Renderer.h"
#include "TriSettingsRegistrar.h"
#include "Tr2RingBuffer.h"

#include "Shader/Tr2Effect.h"


bool g_gdrEnabled = true;

TRI_REGISTER_SETTING( "gdrEnabled", g_gdrEnabled );


CCP_STATS_DECLARE( batchCount, "Trinity/batchCount", true, CST_COUNTER_HIGH, "Batches rendered per frame" );
CCP_STATS_DECLARE( batchNormalDraws, "Trinity/batchNormalDraws", true, CST_COUNTER_HIGH, "Normal batch draw calls per frame" );
CCP_STATS_DECLARE( batchIndirectDraws, "Trinity/batchIndirectDraws", true, CST_COUNTER_HIGH, "Draw calls in ExecuteIndirect() per frame" );
CCP_STATS_DECLARE( batchExecuteIndirectCalls, "Trinity/batchExecuteIndirectCalls", true, CST_COUNTER_HIGH, "ExecuteIndirect() batch calls per frame" );

namespace
{

Tr2EnumerableThreadSpecific<Tr2BindlessResourcesAL> s_perThreadUsedTextures;

void UseTextures( ITriRenderBatchAccumulator* batches, const BlueSharedString& techniqueName, Tr2RenderContextAL& renderContext )
{
#if TRINITY_PLATFORM_SUPPORTS_HEAP_VIEW
	CCP_STATS_ZONE( __FUNCTION__ );
	Tr2BindlessResourcesAL usedTextures;

	usedTextures.Add( Tr2Renderer::GetFallbackTexture( Tr2EffectResource::TEXTURE_2D, "" ) );
	usedTextures.Add( Tr2Renderer::GetFallbackTexture( Tr2EffectResource::TEXTURE_3D, "" ) );
	usedTextures.Add( Tr2Renderer::GetFallbackTexture( Tr2EffectResource::TEXTURE_CUBE, "" ) );

	auto ProcessBatch = [&techniqueName]( auto& batch, Tr2BindlessResourcesAL& resources ) {
		uint32_t technique;
		if( batch.m_shader->GetTechniqueIndex( techniqueName, technique ) )
		{
			batch.m_material->GetUsedBindlessTextures( technique, resources );
		}
	};
	auto ProcessBatches = [&ProcessBatch, &usedTextures]( auto& batches ) {
		const size_t partitionSize = 500;
		if( batches.size() > partitionSize )
		{
			Tr2ParallelFor( Tr2BlockedRange<size_t>( 0, batches.size(), partitionSize ), [&]( Tr2BlockedRange<size_t> range ) {
				for( auto i = range.begin(); i != range.end(); ++i )
				{
					ProcessBatch( batches[i], s_perThreadUsedTextures.local() );
				}
			} );
			for( auto& threadResources : s_perThreadUsedTextures )
			{
				usedTextures.Add( threadResources );
				threadResources.Clear();
			}
		}
		else
		{
			for( auto& batch : batches )
			{
				ProcessBatch( batch, usedTextures );
			}
		}
	};
	ProcessBatches( batches->GetGdprBatches() );
	ProcessBatches( batches->GetBatches() );
	{
		CCP_STATS_ZONE( "renderContext.UseResources" );
		renderContext.UseResources( Tr2UseResourceDestination::RENDER, Tr2GpuUsage::SHADER_RESOURCE, usedTextures );
	}
#endif
}

void SubmitGeometry( const Tr2RenderBatch& batch, Tr2RenderContext& renderContext )
{
	renderContext.SetTopology( batch.m_topology );
	renderContext.m_esm.ApplyVertexDeclaration( batch.m_vertexDeclaration );
	if( batch.m_vertexStreams[0] )
	{
		renderContext.m_esm.ApplyStreamSource( 0, *batch.m_vertexStreams[0], 0, batch.m_stride[0] );
	}
	if( batch.m_vertexStreams[1] )
	{
		renderContext.m_esm.ApplyStreamSource( 1, *batch.m_vertexStreams[1], 0, batch.m_stride[1] );
	}
	if( batch.m_indexBuffer )
	{
		renderContext.m_esm.ApplyIndexBuffer( *batch.m_indexBuffer, batch.m_indexStride );
		renderContext.DrawIndexedInstanced( batch.m_indexCountPerInstance, batch.m_instanceCount, batch.m_startIndexLocation, batch.m_baseVertexLocation, batch.m_startInstanceLocation );
	}
	else
	{
		renderContext.DrawInstanced( batch.m_indexCountPerInstance, batch.m_instanceCount, batch.m_startIndexLocation, batch.m_startInstanceLocation );
	}
}

}

Tr2RenderContextBase::Tr2RenderContextBase( Tr2RenderContext& renderContext ) :
	m_esm( renderContext )
{
	m_esm.Initialize();
#if !TRINITY_PLATFORM_HAS_PRIMARY_CONTEXT
	m_backBuffer.CreateInstance();
	m_backBuffer->SetName( "backbuffer" );
#endif
	m_objectIdVariable = GlobalStore().RegisterVariable( "objectId", 0.0f );
	m_areaIdVariable = GlobalStore().RegisterVariable( "areaId", 0.0f );
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


Tr2RenderContext::Tr2RenderContext() :
	Tr2RenderContextBase( *this ),
	m_parallelContextMutex( "Tr2RenderContext", "m_parallelContextMutex" ),

#ifdef PYTHON2_SUPPORT
	m_parallelContextSemaphore( 0, 1024 )
#else
	m_parallelContextSemaphore( "Tr2RenderContext_m_parallelContextSemaphore", 0, 1024 )
#endif
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
Tr2PrimaryRenderContext::Tr2PrimaryRenderContext() :
	Tr2RenderContextBase( *reinterpret_cast<Tr2RenderContext*>( this ) )
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


bool g_renderContextIsBeingDestroyed = false;

namespace
{
#if TRINITY_PLATFORM_HAS_PRIMARY_CONTEXT
Tr2PrimaryRenderContextPtr s_mainThreadRenderContext;
#else
Tr2RenderContextPtr s_mainThreadRenderContext;
#endif
}

void Tr2RenderContext::DestroyMainThreadRenderContext()
{
	if( s_mainThreadRenderContext )
	{
		g_renderContextIsBeingDestroyed = true;
		Tr2RenderContextAL::SetPrimaryRenderContext( nullptr );
		s_mainThreadRenderContext.Unlock();
		g_renderContextIsBeingDestroyed = false;
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
	Tr2RenderContext* primaryContext = reinterpret_cast<Tr2RenderContext*>( this );
	Tr2RingBuffer::GetInstance<Float4x3>().PrepareBuffer( *primaryContext );
	Tr2RingBuffer::GetInstance<Tr2MorphTargetAnimationData>().PrepareBuffer( *primaryContext );

	D3DPERF_EVENT( L"Tr2RenderContext::RenderBatchesInOrder" );

	Tr2RenderContext* renderContext = reinterpret_cast<Tr2RenderContext*>( this );

	CCP_STATS_ZONE( "Direct drawing (sorted)" );
#if TRINITY_PLATFORM != TRINITY_METAL
	GPU_REGION( *renderContext, "Direct drawing (sorted)" );
#endif

	UseTextures( batches, techniqueName, *renderContext );

	Tr2ConstantBufferAL* perObjectConstantBuffers[CBUFFER_COUNT];
	for( uint32_t i = 0; i != CBUFFER_COUNT; ++i )
	{
		perObjectConstantBuffers[i] = &m_perObjectConstantBuffers[i];
	}

	const Tr2PerObjectData* curPerObjectData = nullptr;
	Tr2Shader* lastShader = nullptr;
	uint32_t technique = 0;
	uint32_t passCount = 0;
	uint32_t shaderMask = 0;

	for( auto& batch : batches->GetBatches() )
	{
		if( batch.m_renderingMode != Tr2EffectStateManager::RM_ANY )
		{
			m_esm.ApplyStandardStates( batch.m_renderingMode );
		}

		if( batch.m_shader != lastShader )
		{
			if( !batch.m_shader->GetTechniqueIndex( techniqueName, technique ) )
			{
				continue;
			}
			passCount = batch.m_shader->GetPassCount( technique );
			if( passCount == 0 )
			{
				continue;
			}
			shaderMask = batch.m_shader->GetShaderTypeMask( technique );
			lastShader = batch.m_shader;
		}

		const char* effectPath = dynamic_cast<Tr2Effect*>( batch.m_material )->GetEffectPathName();
		CCP_STATS_ZONE( effectPath );

#if TRINITY_PLATFORM == TRINITY_DIRECTX12
		GPU_REGION( *renderContext, effectPath );
#endif

		if( batch.m_objectData && ( batch.m_objectData != curPerObjectData ) )
		{
			batch.m_objectData->SetPerObjectDataToDevice( perObjectConstantBuffers, shaderMask, *renderContext );
			curPerObjectData = batch.m_objectData;
		}

		for( uint32_t passIx = 0; passIx < passCount; ++passIx )
		{
			D3DPERF_EVENT1( L"Pass %i", passIx );

			batch.m_shader->ApplyAllStateForPass( technique, passIx, *renderContext );
			batch.m_material->ApplyMaterialDataForPass( technique, passIx, *renderContext );

			Tr2GpuProfiler::GetProfiler().Begin( batch.m_material, GetPassName( passIx ), *renderContext );
			SubmitGeometry( batch, *renderContext );
			Tr2GpuProfiler::GetProfiler().End( *renderContext );

			CCP_STATS_INC( batchCount );
			CCP_STATS_INC( batchNormalDraws );
		}
	}
}

bool Tr2RenderContextBase::TechniqueInBatch( const std::vector<Tr2RenderBatch>& batches, const BlueSharedString& techniqueName )
{
	Tr2Shader* prevShader = nullptr;
	for( auto& batch : batches )
	{
		if( batch.m_shader == prevShader )
		{
			continue;
		}
		prevShader = batch.m_shader;
		uint32_t technique;
		if( !batch.m_shader->GetTechniqueIndex( techniqueName, technique ) )
		{
			continue;
		}
		const uint32_t passCount = batch.m_shader->GetPassCount( technique );
		if( passCount == 0 )
		{
			continue;
		}
		return true;
	}
	return false;
}

void Tr2RenderContextBase::RenderBatchGroup( std::vector<Tr2RenderBatch>::const_iterator startBatch, const BlueSharedString& techniqueName, Tr2ConstantBufferAL** buffers, Tr2RenderContext& renderContext )
{

	auto& firstBatch = *startBatch;
	auto endBatch = startBatch + firstBatch.m_groupCount;

	if( firstBatch.m_renderingMode != Tr2EffectStateManager::RM_ANY )
	{
		m_esm.ApplyStandardStates( firstBatch.m_renderingMode );
	}

	auto currentShader = firstBatch.m_material->GetShaderStateInterface();
	uint32_t technique;
	if( !currentShader->GetTechniqueIndex( techniqueName, technique ) )
	{
		return;
	}
	const uint32_t passCount = currentShader->GetPassCount( technique );
	if( passCount == 0 )
	{
		return;
	}

	const char* effectPath = dynamic_cast<Tr2Effect*>( startBatch->m_material )->GetEffectPathName();
	CCP_STATS_ZONE( effectPath );
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	GPU_REGION( renderContext, effectPath );
#endif

	const auto currentShaderMask = currentShader->GetShaderTypeMask( technique );

	m_esm.ApplyVertexDeclaration( firstBatch.m_vertexDeclaration );
	if( firstBatch.m_vertexStreams[0] )
	{
		m_esm.ApplyStreamSource( 0, *firstBatch.m_vertexStreams[0], 0, firstBatch.m_stride[0] );
	}
	if( firstBatch.m_vertexStreams[1] )
	{
		m_esm.ApplyStreamSource( 1, *firstBatch.m_vertexStreams[1], 0, firstBatch.m_stride[1] );
	}
	if( firstBatch.m_indexBuffer )
	{
		m_esm.ApplyIndexBuffer( *firstBatch.m_indexBuffer, firstBatch.m_indexStride );
	}

	for( uint32_t passIx = 0; passIx < passCount; ++passIx )
	{
		auto batch = startBatch;

		const auto& passDesc = currentShader->GetEffectDescription().techniques[technique].passes[passIx];

		currentShader->ApplyAllStateForPass( technique, passIx, renderContext );

		for( auto batch = startBatch; batch != endBatch; ++batch )
		{
			batch->m_material->ApplyMaterialDataForPass( technique, passIx, renderContext );
			if( batch->m_objectData )
			{
				batch->m_objectData->SetPerObjectDataToDevice( buffers, currentShaderMask, renderContext );
			}
			renderContext.SetTopology( batch->m_topology );
			Tr2GpuProfiler::GetProfiler().Begin( batch->m_material, GetPassName( passIx ), renderContext );
			if( firstBatch.m_indexBuffer )
			{
				renderContext.DrawIndexedInstanced( batch->m_indexCountPerInstance, batch->m_instanceCount, batch->m_startIndexLocation, batch->m_baseVertexLocation, batch->m_startInstanceLocation );
			}
			else
			{
				renderContext.DrawInstanced( batch->m_indexCountPerInstance, batch->m_instanceCount, batch->m_startIndexLocation, batch->m_startInstanceLocation );
			}
			Tr2GpuProfiler::GetProfiler().End( renderContext );
		}
		CCP_STATS_ADD( batchCount, firstBatch.m_groupCount );
		CCP_STATS_ADD( batchNormalDraws, firstBatch.m_groupCount );
	}
}

void Tr2RenderContextBase::RenderSortedBatches( const std::vector<Tr2RenderBatch>& batches, const BlueSharedString& techniqueName, Tr2RenderContext& renderContext )
{
	size_t batchCount = batches.size();

	if( batchCount == 0 )
	{
		return;
	}

	CCP_STATS_ZONE( "Direct drawing" );
#if TRINITY_PLATFORM != TRINITY_METAL
	GPU_REGION( renderContext, "Direct drawing" );
#endif

#if TRINITY_PLATFORM_SUPPORTS_PARALLEL_CONTEXTS
	extern bool g_useParallelEncoding;
	if( g_useParallelEncoding && batchCount > 256 && !Tr2GpuProfiler::GetProfiler().IsCapturing() )
	{
		std::vector<std::vector<Tr2RenderBatch>::const_iterator> encodingTasks;
		for( auto batch = batches.begin(); batch != batches.end(); )
		{
			encodingTasks.push_back( batch );
			batch += batch->m_groupCount;
		}

		const uint32_t coresCount = std::thread::hardware_concurrency();
		if( renderContext.BeginParallelEncoding( std::min( coresCount, uint32_t( encodingTasks.size() ) ) ) )
		{
			Tr2ParallelDo( encodingTasks.begin(), encodingTasks.end(), [&]( auto& task ) {
#if __APPLE__
				@autoreleasepool
#endif
				{
					CCP_STATS_ZONE( "Parallel Encoding Task" );

					Tr2RenderContext* ctx = renderContext.Fork();

					Tr2ConstantBufferAL* perObjectConstantBuffers[CBUFFER_COUNT];
					for( uint32_t i = 0; i != CBUFFER_COUNT; ++i )
					{
						perObjectConstantBuffers[i] = ctx->GetConstantBuffer( i );
					}
					ctx->RenderBatchGroup( task, techniqueName, perObjectConstantBuffers, *ctx );

					renderContext.Join( ctx );
				}
			} );
			renderContext.EndParallelEncoding();
			return;
		}
	}
#endif


	Tr2ConstantBufferAL* perObjectConstantBuffers[CBUFFER_COUNT];
	for( uint32_t i = 0; i != CBUFFER_COUNT; ++i )
	{
		perObjectConstantBuffers[i] = renderContext.GetConstantBuffer( i );
	}

	for( auto firstBatch = batches.begin(); firstBatch != batches.end(); )
	{
		RenderBatchGroup( firstBatch, techniqueName, perObjectConstantBuffers, renderContext );
		firstBatch += firstBatch->m_groupCount;
	}
}

void Tr2RenderContextBase::RenderGdprBatches( ITriRenderBatchAccumulator* batches, const BlueSharedString& techniqueName )
{

	CCP_STATS_ZONE( __FUNCTION__ );

	Tr2RenderContext* renderContext = reinterpret_cast<Tr2RenderContext*>( this );

	renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES );

	auto& gdprBatches = batches->GetGdprBatches();

#if TRINITY_PLATFORM == TRINITY_DIRECTX12 || TRINITY_PLATFORM == TRINITY_METAL
	if( g_gdrEnabled )
	{
		CCP_STATS_ZONE( "Indirect drawing" );
#if TRINITY_PLATFORM != TRINITY_METAL
		GPU_REGION( *renderContext, "Indirect drawing" );
#endif

		Tr2RenderContext* primaryContext = reinterpret_cast<Tr2RenderContext*>( this );

		static Tr2IndirectDrawBuffer s_buffer;
		if( !s_buffer.IsValid() )
		{
			s_buffer.Create( 1024 * 1024 );
			if( !s_buffer.IsValid() )
			{
				return;
			}
		}

		s_buffer.SetFrameNumbers( primaryContext->GetPrimaryRenderContext().GetRecordingFrameNumber(), primaryContext->GetPrimaryRenderContext().GetRenderedFrameNumber() );

		struct Bin
		{
			Tr2IndirectDrawBufferWriter writer;
			uint32_t firstIndex;
			uint32_t endIndex;
			uint32_t technique;
			uint32_t pass;
			bool binStart;
		};

		std::vector<Bin> writers;
		writers.reserve( gdprBatches.size() );

		{
			CCP_STATS_ZONE( "Prepare" );

			uint32_t size = uint32_t( gdprBatches.size() );
			uint32_t endIndex = 0;
			for( uint32_t firstIndex = 0; firstIndex < size; firstIndex = endIndex )
			{
				auto& firstBatch = gdprBatches[firstIndex];
				endIndex = firstIndex + firstBatch.m_groupCount;

				uint32_t technique;
				if( !firstBatch.m_shader->GetTechniqueIndex( techniqueName, technique ) )
				{
					continue;
				}
				const uint32_t passCount = firstBatch.m_shader->GetPassCount( technique );
				if( passCount == 0 )
				{
					continue;
				}

				for( uint32_t passIx = 0; passIx < passCount; ++passIx )
				{
					const auto& passDesc = firstBatch.m_shader->GetEffectDescription().techniques[technique].passes[passIx];
					const uint32_t MAX_BATCHES = 64;
					for( auto batch = firstIndex; batch < endIndex; batch += MAX_BATCHES )
					{
						Tr2IndirectDrawBufferWriter writer( s_buffer, passDesc.indirectLayout, std::min( MAX_BATCHES, uint32_t( endIndex - batch ) ), *renderContext );
						writers.push_back( { writer, batch, std::min( endIndex, batch + MAX_BATCHES ), technique, passIx, batch == firstIndex } );
					}
				}
			}
		}

		{
			CCP_STATS_ZONE( "Record All" );

			auto RecordBin = [&]( Bin& bin ) {
				CCP_STATS_ZONE( "Record" );
				for( uint32_t k = bin.firstIndex; k < bin.endIndex; ++k )
				{
					auto& batch = gdprBatches[k];
					batch.m_material->ApplyConstantBuffers( bin.technique, bin.pass, bin.writer, *primaryContext );
					if( batch.m_objectData )
					{
						batch.m_objectData->ApplyConstantBuffers( bin.writer, *primaryContext );
					}
					bin.writer.DrawIndexed( batch.m_indexCountPerInstance, batch.m_instanceCount, batch.m_startIndexLocation, batch.m_baseVertexLocation, batch.m_startInstanceLocation );
					bin.writer.Next();
				}
			};

			Tr2ParallelDo( begin( writers ), end( writers ), RecordBin );
		}

		s_buffer.CopyArguments();

		{
			CCP_STATS_ZONE( "Submit" );

			uint32_t drawCalls = 0;
			for( auto& bin : writers )
			{

				auto& firstBatch = gdprBatches[bin.firstIndex];

				const char* effectPath = dynamic_cast<Tr2Effect*>( firstBatch.m_material )->GetEffectPathName();
				CCP_STATS_ZONE( effectPath );
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
				GPU_REGION( *renderContext, effectPath );
#endif

				if( bin.binStart )
				{
					m_esm.ApplyVertexDeclaration( firstBatch.m_vertexDeclaration );
					if( firstBatch.m_vertexStreams[0] )
					{
						m_esm.ApplyStreamSource( 0, *firstBatch.m_vertexStreams[0], 0, firstBatch.m_stride[0] );
					}
					if( firstBatch.m_vertexStreams[1] )
					{
						m_esm.ApplyStreamSource( 1, *firstBatch.m_vertexStreams[1], 0, firstBatch.m_stride[1] );
					}
					m_esm.ApplyIndexBuffer( *firstBatch.m_indexBuffer, firstBatch.m_indexStride );

					firstBatch.m_shader->ApplyAllStateForPass( bin.technique, bin.pass, *renderContext );
					firstBatch.m_material->ApplyMaterialDataForPass( bin.technique, bin.pass, *renderContext );
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
					renderContext->SetAllState();
					renderContext->FlushGraphicsBarriersDx12();
#elif TRINITY_PLATFORM == TRINITY_METAL
					renderContext->CheckDrawResources();
#endif
				}
				s_buffer.Submit( bin.writer );
				drawCalls += bin.endIndex - bin.firstIndex;
			}
			CCP_STATS_ADD( batchExecuteIndirectCalls, writers.size() );
			CCP_STATS_ADD( batchIndirectDraws, drawCalls );
			CCP_STATS_ADD( batchCount, drawCalls );
		}
		return;
	}
#endif

	RenderSortedBatches( gdprBatches, techniqueName, *renderContext );
}

void Tr2RenderContextBase::RenderBatchesSortedByEffect( ITriRenderBatchAccumulator* batches, const BlueSharedString& techniqueName )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	D3DPERF_EVENT( L"Tr2EffectStateManager::RenderBatchesSortedByEffect" );

	Tr2RenderContext* primaryContext = reinterpret_cast<Tr2RenderContext*>( this );
	Tr2RingBuffer::GetInstance<Float4x3>().PrepareBuffer( *primaryContext );
	Tr2RingBuffer::GetInstance<Tr2MorphTargetAnimationData>().PrepareBuffer( *primaryContext );

	UseTextures( batches, techniqueName, *primaryContext );


	RenderGdprBatches( batches, techniqueName );
	RenderSortedBatches( batches->GetBatches(), techniqueName, *primaryContext );
}

void Tr2RenderContextBase::RenderBatches( ITriRenderBatchAccumulator* batches, const BlueSharedString& techniqueName )
{
	Tr2RenderContext* primaryContext = reinterpret_cast<Tr2RenderContext*>( this );
	Tr2RingBuffer::GetInstance<Float4x3>().PrepareBuffer( *primaryContext );
	Tr2RingBuffer::GetInstance<Tr2MorphTargetAnimationData>().PrepareBuffer( *primaryContext );

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

	Tr2RenderContext* primaryContext = reinterpret_cast<Tr2RenderContext*>( this );
	Tr2RingBuffer::GetInstance<Float4x3>().PrepareBuffer( *primaryContext );
	Tr2RingBuffer::GetInstance<Tr2MorphTargetAnimationData>().PrepareBuffer( *primaryContext );

	D3DPERF_EVENT( L"Tr2RenderContextBase::RenderBatchesWithOverride" );

	auto overrideShader = overrideEffect->GetShaderStateInterface();
	if( !overrideShader )
	{
		return;
	}

	Tr2RenderContext* renderContext = reinterpret_cast<Tr2RenderContext*>( this );
	UseTextures( batches, techniqueName, *renderContext );

	Tr2ConstantBufferAL* perObjectConstantBuffers[CBUFFER_COUNT];
	for( uint32_t i = 0; i != CBUFFER_COUNT; ++i )
	{
		perObjectConstantBuffers[i] = &m_perObjectConstantBuffers[i];
	}

	auto overrideMaterial = overrideEffect;

	auto RenderBatch = [&]( auto& batch ) {
		uint32_t technique;
		if( !batch.m_shader->GetTechniqueIndex( techniqueName, technique ) )
		{
			return;
		}

		m_esm.ApplyStandardStates( batch.m_renderingMode );

		uint32_t passCount = overrideShader->GetPassCount( technique );
		for( uint32_t passIx = 0; passIx < passCount; ++passIx )
		{
			D3DPERF_EVENT1( L"Pass %i", passIx );

			auto program = batch.m_shader->ApplyShaderOverride( technique, 0, *overrideShader, passIx, *renderContext );
			batch.m_material->ApplyMaterialDataForPassWithOverride( technique, 0, program, *renderContext );
			overrideShader->ApplyRenderStates( 0, passIx, *renderContext );

			if( batch.m_objectData )
			{
				batch.m_objectData->SetPerObjectDataToDevice( perObjectConstantBuffers, overrideShader->GetShaderTypeMask( 0 ), *renderContext );
			}

			Tr2GpuProfiler::GetProfiler().Begin( batch.m_material, GetPassName( passIx ), *renderContext );

			SubmitGeometry( batch, *renderContext );

			Tr2GpuProfiler::GetProfiler().End( *renderContext );

			CCP_STATS_INC( batchCount );
		}
	};

	for( auto& batch : batches->GetGdprBatches() )
	{
		RenderBatch( batch );
	}
	for( auto& batch : batches->GetBatches() )
	{
		RenderBatch( batch );
	}
}

void Tr2RenderContextBase::RenderBatchesForPicking( ITriRenderBatchAccumulator* batches, const BlueSharedString& techniqueName )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	D3DPERF_EVENT( L"Tr2EffectStateManager::RenderBatchesForPicking" );

	Tr2RenderContext* primaryContext = reinterpret_cast<Tr2RenderContext*>( this );
	Tr2RingBuffer::GetInstance<Float4x3>().PrepareBuffer( *primaryContext );
	Tr2RingBuffer::GetInstance<Tr2MorphTargetAnimationData>().PrepareBuffer( *primaryContext );


	Tr2RenderContext* renderContext = reinterpret_cast<Tr2RenderContext*>( this );
	UseTextures( batches, techniqueName, *renderContext );

	const Tr2PerObjectData* curPerObjectData = nullptr;

	Tr2ConstantBufferAL* perObjectConstantBuffers[CBUFFER_COUNT];
	for( uint32_t i = 0; i != CBUFFER_COUNT; ++i )
	{
		perObjectConstantBuffers[i] = &m_perObjectConstantBuffers[i];
	}

	for( auto& batch : batches->GetBatches() )
	{
		uint32_t technique;
		if( !batch.m_shader->GetTechniqueIndex( techniqueName, technique ) )
		{
			continue;
		}

		if( batch.m_objectData )
		{
			if( batch.m_objectData != curPerObjectData )
			{
				D3DPERF_EVENT( L"Set per-object data to device" );
				batch.m_objectData->SetPerObjectDataToDevice( perObjectConstantBuffers, batch.m_shader->GetShaderTypeMask( technique ), *renderContext );
				curPerObjectData = batch.m_objectData;
			}

			uint32_t id = batch.m_objectData->GetUserData();
			if( m_objectIdVariable )
			{
				m_objectIdVariable->SetValue( (float)id );
			}

			uint32_t areaID = batch.m_pickingData;
			if( m_areaIdVariable )
			{
				m_areaIdVariable->SetValue( (float)areaID );
			}
		}

		uint32_t passCount = batch.m_shader->GetPassCount( technique );
		CCP_ASSERT( passCount == 1 );

		batch.m_shader->ApplyAllStateForPass( technique, 0, *renderContext );
		batch.m_material->ApplyMaterialDataForPass( technique, 0, *renderContext );
		Tr2GpuProfiler::GetProfiler().Begin( batch.m_material, "", *renderContext );
		SubmitGeometry( batch, *renderContext );
		Tr2GpuProfiler::GetProfiler().End( *renderContext );

		CCP_STATS_INC( batchCount );
	}
}
