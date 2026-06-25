// Copyright © 2020 CCP ehf.

#if TRINITY_PLATFORM == TRINITY_METAL
#import <Foundation/Foundation.h>
#include <float.h>
#include "MetalWorkQueue.h"
#include "MetalContext.h"
#include "MetalShaderTypes.h"
#include "MetalShaderStrings.h"
#include "Tr2PipelineStatsQueryALMetal.h"
#include "Tr2VertexLayoutALMetal.h"
#include "Tr2RtPipelineStateALMetal.h"
#include "Tr2RtShaderTableALMetal.h"
#include "ALLog.h"

// NOTE: If you spot any rendering artifacts - try disabling render/compute state cashing.
#define METAL_ENABLE_RENDER_STATE_CACHING 1
#define METAL_ENABLE_COMPUTE_STATE_CACHING 1

using namespace Tr2RenderContextEnum;

CCP_STATS_DECLARE( gpuFrameTime, "Trinity/AL/GpuFrameTime", false, CST_TIME, "Time spent on GPU processing a frame" );

namespace
{
double s_gpuFrameTime = 0;
}

namespace TrinityALImpl
{

MetalWorkQueue::MetalWorkQueue() :
	m_numRenderAttachments( 0 ),
	m_isPrimary( true ),
	m_dirtyRenderEncoderState( METAL_RENDERENCODERDIRTYSTATE_ALL ),
	m_numCommands( 0 ),
	m_encoderIndex( 1 ),
	m_encoderInUse( false ),
	m_encoderEnded( false ),
	m_encoderHasWork( false ),
	m_generatesEndOfQueueEvent( false ),
	m_isIntelGpu( false ),
	m_isAppleSilicon( false ),
	m_viewport( MTLViewport{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f } ),
	m_scissorRect( MTLScissorRect{ 0, 0, 0, 0 } ),
	m_validViewport( false ),
	m_cullMode( MTLCullModeNone ),
	m_fillMode( MTLTriangleFillModeFill ),
	m_depthBias( MetalDepthBias{ 0.0f, 0.0f, FLT_MAX } ),
	m_depthClipMode( MTLDepthClipModeClip ),
	m_stencilReferenceValue( 0 ),
	m_outputPixelFormat( MTLPixelFormatInvalid ),
	m_outputDepthFormat( MTLPixelFormatInvalid ),
	m_currentVertexDescriptorHash( 0 ),
	m_currentAttachmentsHash( 0 ),
	m_currentRenderPipelineDescriptorHash( 0 ),
	m_currentComputePipelineDescriptorHash( 0 ),
	m_currentDepthStencilDescriptorHash( 0 ),
	m_threadGroupSize( MTLSizeMake( 1, 1, 1 ) ),
	m_shaderResourceMasks( nullptr ),
	m_currentVertexDescriptor( nil ),
	m_currentVertexStreamMask( 0 ),
	m_currentVertexDescriptorBaseHash( 0 ),
	m_hasPendingRenderPassHint( false ),
	m_hasPendingRenderTargetBarrier( false ),
	m_pendingClear( false ),
	m_visibilityQueryInProgress( false )
{
	static_assert( sizeof( m_activeConstBuffersMask[0] ) * 8 >= METAL_MAX_BOUND_BUFFERS,
				   "Mask data type is too small to hold all flags." );
	static_assert( sizeof( m_activeBuffersMask[0] ) * 8 >= METAL_MAX_BOUND_BUFFERS,
				   "Mask data type is too small to hold all flags." );

	static_assert( sizeof( m_dirtyConstBuffersMask[0] ) * 8 >= METAL_MAX_BOUND_BUFFERS,
				   "Mask data type is too small to hold all buffer flags." );
	static_assert( sizeof( m_dirtyBuffersMask[0] ) * 8 >= METAL_MAX_BOUND_BUFFERS,
				   "Mask data type is too small to hold all buffer flags." );
	static_assert( sizeof( m_dirtyTexturesMask[0] ) * 8 >= METAL_MAX_BOUND_TEXTURES,
				   "Mask data type is too small to hold all texture flags." );
	static_assert( sizeof( m_dirtySamplersMask[0] ) * 8 >= METAL_MAX_BOUND_SAMPLERS,
				   "Mask data type is too small to hold all sampler flags." );

	for( uint32_t i = 0; i < METAL_MAX_RENDER_TARGETS; ++i )
	{
		m_blendState[i].blendType = METAL_BLENDING_DISABLED;
		m_blendState[i].alphaCoverageEnable = false;
		m_blendState[i].rgbBlendOp = MTLBlendOperationAdd;
		m_blendState[i].alphaBlendOp = MTLBlendOperationAdd;
		m_blendState[i].srcColorFactor = MTLBlendFactorOne;
		m_blendState[i].destColorFactor = MTLBlendFactorZero;
		m_blendState[i].srcAlphaFactor = MTLBlendFactorOne;
		m_blendState[i].destAlphaFactor = MTLBlendFactorZero;
		m_blendState[i].blendColor = MetalColor{ 0.0f, 0.0f, 0.0f, 0.0f };
		m_blendState[i].writeMask = MTLColorWriteMaskAll;
	}

	m_frameSemaphore = dispatch_semaphore_create( 2 );

	m_depthStencilDescriptor = [MTLDepthStencilDescriptor new];
	m_depthStencilDescriptor.depthWriteEnabled = false;
	m_depthStencilDescriptor.depthCompareFunction = MTLCompareFunctionLessEqual;

	for( int i = 0; i < METAL_DEPTHSTENCIL_CACHE_SIZE; ++i )
	{
		m_depthStencilStateCache[i] = nil;
	}

#if 0
	for (int i = 0; i < METAL_MAX_RENDER_TARGETS; i++)
	{
		m_clearState.colorLoadAction[i]  = MTLLoadActionLoad;
		m_clearState.colorStoreAction[i] = MTLStoreActionStore;
		m_clearState.clearColorValue[i]  = MTLClearColor {0.0f, 0.0f, 0.0f, 0.0f};
	}

	m_clearState.depthLoadAction  = MTLLoadActionLoad;
	m_clearState.depthStoreAction = MTLStoreActionStore;
	m_clearState.clearDepthValue  = 0.0f;

	m_clearState.stencilLoadAction  = MTLLoadActionLoad;
	m_clearState.stencilStoreAction = MTLStoreActionStore;
	m_clearState.clearStencilValue  = 0;
#endif

	m_frontFaceStencilDescriptor = [[MTLStencilDescriptor alloc] init];
	m_backFaceStencilDescriptor = [[MTLStencilDescriptor alloc] init];
	m_drawRenderPassDescriptor = [[MTLRenderPassDescriptor alloc] init];
	m_blitAndPresentRenderPassDescriptor = [[MTLRenderPassDescriptor alloc] init];

	for( int i = 0; i < METAL_VERTEX_STREAM_BUFFER_COUNT; ++i )
	{
		m_boundVertexStreams[i].stride = 0;
	}

	const ShaderType shaderTypes[] = { VERTEX_SHADER, PIXEL_SHADER, COMPUTE_SHADER };
	for( auto shaderType : shaderTypes )
	{
		ResetBuffers( shaderType );
		ResetTextures( shaderType );
		ResetSamplers( shaderType );
	}

	m_vertexFunction = nil;
	m_fragmentFunction = nil;
	m_computeFunction = nil;
	m_shaderResourceMasks = nullptr;

	m_clearBufferComputeFunctions[0] = nil;
	m_clearBufferComputeFunctions[1] = nil;
	m_clearTextureComputeFunctions[0] = nil;
	m_clearTextureComputeFunctions[1] = nil;

	m_visibilityBuffer = nil;
	m_maxVisibilityQueries = 0;
	m_currentVisibilityQueryIndex = 0;
	m_numCompletedVisibilityQueries = 0;
	m_visibilityQueryInProgress = false;

	ResetFrame();
	ResetWorkQueue();
}

MetalWorkQueue::~MetalWorkQueue()
{
	METAL_LOG( @"Log:~MetalWorkQueue()" );

	// Flush all outstanding work and wait until it's done.
	if( m_commandBuffer )
	{
		if( !m_encoderEnded && m_encoderHasWork )
		{
			// Release and terminate the current encoder.
			ReleaseEncoder( true );
		}

		[m_commandBuffer commit];
		[m_commandBuffer waitUntilCompleted];
	}

#if !__has_feature( objc_arc )
	[m_depthStencilDescriptor release];
	[m_frontFaceStencilDescriptor release];
	[m_backFaceStencilDescriptor release];
	[m_drawRenderPassDescriptor release];
	[m_blitAndPresentRenderPassDescriptor release];

	for( int i = 0; i < METAL_DEPTHSTENCIL_CACHE_SIZE; ++i )
	{
		[m_depthStencilStateCache[i] release];
	}

	if( m_presentBlitPipeline )
	{
		[m_presentBlitPipeline release];
	}
	if( m_presentBlitPipelineDesciptor )
	{
		[m_presentBlitPipelineDesciptor release];
	}
#endif
}

void MetalWorkQueue::SetMetalContext( MetalContext* metalContext )
{
	m_context = metalContext;
}

void MetalWorkQueue::InitRenderPassDescriptor()
{
	CCP_ASSERT( m_isPrimary );

	METAL_LOG( @"Log:InitRenderPassDescriptor" );
	for( int i = 0; i < METAL_MAX_RENDER_TARGETS; i++ )
	{
		m_drawRenderPassDescriptor.colorAttachments[i].clearColor = MTLClearColorMake( 0.0f, 0.0f, 0.0f, 0.0f );
		m_drawRenderPassDescriptor.colorAttachments[i].loadAction = MTLLoadActionDontCare;
		m_drawRenderPassDescriptor.colorAttachments[i].storeAction = MTLStoreActionDontCare;
		m_drawRenderPassDescriptor.colorAttachments[i].texture = nil;
	}

	m_drawRenderPassDescriptor.depthAttachment.clearDepth = 0;
	m_drawRenderPassDescriptor.depthAttachment.loadAction = MTLLoadActionDontCare;
	m_drawRenderPassDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
	m_drawRenderPassDescriptor.depthAttachment.texture = nil;

	m_drawRenderPassDescriptor.stencilAttachment.clearStencil = 0;
	m_drawRenderPassDescriptor.stencilAttachment.loadAction = MTLLoadActionDontCare;
	m_drawRenderPassDescriptor.stencilAttachment.storeAction = MTLStoreActionDontCare;
	m_drawRenderPassDescriptor.stencilAttachment.texture = nil;

	m_drawRenderPassDescriptor.visibilityResultBuffer = m_visibilityBuffer;

	m_numRenderAttachments = 0;
	m_numCommands = 0;

	m_blitAndPresentRenderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
	m_blitAndPresentRenderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake( 0.0, 0.0, 0.0, 1.0 );
}

void MetalWorkQueue::CreateClearFunctions()
{
	CCP_ASSERT( m_isPrimary );
	NSError* error;
	NSString* sClearShaderSourceString = [[NSString alloc] initWithUTF8String:sClearShaderSource];
	id<MTLLibrary> shaderLib = [m_device newLibraryWithSource:sClearShaderSourceString options:nil error:&error];

	if( !shaderLib )
	{
		CCP_AL_LOGERR( "Couldn't create shader library. Error: %s", error.localizedDescription.UTF8String );
		// assert here becuase if the shader libary isn't loading, nothing good will happen
		CCP_ASSERT( shaderLib != nil );
	}

	// for (int i = 0; i < shaderLib.functionNames.count; i++)
	// {
	// 	METAL_LOG(@"Found function %@", shaderLib.functionNames[i]);
	// }

	m_clearBufferComputeFunctions[0] = [shaderLib newFunctionWithName:@"ClearFloatBuffer"];
	if( !m_clearBufferComputeFunctions[0] )
	{
		CCP_AL_LOGERR( "Couldn't load ClearFloatBuffer function from library." );
		CCP_ASSERT( false );
	}

	m_clearBufferComputeFunctions[1] = [shaderLib newFunctionWithName:@"ClearUIntBuffer"];
	if( !m_clearBufferComputeFunctions[1] )
	{
		CCP_AL_LOGERR( "Couldn't load ClearUIntBuffer function from library." );
		CCP_ASSERT( false );
	}

	m_clearTextureComputeFunctions[0] = [shaderLib newFunctionWithName:@"ClearFloatTexture"];
	if( !m_clearTextureComputeFunctions[0] )
	{
		CCP_AL_LOGERR( "Couldn't load ClearFloatTexture function from library." );
		CCP_ASSERT( false );
	}

	m_clearTextureComputeFunctions[1] = [shaderLib newFunctionWithName:@"ClearUIntTexture"];
	if( !m_clearTextureComputeFunctions[1] )
	{
		CCP_AL_LOGERR( "Couldn't load ClearUIntTexture function from library." );
		CCP_ASSERT( false );
	}
}

void MetalWorkQueue::SetCommandQueue( id<MTLCommandQueue> commandQueue )
{
	CCP_ASSERT( m_isPrimary );

	m_commandQueue = commandQueue;
	m_device = commandQueue.device;

	m_isIntelGpu = [m_device.name containsString:@"Intel"];
	if( @available( macOS 10.15, * ) )
	{
		m_isAppleSilicon = [m_device supportsFamily:MTLGPUFamilyMac2] && [m_device supportsFamily:MTLGPUFamilyApple7];
	}

	SetupPresentBlitResources();
	CreateClearFunctions();
}

void MetalWorkQueue::ResetWorkQueue()
{
	m_commandBuffer = nil;

	m_encoderInUse = false;
	m_encoderEnded = false;
	m_encoderHasWork = false;
	m_currentEncoderType = MTLENCODERTYPE_NONE;
	m_currentBlitEncoder = nil;
	m_currentRenderEncoder = nil;
	m_currentComputeEncoder = nil;
	if( @available( macOS 11.0, * ) )
	{
		m_currentAccelerationStructureEncoder = nil;
	}
	m_generatesEndOfQueueEvent = false;
	m_pendingClear = false;

	// Can't reset vertex descriptor because operations like synchronising resources cause command buffer flushes
	// and we need to retain this. Probably applies to other state too so may have to rethink the point at which call this fn.
	// m_currentVertexDescriptor              = nil;
	// m_currentVertexDescriptorHash          = 0;
	m_currentAttachmentsHash = 0;
	m_currentRenderPipelineDescriptorHash = 0;
	m_currentRenderPipelineState = nil;
	m_currentDepthStencilDescriptorHash = 0;
	m_currentDepthStencilState = nil;
	m_currentComputePipelineDescriptorHash = 0;
	m_currentComputePipelineState = nil;
	m_currentRenderPassDescriptor = m_drawRenderPassDescriptor;
	m_visibilityQueriesInThisEncoder = 0;

	m_numCommands = 0;
}

void MetalWorkQueue::ResetFrame()
{
	CCP_ASSERT( m_isPrimary );

	m_pipelineQueriesInProgress.clear();

	for( int i = 0; i < METAL_VERTEX_STREAM_BUFFER_COUNT; ++i )
	{
		m_boundVertexStreams[i].stride = 0;
	}

	const ShaderType shaderTypes[] = { VERTEX_SHADER, PIXEL_SHADER, COMPUTE_SHADER };
	for( auto shaderType : shaderTypes )
	{
		ResetBuffers( shaderType );
		ResetTextures( shaderType );
		ResetSamplers( shaderType );
	}

	InitRenderPassDescriptor();
}

void MetalWorkQueue::CreateCommandBuffer()
{
	CCP_ASSERT( m_isPrimary );

	if( m_commandBuffer == nil )
	{
		m_commandBuffer = [m_commandQueue commandBuffer];
#if !__has_feature( objc_arc )
		[m_commandBuffer retain];
#endif
	}
	// We'll reuse an existing buffer silently if it's empty, otherwise emit warning
	else if( m_encoderHasWork )
	{
		CCP_AL_LOGWARN( "Command buffer already exists." );
	}
}

void MetalWorkQueue::FlushOutstandingOperations()
{
	CCP_ASSERT( m_isPrimary );

	// The tests just do a present with no render
	if( !m_commandBuffer )
	{
		CreateCommandBuffer();
	}

	if( !m_encoderEnded && m_encoderHasWork )
	{
		// Release and terminate the current encoder
		ReleaseEncoder( true );
	}

	// Flush any outstanding clears on the current surface
	if( m_pendingClear )
	{
		auto bkHasPendingRenderPassHint = m_hasPendingRenderPassHint;
		m_hasPendingRenderPassHint = false;
		GetRenderEncoder( @"Flush clears encoder" );
		ReleaseEncoder( true );
		m_hasPendingRenderPassHint = bkHasPendingRenderPassHint;
	}
}

void MetalWorkQueue::EndCurrentRenderPass()
{
	FlushOutstandingOperations();
}

void MetalWorkQueue::RenderTargetBarrier()
{
	CCP_ASSERT( m_isPrimary );

	if( !m_currentRenderEncoder )
	{
		return;
	}

	if( m_isAppleSilicon )
	{
		GetRenderEncoder();
		ReleaseEncoder( true );
	}
	else
	{
		m_hasPendingRenderTargetBarrier = true;
	}
}

void MetalWorkQueue::CommitCommandBuffer( MetalCBCommitFlags flags )
{
	CCP_ASSERT( m_isPrimary );

	// Check if there was any work to submit on this queue
	if( m_commandBuffer == nil )
	{
		CCP_AL_LOGERR( "Can't commit command buffer if it was never created." );
		return;
	}

	// Check that we actually encoded something to commit..
	if( m_encoderHasWork )
	{
		if( m_encoderInUse )
		{
			CCP_AL_LOGERR( "Can't commit command buffer if encoder is still in use." );
		}

		// If the last used encoder wasn't ended then we need to end it now
		if( !m_encoderEnded )
		{
			m_encoderInUse = true;
			ReleaseEncoder( true );
		}
		METAL_LOG( @"Committing command buffer: %@", m_commandBuffer.label );
	}

	// Only schedule a callbvack if we have something to do.
	if( m_visibilityQueriesInThisEncoder )
	{
		__block uint64_t block_numCompletedVisibilityQueries = m_currentVisibilityQueryIndex;
		[m_commandBuffer addCompletedHandler:^( id<MTLCommandBuffer> buffer ) {
			m_numCompletedVisibilityQueries = block_numCompletedVisibilityQueries;
			METAL_LOG( @"Visibvility queries < (%llu) now valid", m_numCompletedVisibilityQueries );
		}];
	}

	[m_commandBuffer commit];

	if( ( flags & MTLCBCOMMIT_WAITUNTILSCHEDULED ) && m_encoderHasWork )
	{
		[m_commandBuffer waitUntilScheduled];
	}
	else if( ( flags & MTLCBCOMMIT_WAITUNTILCOMPLETE ) && m_encoderHasWork )
	{
		[m_commandBuffer waitUntilCompleted];
	}

#if !__has_feature( objc_arc )
	[m_commandBuffer release];
#endif

	ResetWorkQueue();
}

void MetalWorkQueue::BeginFrame()
{
	CCP_ASSERT( m_isPrimary );

	METAL_LOG( @"Log:------- BeginFrame() --------" );
}

void MetalWorkQueue::EndFrame()
{
	CCP_ASSERT( m_isPrimary );

	FlushOutstandingOperations();

	// Reset state at end of frame, not beginning of frame as some setup occurs prior to BeginScene being called
	// Probably not required now as we're calling ResetRenderTargets from the RenderContext
	// InitRenderPassDescriptor();
	m_dirtyRenderEncoderState = METAL_RENDERENCODERDIRTYSTATE_ALL;

	METAL_LOG( @"Log:------- EndFrame() -------" );
	ResetFrame();
}

void MetalWorkQueue::SetupPresentBlitResources()
{
	CCP_ASSERT( m_isPrimary );

	NSError* error;
	NSString* sBlitShaderSourceString = [[NSString alloc] initWithUTF8String:sBlitShaderSource];
	id<MTLLibrary> shaderLib = [m_device newLibraryWithSource:sBlitShaderSourceString options:nil error:&error];

	if( !shaderLib )
	{
		CCP_AL_LOGERR( "Couldnt create a default shader library. Error: %s", error.localizedDescription.UTF8String );
		// assert here becuase if the shader libary isn't loading, nothing good will happen
		CCP_ASSERT( shaderLib != nil );
	}

	id<MTLFunction> vertexProgram = [shaderLib newFunctionWithName:@"blitPresentVertexShader"];
	if( !vertexProgram )
	{
		CCP_AL_LOGERR( "Couldn't load vertex function from default library." );
		CCP_ASSERT( vertexProgram != nil );
	}

	id<MTLFunction> fragmentProgram = [shaderLib newFunctionWithName:@"blitPresentFragmentShader"];
	if( !fragmentProgram )
	{
		CCP_AL_LOGERR( "Couldn't load fragment function from default library." );
		CCP_ASSERT( fragmentProgram != nil );
	}

	// Create a pipeline state descriptor which can be used to create a compiled pipeline state object
	m_presentBlitPipelineDesciptor = [[MTLRenderPipelineDescriptor alloc] init];

	m_presentBlitPipelineDesciptor.label = @"PresentBlitPipeline";
	m_presentBlitPipelineDesciptor.vertexFunction = vertexProgram;
	m_presentBlitPipelineDesciptor.fragmentFunction = fragmentProgram;
	m_presentBlitPipelineDesciptor.colorAttachments[0].pixelFormat = MTLPixelFormatInvalid;

	m_presentBlitPipeline = nil;
	m_numCommands = 0;
}

bool MetalWorkQueue::BlitToDrawableAndPresent( id<MTLTexture> srcTexture, NSView* view, uint64_t* renderedFrameNumber )
{
	CCP_ASSERT( m_isPrimary );

	METAL_LOG( @"Log:BlitToDrawableAndPresent" );

	dispatch_semaphore_wait( m_frameSemaphore, DISPATCH_TIME_FOREVER );

	// JM - this should not be done every blit but rather setup only when something changes.
	// Probably from the SwapChain or PresentParameters code.
	CAMetalLayer* caMetalLayer = (CAMetalLayer*)view.layer;
	caMetalLayer.device = m_device;
	caMetalLayer.pixelFormat = srcTexture.pixelFormat;
	id<CAMetalDrawable> drawable = [caMetalLayer nextDrawable];

	CCP_ASSERT( view != nil && caMetalLayer != nil && drawable != nil );

	if( view == nil || caMetalLayer == nil || drawable == nil )
	{
		if( view == nil )
		{
			CCP_AL_LOGERR( "No target view - present failed." );
		}

		if( caMetalLayer == nil )
		{
			CCP_AL_LOGERR( "Target view doesn't have a Metal layer - present failed." );
		}

		if( drawable == nil )
		{
			CCP_AL_LOGERR( "Next drawable is not available - present failed." );
		}

		return false;
	}

	FlushOutstandingOperations();

	m_blitAndPresentRenderPassDescriptor.colorAttachments[0].texture = drawable.texture;
	m_currentRenderPassDescriptor = m_blitAndPresentRenderPassDescriptor;

	// Create the render command encoder from the descriptor we just set up.
	id<MTLRenderCommandEncoder> renderEncoder = GetRenderEncoder( @"Present Blit" );

	// Check if we need to create a new pipeline
	if( m_presentBlitPipelineDesciptor.colorAttachments[0].pixelFormat != drawable.texture.pixelFormat )
	{
#if !__has_feature( objc_arc )
		if( m_presentBlitPipeline )
		{
			[m_presentBlitPipeline release];
		}
#endif
		m_presentBlitPipelineDesciptor.colorAttachments[0].pixelFormat = drawable.texture.pixelFormat;

		NSError* error;

		m_presentBlitPipeline = [m_device newRenderPipelineStateWithDescriptor:m_presentBlitPipelineDesciptor
																		 error:&error];
		if( !m_presentBlitPipeline )
		{
			CCP_AL_LOGERR( "Failed aquiring pipeline state. Error: %s", error.localizedDescription.UTF8String );
#if !__has_feature( objc_arc )
			[error release];
#endif
			return false;
		}
	}

	[renderEncoder setRenderPipelineState:m_presentBlitPipeline];

	static const BlitPresentVertex quad[] = { (vector_float2){ -1.0f, 1.0f },  (vector_float2){ 0.0, 0.0f },
											  (vector_float2){ 1.0f, 1.0f },   (vector_float2){ 1.0f, 0.0f },
											  (vector_float2){ -1.0f, -1.0f }, (vector_float2){ 0.0f, 1.0f },
											  (vector_float2){ 1.0f, -1.0f },  (vector_float2){ 1.0f, 1.0f } };
	uint32_t quadSizeInBytes = sizeof( quad );

	[renderEncoder setVertexBytes:&quad length:quadSizeInBytes atIndex:BlitPresentInputIndexVertices];

	[renderEncoder setFragmentTexture:srcTexture atIndex:BlitPresentFragmentInputIndexTexture];
	[renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];

	ReleaseEncoder( true );

#define METAL_SERIALISE_PRESENT 0

#if METAL_SERIALISE_PRESENT
	[m_commandBuffer addCompletedHandler:^( id<MTLCommandBuffer> commandBuffer ) {
		if( @available( macOS 10.15, * ) )
		{
			auto duration = commandBuffer.GPUEndTime - commandBuffer.GPUStartTime;
			s_gpuFrameTime += duration;
#if CCP_STATS_ENABLED
			g_ccpStatistics_gpuFrameTime.Set( s_gpuFrameTime );
#endif
			s_gpuFrameTime = 0;
		}
	}];
	[m_commandBuffer presentDrawable:drawable];
	CommitCommandBuffer( MTLCBCOMMIT_WAITUNTILCOMPLETE );
	++*renderedFrameNumber;
#else
	__block dispatch_semaphore_t frameSemaphore = m_frameSemaphore;
	[m_commandBuffer addCompletedHandler:^( id<MTLCommandBuffer> commandBuffer ) {
		if( @available( macOS 10.15, * ) )
		{
			auto duration = commandBuffer.GPUEndTime - commandBuffer.GPUStartTime;
			s_gpuFrameTime += duration;
#if CCP_STATS_ENABLED
			g_ccpStatistics_gpuFrameTime.Set( s_gpuFrameTime );
#endif
			s_gpuFrameTime = 0;
		}
		++*renderedFrameNumber;
		dispatch_semaphore_signal( frameSemaphore );
	}];
	[m_commandBuffer presentDrawable:drawable];
	CommitCommandBuffer( MTLCBCOMMIT_NOFLAGS );
#endif

	m_currentRenderPassDescriptor = m_drawRenderPassDescriptor;
	return true;
}

void MetalWorkQueue::EndEncoder()
{
	if( m_encoderInUse || !m_encoderEnded )
	{
		ReleaseEncoder( true );
	}
}

void MetalWorkQueue::ReleaseEncoder( bool endEncoding )
{
	if( !m_encoderInUse && !endEncoding )
	{
		CCP_AL_LOGERR( "No encoder to release." );
		return;
	}
	if( !m_commandBuffer )
	{
		CCP_AL_LOGERR( "Shouldn't be able to get here without having a command buffer created." );
		return;
	}

	// Automatically create a new render encoder every 'IntelCommandCountPerEncoder'.
	// This is only done on Intel GPU to save frametime.
	static const int32_t IntelCommandCountPerEncoder = 32;

	m_numCommands++;
	if( m_isIntelGpu && m_isPrimary )
	{
		if( m_numCommands == IntelCommandCountPerEncoder )
		{
			m_numCommands = 0;
			endEncoding = true;
		}
	}

	if( endEncoding )
	{
		for( auto query : m_pipelineQueriesInProgress )
		{
			query->EncoderEnding( this );
		}

		m_hasPendingRenderTargetBarrier = false;

		switch( m_currentEncoderType )
		{
		case MTLENCODERTYPE_RENDER: {
			METAL_LOG( @"Log:Releasing and ending render encoder %@", m_currentRenderEncoder.label );
			CCP_ASSERT( !m_visibilityQueryInProgress );

			[m_currentRenderEncoder endEncoding];
			m_currentRenderEncoder = nil;
			m_currentRenderPipelineState = nil;
			break;
		}
		case MTLENCODERTYPE_COMPUTE: {
			CCP_ASSERT( m_isPrimary );

			METAL_LOG( @"Log:Releasing and ending compute encoder %@", m_currentComputeEncoder.label );
			[m_currentComputeEncoder endEncoding];
			m_currentComputePipelineState = nil;
			m_threadGroupSize = MTLSizeMake( 1, 1, 1 );
			m_currentComputeEncoder = nil;
			break;
		}
		case MTLENCODERTYPE_BLIT: {
			CCP_ASSERT( m_isPrimary );

			METAL_LOG( @"Log:Releasing and ending blit encoder %@", m_currentBlitEncoder.label );
			[m_currentBlitEncoder endEncoding];
			m_currentBlitEncoder = nil;
			break;
		}
		case MTLENCODERTYPE_ACCELERATION_STRUCTURE: {
			if( @available( macOS 11.0, * ) )
			{
				CCP_ASSERT( m_isPrimary );

				METAL_LOG( @"Log:Releasing and ending acceleration structure encoder %@",
						   m_currentAccelerationStructureEncoder.label );
				[m_currentAccelerationStructureEncoder endEncoding];
				m_currentAccelerationStructureEncoder = nil;
			}
			break;
		}
		case MTLENCODERTYPE_NONE:
		default:
			CCP_ASSERT( m_isPrimary );
			// CCP_AL_LOGERR( "Unsupported encoder type to flush." );
			break;
		}
		m_currentEncoderType = MTLENCODERTYPE_NONE;
		m_encoderEnded = true;
	}
	m_encoderInUse = false;
}

void MetalWorkQueue::ApplyRenderPassHint()
{
	CCP_ASSERT( m_isPrimary );

	// Check we have a valid render pass descriptor
	if( !m_currentRenderPassDescriptor )
	{
		CCP_AL_LOGERR( "Can ony pass null renderPassDescriptor if the render encoder is currently active." );
	}

	if( m_hasPendingRenderPassHint )
	{
		if( m_currentRenderPassDescriptor.depthAttachment.texture )
		{
			if( m_currentRenderPassDescriptor.depthAttachment.loadAction != MTLLoadActionClear )
			{
				m_currentRenderPassDescriptor.depthAttachment.loadAction = m_pendingRenderPassHint.depth.load;
			}
			m_currentRenderPassDescriptor.depthAttachment.storeAction = m_pendingRenderPassHint.depth.store;
			m_currentRenderPassDescriptor.depthAttachment.clearDepth = m_pendingRenderPassHint.depth.clearValue;
		}
		for( int i = 0; i < METAL_MAX_RENDER_TARGETS; ++i )
		{
			if( m_currentRenderPassDescriptor.colorAttachments[i].texture )
			{
				if( m_currentRenderPassDescriptor.colorAttachments[i].loadAction != MTLLoadActionClear )
				{
					m_currentRenderPassDescriptor.colorAttachments[i].loadAction =
						m_pendingRenderPassHint.color[i].load;
				}
				m_currentRenderPassDescriptor.colorAttachments[i].storeAction = m_pendingRenderPassHint.color[i].store;
				m_currentRenderPassDescriptor.colorAttachments[i].clearColor =
					m_pendingRenderPassHint.color[i].clearColor;
			}
		}
		m_hasPendingRenderPassHint = false;
	}
}

void MetalWorkQueue::SetRenderStatesDirty()
{
	m_dirtyRenderEncoderState = METAL_RENDERENCODERDIRTYSTATE_ALL;

	m_dirtyConstBuffersMask[VERTEX_SHADER] = ~0u;
	m_dirtyConstBufferPageMask[VERTEX_SHADER] = ~0u;
	m_dirtyBuffersMask[VERTEX_SHADER] = ~0u;
	m_dirtyTexturesMask[VERTEX_SHADER] = ~0u;
	m_dirtySamplersMask[VERTEX_SHADER] = ~0u;

	m_dirtyConstBuffersMask[PIXEL_SHADER] = ~0u;
	m_dirtyConstBufferPageMask[PIXEL_SHADER] = ~0u;
	m_dirtyBuffersMask[PIXEL_SHADER] = ~0u;
	m_dirtyTexturesMask[PIXEL_SHADER] = ~0u;
	m_dirtySamplersMask[PIXEL_SHADER] = ~0u;
}

void MetalWorkQueue::MarkConstantBuffersDirty()
{
	m_dirtyConstBuffersMask[VERTEX_SHADER] = ~0u;
	m_dirtyConstBufferPageMask[VERTEX_SHADER] = ~0u;

	m_dirtyConstBuffersMask[PIXEL_SHADER] = ~0u;
	m_dirtyConstBufferPageMask[PIXEL_SHADER] = ~0u;

	m_dirtyConstBuffersMask[COMPUTE_SHADER] = ~0u;
	m_dirtyConstBufferPageMask[COMPUTE_SHADER] = ~0u;
}

void MetalWorkQueue::SetCurrentEncoder( MetalEncoderType encoderType, NSString* encoderLabel )
{
	if( m_encoderInUse )
	{
		CCP_AL_LOGERR( "Need to release the current encoder before getting a new one." );
	}
	if( !m_commandBuffer )
	{
		CreateCommandBuffer();
	}

	if( encoderType != MTLENCODERTYPE_RENDER && m_hasPendingRenderPassHint )
	{
		GetRenderEncoder();
		ReleaseEncoder( true );
	}

	// Check if we have a cuurently active encoder
	if( m_currentEncoderType != MTLENCODERTYPE_NONE )
	{
		// If the encoder types match then we can carry on using the current encoder otherwise we need to create a new one
		if( m_currentEncoderType == encoderType )
		{
			if( encoderType == MTLENCODERTYPE_RENDER && m_hasPendingRenderTargetBarrier )
			{
				[m_currentRenderEncoder memoryBarrierWithScope:MTLBarrierScopeRenderTargets
												   afterStages:MTLRenderStageFragment
												  beforeStages:MTLRenderStageVertex];
				m_hasPendingRenderTargetBarrier = false;
			}
			// Mark that this curent encoder is in use and return
			m_encoderInUse = true;
			return;
		}
		// If we're switching encoder types then we should end the current one
		else if( m_currentEncoderType != encoderType && !m_encoderEnded )
		{
			// Mark encoder in use so we can do release+end
			m_encoderInUse = true;
			ReleaseEncoder( true );
		}
	}

	CCP_ASSERT( m_isPrimary );
	// Create a new encoder
	switch( encoderType )
	{
	case MTLENCODERTYPE_RENDER: {
		ApplyRenderPassHint();

		if( !m_isPrimary )
		{
			CCP_AL_LOGERR( "We can't create encoder in this cotext" );
		}

		m_currentRenderEncoder = [m_commandBuffer renderCommandEncoderWithDescriptor:m_currentRenderPassDescriptor];
		m_currentRenderEncoder.label = encoderLabel ? encoderLabel : @"Standard render encoder";
		METAL_LOG( @"Log:SetCurrentEncoder(Render) %@", m_currentRenderEncoder.label );

		// Since the encoder is new we'll need to emit all the state again
		SetRenderStatesDirty();

		// We should remove any clear state just processed by this render encoder
		ResetClearState();

		++m_encoderIndex;

		break;
	}
	case MTLENCODERTYPE_COMPUTE: {
#if 0
		if( m_concurrentDispatchSupported )
		{
			m_currentComputeEncoder = [m_commandBuffer computeCommandEncoderWithDispatchType:MTLDispatchTypeConcurrent];
		}
		else
#endif
		{
			m_currentComputeEncoder = [m_commandBuffer computeCommandEncoder];
		}
		m_currentComputeEncoder.label = encoderLabel ? encoderLabel : @"Standard compute encoder";
		METAL_LOG( @"Log:SetCurrentEncoder(Compute) %@", m_currentBlitEncoder.label );

		m_dirtyConstBuffersMask[COMPUTE_SHADER] = ~0u;
		m_dirtyConstBufferPageMask[COMPUTE_SHADER] = ~0u;
		m_dirtyBuffersMask[COMPUTE_SHADER] = ~0u;
		m_dirtyTexturesMask[COMPUTE_SHADER] = ~0u;
		m_dirtySamplersMask[COMPUTE_SHADER] = ~0u;

		++m_encoderIndex;

		break;
	}
	case MTLENCODERTYPE_BLIT: {
		m_currentBlitEncoder = [m_commandBuffer blitCommandEncoder];
		m_currentBlitEncoder.label = encoderLabel ? encoderLabel : @"Standard blit encoder";
		METAL_LOG( @"Log:SetCurrentEncoder(Blit) %@", m_currentBlitEncoder.label );

		++m_encoderIndex;
		break;
	}
	case MTLENCODERTYPE_ACCELERATION_STRUCTURE: {
		if( @available( macOS 11.0, * ) )
		{
			m_currentAccelerationStructureEncoder = [m_commandBuffer accelerationStructureCommandEncoder];
			m_currentAccelerationStructureEncoder.label =
				encoderLabel ? encoderLabel : @"Standard acceleration structure encoder";
			METAL_LOG( @"Log:SetCurrentEncoder(AccelerationStructure) %@",
					   m_currentAccelerationStructureEncoder.label );

			++m_encoderIndex;
		}
		break;
	}
	case MTLENCODERTYPE_NONE:
	default:
		CCP_AL_LOGERR( "Invalid encoder type!" );
	}

	// Mark that this curent encoder is in use
	m_currentEncoderType = encoderType;
	m_encoderInUse = true;
	m_encoderEnded = false;
	m_encoderHasWork = true;

	m_numCommands = 0;

	for( auto query : m_pipelineQueriesInProgress )
	{
		query->EncoderStarted( this );
	}
}

id<MTLBlitCommandEncoder> MetalWorkQueue::GetBlitEncoder( NSString* encoderLabel )
{
	CCP_ASSERT( m_isPrimary );

	SetCurrentEncoder( MTLENCODERTYPE_BLIT );
	return m_currentBlitEncoder;
}

id<MTLComputeCommandEncoder> MetalWorkQueue::GetComputeEncoder( NSString* encoderLabel )
{
	CCP_ASSERT( m_isPrimary );

	SetCurrentEncoder( MTLENCODERTYPE_COMPUTE );
	return m_currentComputeEncoder;
}

// If a renderpass descriptor is provided a new render encoder will be created otherwise we'll use the current one
id<MTLRenderCommandEncoder> MetalWorkQueue::GetRenderEncoder( NSString* encoderLabel )
{
	SetCurrentEncoder( MTLENCODERTYPE_RENDER, encoderLabel );
	return m_currentRenderEncoder;
}

id<MTLAccelerationStructureCommandEncoder> MetalWorkQueue::GetAccelerationStructureEncoder( NSString* encoderLabel )
{
	SetCurrentEncoder( MTLENCODERTYPE_ACCELERATION_STRUCTURE, encoderLabel );
	return m_currentAccelerationStructureEncoder;
}

id<MTLParallelRenderCommandEncoder> MetalWorkQueue::GetParallelEncoder( NSString* encoderLabel )
{
	if( m_currentParallelEncoder )
	{
		return m_currentParallelEncoder;
	}

	ApplyRenderPassHint();

	m_currentParallelEncoder =
		[m_commandBuffer parallelRenderCommandEncoderWithDescriptor:m_currentRenderPassDescriptor];
	m_currentParallelEncoder.label = encoderLabel ? encoderLabel : @"Standard parallel encoder";
	METAL_LOG( @"Log:CreateParallelEncoder(Render) %@", m_currentParallelEncoder.label );

	ResetClearState();

	return m_currentParallelEncoder;
}

void MetalWorkQueue::CopyDataToBuffer( id<MTLBuffer> buffer, const void* data, size_t offset, size_t sizeInBytes )
{
	if( buffer.storageMode == MTLResourceStorageModePrivate )
	{
		CCP_AL_LOGERR( "MTLResourceStorageModePrivate is not supported yet." );
	}
	else
	{
		void* offsetDest = (void*)&( (char*)buffer.contents )[offset];

		memcpy( offsetDest, data, sizeInBytes );

		[buffer didModifyRange:NSMakeRange( offset, sizeInBytes )];
	}
}

void MetalWorkQueue::CopyBufferToBuffer( id<MTLBuffer> dest,
										 size_t destOffset,
										 id<MTLBuffer> src,
										 size_t srcOffset,
										 size_t size )
{
	CCP_ASSERT( m_isPrimary );
	id<MTLBlitCommandEncoder> blitEncoder = GetBlitEncoder();
	[blitEncoder copyFromBuffer:src sourceOffset:srcOffset toBuffer:dest destinationOffset:destOffset size:size];
	ReleaseEncoder( false );
}

void MetalWorkQueue::ReadBackBufferToCPU( id<MTLBuffer> buffer, bool waitForData )
{
	CCP_ASSERT( m_isPrimary );
	FlushOutstandingOperations();

	id<MTLBlitCommandEncoder> blitEncoder = GetBlitEncoder();

	[blitEncoder synchronizeResource:buffer]; // sync to CPU

	if( waitForData )
	{
		ReleaseEncoder( true );
		[m_commandBuffer addCompletedHandler:^( id<MTLCommandBuffer> commandBuffer ) {
			if( @available( macOS 10.15, * ) )
			{
				auto duration = commandBuffer.GPUEndTime - commandBuffer.GPUStartTime;
				s_gpuFrameTime += duration;
			}
		}];
		CommitCommandBuffer( MTLCBCOMMIT_WAITUNTILCOMPLETE );
	}
	else
	{
		ReleaseEncoder( false );
	}
}

void MetalWorkQueue::UploadTexture( id<MTLTexture> texture,
									id<MTLBuffer> srcBuffer,
									uint32_t slice,
									uint32_t mip,
									uint32_t bytesPerRow,
									uint32_t bytesPerSlice,
									uint32_t srcOffset,
									bool syncOnBuffer )
{
	CCP_ASSERT( m_isPrimary );
	// Do we need to wait for some operations on the buffer to complete?
	if( syncOnBuffer )
	{
		FlushOutstandingOperations();
	}

	id<MTLBlitCommandEncoder> blitEncoder = GetBlitEncoder();

	MTLSize dimensions =
		MTLSizeMake( MAX( texture.width >> mip, 1 ), MAX( texture.height >> mip, 1 ), MAX( texture.depth >> mip, 1 ) );
	MTLOrigin origin = MTLOriginMake( 0, 0, 0 );

	[blitEncoder copyFromBuffer:srcBuffer
				   sourceOffset:srcOffset
			  sourceBytesPerRow:bytesPerRow
			sourceBytesPerImage:bytesPerSlice
					 sourceSize:dimensions
					  toTexture:texture
			   destinationSlice:slice
			   destinationLevel:mip
			  destinationOrigin:origin];

	ReleaseEncoder( false );
}

void MetalWorkQueue::UploadTexture( id<MTLTexture> texture,
									id<MTLBuffer> srcBuffer,
									uint32_t slice,
									uint32_t mip,
									const MTLOrigin& origin,
									const MTLSize& size,
									uint32_t bytesPerRow,
									uint32_t bytesPerSlice,
									uint32_t srcOffset,
									bool syncOnBuffer )
{
	CCP_ASSERT( m_isPrimary );
	// Do we need to wait for some operations on the buffer to complete?
	if( syncOnBuffer )
	{
		FlushOutstandingOperations();
	}

	id<MTLBlitCommandEncoder> blitEncoder = GetBlitEncoder();

	[blitEncoder copyFromBuffer:srcBuffer
				   sourceOffset:srcOffset
			  sourceBytesPerRow:bytesPerRow
			sourceBytesPerImage:bytesPerSlice
					 sourceSize:size
					  toTexture:texture
			   destinationSlice:slice
			   destinationLevel:mip
			  destinationOrigin:origin];

	ReleaseEncoder( false );
}

void MetalWorkQueue::UploadTexture( id<MTLTexture> texture,
									const void* srcData,
									uint32_t slice,
									uint32_t mip,
									uint32_t bytesPerRow,
									uint32_t bytesPerSlice,
									uint32_t srcOffset,
									bool syncOnBuffer )
{
	CCP_ASSERT( m_isPrimary );
	size_t bytesPerTexture = MAX( texture.depth >> mip, 1 ) * bytesPerSlice;

	id<MTLBuffer> srcBuffer = [m_device newBufferWithBytes:srcData
													length:bytesPerTexture
												   options:MTLResourceStorageModeShared];

	UploadTexture( texture, srcBuffer, slice, mip, bytesPerRow, bytesPerSlice, srcOffset, syncOnBuffer );

#if !__has_feature( objc_arc )
	[srcBuffer release];
#endif
}

void MetalWorkQueue::GenerateMipMaps( id<MTLTexture> texture )
{
	CCP_ASSERT( m_isPrimary );
	if( texture.mipmapLevelCount > 1 )
	{
		id<MTLBlitCommandEncoder> blitEncoder = GetBlitEncoder();
		[blitEncoder generateMipmapsForTexture:texture];

		ReleaseEncoder( false );
	}
}

void MetalWorkQueue::ClearBuffer( id<MTLBuffer> buffer, const float values[4] )
{
	CCP_ASSERT( m_isPrimary );
	// Proper thread group size is a multiple of threadExecutionWidth and less than maxTotalThreadsPerThreadgroup
	// (both are preperties of MTLComputePipelineState).
	MTLSize threadGroupSize = MTLSizeMake( 32, 1, 1 );

	NSUInteger width = buffer.length / sizeof( float );
	NSUInteger groupDimX = ( width + threadGroupSize.width - 1 ) / threadGroupSize.width;
	size_t length = buffer.length / sizeof( float );

	id<MTLFunction> oldComputeFunction = m_computeFunction;
	m_computeFunction = m_clearBufferComputeFunctions[0];

	id<MTLComputeCommandEncoder> computeEncoder = GetComputeEncoder();

	// Create and set a new pipelinestate if required.
	if( EmitComputePipelineState() )
	{
		[computeEncoder setBytes:values length:4 * sizeof( *values ) atIndex:0];
		[computeEncoder setBytes:&length length:sizeof( length ) atIndex:1];
		[computeEncoder setBuffer:buffer offset:0 atIndex:2];

		[computeEncoder dispatchThreadgroups:MTLSizeMake( groupDimX, 1, 1 ) threadsPerThreadgroup:threadGroupSize];
	}
	ReleaseEncoder( false );

	m_computeFunction = oldComputeFunction;

	// TODO: Insert a resource barrier here, so the following render stage knows when clear is complete.
}

void MetalWorkQueue::ClearBuffer( id<MTLBuffer> buffer, const uint32_t values[4] )
{
	CCP_ASSERT( m_isPrimary );

	// Fast path.
	if( values[0] == 0 && values[1] == 0 && values[2] == 0 && values[3] == 0 )
	{
		id<MTLBlitCommandEncoder> blitEncoder = GetBlitEncoder();
		[blitEncoder fillBuffer:buffer range:NSMakeRange( 0, buffer.length ) value:0];

		ReleaseEncoder( false );

		return;
	}

	// Proper thread group size is a multiple of threadExecutionWidth and less than maxTotalThreadsPerThreadgroup
	// (both are preperties of MTLComputePipelineState).
	MTLSize threadGroupSize = MTLSizeMake( 32, 1, 1 );

	NSUInteger width = buffer.length / sizeof( uint32_t );
	NSUInteger groupDimX = ( width + threadGroupSize.width - 1 ) / threadGroupSize.width;
	size_t length = buffer.length / sizeof( uint32_t );

	id<MTLFunction> oldComputeFunction = m_computeFunction;
	m_computeFunction = m_clearBufferComputeFunctions[1];

	id<MTLComputeCommandEncoder> computeEncoder = GetComputeEncoder();

	// Create and set a new pipelinestate if required.
	if( EmitComputePipelineState() )
	{
		[computeEncoder setBytes:values length:4 * sizeof( *values ) atIndex:0];
		[computeEncoder setBytes:&length length:sizeof( length ) atIndex:1];
		[computeEncoder setBuffer:buffer offset:0 atIndex:2];

		[computeEncoder dispatchThreadgroups:MTLSizeMake( groupDimX, 1, 1 ) threadsPerThreadgroup:threadGroupSize];
	}
	ReleaseEncoder( false );

	m_computeFunction = oldComputeFunction;

	// TODO: Insert a resource barrier here, so the following render stage knows when clear is complete.
}

void MetalWorkQueue::ClearTexture( id<MTLTexture> texture, uint32_t mipLevel, const float values[4] )
{
	CCP_ASSERT( m_isPrimary );
	// Only 2D textures are supported right now.
	CCP_ASSERT( texture.textureType == MTLTextureType2D );
	// Only writing to mip level 0 supported on macOS.
	CCP_ASSERT( mipLevel == 0 );
	// CCP_ASSERT( texture.mipmapLevelCount > mipLevel );

	// Proper thread group size is a multiple of threadExecutionWidth and less than maxTotalThreadsPerThreadgroup
	// (both are preperties of MTLComputePipelineState).
	MTLSize threadGroupSize = MTLSizeMake( 32, 32, 1 );

	NSUInteger width = std::max<NSUInteger>( texture.width >> mipLevel, 1 );
	NSUInteger height = std::max<NSUInteger>( texture.height >> mipLevel, 1 );
	NSUInteger groupDimX = ( width + threadGroupSize.width - 1 ) / threadGroupSize.width;
	NSUInteger groupDimY = ( height + threadGroupSize.height - 1 ) / threadGroupSize.height;

	id<MTLFunction> oldComputeFunction = m_computeFunction;
	m_computeFunction = m_clearTextureComputeFunctions[0];

	id<MTLComputeCommandEncoder> computeEncoder = GetComputeEncoder();

	// Create and set a new pipelinestate if required.
	if( EmitComputePipelineState() )
	{
		[computeEncoder setBytes:values length:4 * sizeof( *values ) atIndex:0];
		[computeEncoder setTexture:texture atIndex:0];

		[computeEncoder dispatchThreadgroups:MTLSizeMake( groupDimX, groupDimY, 1 )
					   threadsPerThreadgroup:threadGroupSize];
	}
	ReleaseEncoder( false );

	m_computeFunction = oldComputeFunction;

	// TODO: Insert a resource barrier here, so the following render stage knows when clear is complete.
}

void MetalWorkQueue::ClearTexture( id<MTLTexture> texture, uint32_t mipLevel, const uint32_t values[4] )
{
	CCP_ASSERT( m_isPrimary );
	// Only 2D textures are supported right now.
	CCP_ASSERT( texture.textureType == MTLTextureType2D );
	// Only writing to mip level 0 supported on macOS.
	CCP_ASSERT( mipLevel == 0 );
	// CCP_ASSERT( texture.mipmapLevelCount > mipLevel );

	// Proper thread group size is a multiple of threadExecutionWidth and less than maxTotalThreadsPerThreadgroup
	// (both are preperties of MTLComputePipelineState).
	MTLSize threadGroupSize = MTLSizeMake( 32, 32, 1 );

	NSUInteger width = std::max<NSUInteger>( texture.width >> mipLevel, 1 );
	NSUInteger height = std::max<NSUInteger>( texture.height >> mipLevel, 1 );
	NSUInteger groupDimX = ( width + threadGroupSize.width - 1 ) / threadGroupSize.width;
	NSUInteger groupDimY = ( height + threadGroupSize.height - 1 ) / threadGroupSize.height;

	id<MTLFunction> oldComputeFunction = m_computeFunction;
	m_computeFunction = m_clearTextureComputeFunctions[1];

	id<MTLComputeCommandEncoder> computeEncoder = GetComputeEncoder();

	// Create and set a new pipelinestate if required.
	if( EmitComputePipelineState() )
	{
		[computeEncoder setBytes:values length:4 * sizeof( *values ) atIndex:0];
		[computeEncoder setTexture:texture atIndex:0];

		[computeEncoder dispatchThreadgroups:MTLSizeMake( groupDimX, groupDimY, 1 )
					   threadsPerThreadgroup:threadGroupSize];
	}
	ReleaseEncoder( false );

	m_computeFunction = oldComputeFunction;

	// TODO: Insert a resource barrier here, so the following render stage knows when clear is complete.
}

void MetalWorkQueue::CopyTextureToMTLBuffer( id<MTLTexture> texture,
											 id<MTLBuffer> destBuffer,
											 uint32_t bytesPerRow,
											 uint32_t bytesPerSlice,
											 MTLOrigin origin,
											 MTLSize size,
											 uint32_t mipLevel,
											 bool waitForData )
{
	CCP_ASSERT( m_isPrimary );
	FlushOutstandingOperations();

	id<MTLBlitCommandEncoder> blitEncoder = GetBlitEncoder();

	[blitEncoder copyFromTexture:texture
					 sourceSlice:0
					 sourceLevel:mipLevel
					sourceOrigin:origin
					  sourceSize:size
						toBuffer:destBuffer
			   destinationOffset:0
		  destinationBytesPerRow:bytesPerRow
		destinationBytesPerImage:bytesPerSlice];

	ReleaseEncoder( true );
	blitEncoder = GetBlitEncoder();

	[blitEncoder synchronizeResource:destBuffer]; // sync to CPU

	if( waitForData )
	{
		ReleaseEncoder( true );
		[m_commandBuffer addCompletedHandler:^( id<MTLCommandBuffer> commandBuffer ) {
			if( @available( macOS 10.15, * ) )
			{
				auto duration = commandBuffer.GPUEndTime - commandBuffer.GPUStartTime;
				s_gpuFrameTime += duration;
			}
		}];
		CommitCommandBuffer( MTLCBCOMMIT_WAITUNTILCOMPLETE );
	}
	else
	{
		ReleaseEncoder( false );
	}
}

void MetalWorkQueue::CopyTextureToTexture( id<MTLTexture> srcTexture,
										   uint32_t srcSlice,
										   uint32_t srcMip,
										   MTLOrigin srcOrigin,
										   MTLSize srcSize,
										   id<MTLTexture> dstTexture,
										   uint32_t dstSlice,
										   uint32_t dstMip,
										   MTLOrigin dstOrigin )
{
	CCP_ASSERT( m_isPrimary );
	id<MTLBlitCommandEncoder> blitEncoder = GetBlitEncoder();

	[blitEncoder copyFromTexture:srcTexture
					 sourceSlice:srcSlice
					 sourceLevel:srcMip
					sourceOrigin:srcOrigin
					  sourceSize:srcSize
					   toTexture:dstTexture
				destinationSlice:dstSlice
				destinationLevel:dstMip
			   destinationOrigin:dstOrigin];

	ReleaseEncoder( false );
}

void MetalWorkQueue::ResolveMsaa( id<MTLTexture> source, id<MTLTexture> destination )
{
	CCP_ASSERT( m_isPrimary );
	auto bkHasPendingRenderPassHint = m_hasPendingRenderPassHint;
	m_hasPendingRenderPassHint = false;

	// Changing an attachment requires a new render encoder so we must flush any outstanding work.
	ReleaseEncoder( true );

	if( source.pixelFormat == MTLPixelFormatDepth16Unorm || source.pixelFormat == MTLPixelFormatDepth32Float ||
		source.pixelFormat == MTLPixelFormatDepth24Unorm_Stencil8 ||
		source.pixelFormat == MTLPixelFormatDepth32Float_Stencil8 )
	{
		id<MTLTexture> oldAttachment = m_currentRenderPassDescriptor.depthAttachment.texture;

		SetDepthStencilAttachment( source );
		m_currentRenderPassDescriptor.depthAttachment.resolveTexture = destination;
		m_currentRenderPassDescriptor.depthAttachment.storeAction = MTLStoreAction::MTLStoreActionMultisampleResolve;

		GetRenderEncoder( @"ResolveDepthEncoder" );
		ReleaseEncoder( true );

		m_currentRenderPassDescriptor.depthAttachment.resolveTexture = nil;
		SetDepthStencilAttachment( oldAttachment );
	}
	else
	{
		uint32_t numOldColorAttachments = m_numRenderAttachments;
		id<MTLTexture> oldColorAttachments[METAL_MAX_RENDER_TARGETS];
		std::fill_n( oldColorAttachments, METAL_MAX_RENDER_TARGETS, nil );
		uint32_t oldAttachmentSlice[METAL_MAX_RENDER_TARGETS];
		std::fill_n( oldAttachmentSlice, METAL_MAX_RENDER_TARGETS, 0 );

		for( uint32_t i = 0; i < numOldColorAttachments; ++i )
		{
			oldColorAttachments[i] = m_currentRenderPassDescriptor.colorAttachments[i].texture;
			oldAttachmentSlice[i] = uint32_t( m_currentRenderPassDescriptor.colorAttachments[i].depthPlane );
			SetRenderAttachments( nil, i );
		}

		SetRenderAttachments( source, 0 );
		m_currentRenderPassDescriptor.colorAttachments[0].resolveTexture = destination;
		m_currentRenderPassDescriptor.colorAttachments[0].storeAction =
			MTLStoreAction::MTLStoreActionMultisampleResolve;

		GetRenderEncoder( @"ResolveColorEncoder" );
		ReleaseEncoder( true );

		m_currentRenderPassDescriptor.colorAttachments[0].resolveTexture = nil;
		SetRenderAttachments( nil, 0 );
		for( uint32_t i = 0; i < numOldColorAttachments; ++i )
		{
			SetRenderAttachments( oldColorAttachments[i], i, oldAttachmentSlice[i] );
		}
	}
	m_hasPendingRenderPassHint = bkHasPendingRenderPassHint;
}

// FROM BOOST
template <class T> inline void hash_combine( std::size_t& seed, const T& v )
{
	std::hash<T> hasher;
	seed ^= hasher( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
}

id<MTLDepthStencilState> MetalWorkQueue::GetCachedDepthStencilState( MTLDepthStencilDescriptor* descriptor )
{
	if( !descriptor )
	{
		return nil;
	}

	id<MTLDepthStencilState> result = nil;
	int index = METAL_DEPTHSTENCIL_CACHE_SIZE - 1; // index of temporary state

	// NOTE: For simplicity we will cache only depth-stencil states without stencil. All other depth-stencil
	// states will go into a temporary cache entry (the last one).
	if( m_depthStencilDescriptor.frontFaceStencil.stencilCompareFunction == MTLCompareFunctionAlways &&
		m_depthStencilDescriptor.backFaceStencil.stencilCompareFunction == MTLCompareFunctionAlways )
	{
		index = int( descriptor.depthCompareFunction * 2 + ( descriptor.depthWriteEnabled ? 1 : 0 ) );
		CCP_ASSERT( index < METAL_DEPTHSTENCIL_CACHE_SIZE );

		result = m_depthStencilStateCache[index];

		// Cache entry is not yet populated?
		if( !result )
		{
			result = [m_device newDepthStencilStateWithDescriptor:descriptor];
			m_depthStencilStateCache[index] = result;
		}
	}
	else
	{
#if !__has_feature( objc_arc )
		[m_depthStencilStateCache[index] release];
#endif

		result = [m_device newDepthStencilStateWithDescriptor:descriptor];
		m_depthStencilStateCache[index] = result;
	}

	return result;
}

size_t MetalWorkQueue::CalculateVertexDescriptorHash()
{
	size_t hashVal = 0;
	if( m_currentVertexDescriptor )
	{
		hash_combine( hashVal, m_currentVertexDescriptorBaseHash );
		unsigned int mask = m_currentVertexStreamMask;
		for( int i = 0; mask && i < METAL_VERTEX_STREAM_BUFFER_COUNT; ++i )
		{
			unsigned int flag = ( 1 << i );
			if( mask & flag )
			{
				hash_combine( hashVal, (uint32_t)m_currentVertexDescriptor.layouts[i].stride );

				mask = mask & ~flag;
			}
		}
	}

	return hashVal;
}

size_t MetalWorkQueue::CalculateAttachmentsHash()
{
	size_t hashVal = 0;
	size_t sampleCount = 0;

	for( uint32_t i = 0; i < m_numRenderAttachments; ++i )
	{
		hash_combine( hashVal, (uint32_t)m_blendState[i].blendType );
		hash_combine( hashVal, (uint32_t)m_blendState[i].alphaCoverageEnable );
		hash_combine( hashVal, (uint32_t)m_blendState[i].rgbBlendOp );
		hash_combine( hashVal, (uint32_t)m_blendState[i].alphaBlendOp );
		hash_combine( hashVal, (uint32_t)m_blendState[i].srcColorFactor );
		hash_combine( hashVal, (uint32_t)m_blendState[i].srcAlphaFactor );
		hash_combine( hashVal, (uint32_t)m_blendState[i].destColorFactor );
		hash_combine( hashVal, (uint32_t)m_blendState[i].destAlphaFactor );
		hash_combine( hashVal, (uint32_t)m_blendState[i].writeMask );

		if( m_currentRenderPassDescriptor.colorAttachments[i].texture )
		{
			hash_combine( hashVal, (uint32_t)m_currentRenderPassDescriptor.colorAttachments[i].texture.pixelFormat );
			sampleCount = m_currentRenderPassDescriptor.colorAttachments[i].texture.sampleCount;
		}
	}

	if( m_currentRenderPassDescriptor.depthAttachment.texture )
	{
		hash_combine( hashVal, (uint32_t)m_currentRenderPassDescriptor.depthAttachment.texture.pixelFormat );
		sampleCount = m_currentRenderPassDescriptor.depthAttachment.texture.sampleCount;
	}
	if( m_currentRenderPassDescriptor.stencilAttachment.texture )
	{
		hash_combine( hashVal, (uint32_t)m_currentRenderPassDescriptor.stencilAttachment.texture.pixelFormat );
	}

	hash_combine( hashVal, (uint32_t)sampleCount );

	return hashVal;
}

void MetalWorkQueue::SetRenderTargetSizedScissorRect()
{
	NSUInteger width = 0;
	NSUInteger height = 0;
	if( m_currentRenderPassDescriptor.depthAttachment.texture )
	{
		width = m_currentRenderPassDescriptor.depthAttachment.texture.width;
		height = m_currentRenderPassDescriptor.depthAttachment.texture.height;
	}
	else
	{
		// NOTE: This will fail if a render pass has color attachment 0 unset.
		if( m_currentRenderPassDescriptor.colorAttachments[0].texture )
		{
			width = m_currentRenderPassDescriptor.colorAttachments[0].texture.width;
			height = m_currentRenderPassDescriptor.colorAttachments[0].texture.height;
		}
	}
	m_scissorRect = { 0, 0, width, height };
	m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_VIEWPORT;
}

bool MetalWorkQueue::EmitRenderPipelineState()
{
	if( m_currentEncoderType != MTLENCODERTYPE_RENDER || !m_encoderInUse || !m_currentRenderEncoder )
	{
		CCP_AL_LOGERR( "Not valid to call EmitRenderPipelineState() without an active render encoder." );
		return false;
	}

	id<MTLRenderPipelineState> pipelineState = nil;

#if METAL_ENABLE_RENDER_STATE_CACHING
	size_t hashVal = 0;

	hash_combine( hashVal, (__bridge void*)m_device );
	hash_combine( hashVal, (__bridge void*)m_vertexFunction );
	hash_combine( hashVal, (__bridge void*)m_fragmentFunction );
	hash_combine( hashVal, m_currentVertexDescriptorHash );
	hash_combine( hashVal, m_currentAttachmentsHash );

	pipelineState = m_context->GetCachedRenderPipelineState( hashVal );
	m_currentRenderPipelineDescriptorHash = hashVal;
#endif

	if( !pipelineState )
	{
		MTLRenderPipelineDescriptor* renderPipelineStateDescriptor = [MTLRenderPipelineDescriptor new];

		renderPipelineStateDescriptor.label = @"Standard render pipeline";

		renderPipelineStateDescriptor.vertexDescriptor = m_currentVertexDescriptor;
		renderPipelineStateDescriptor.inputPrimitiveTopology = MTLPrimitiveTopologyClassUnspecified;

		renderPipelineStateDescriptor.vertexFunction = m_vertexFunction;
		renderPipelineStateDescriptor.fragmentFunction = m_fragmentFunction;
		renderPipelineStateDescriptor.rasterizationEnabled = ( m_fragmentFunction != nil );

#if METAL_TESSELLATION_SUPPORT
		renderPipelineStateDescriptor.maxTessellationFactor = 1;
		renderPipelineStateDescriptor.tessellationFactorScaleEnabled = NO;
		renderPipelineStateDescriptor.tessellationFactorFormat = MTLTessellationFactorFormatHalf;
		renderPipelineStateDescriptor.tessellationControlPointIndexType = MTLTessellationControlPointIndexTypeNone;
		renderPipelineStateDescriptor.tessellationFactorStepFunction = MTLTessellationFactorStepFunctionConstant;
		renderPipelineStateDescriptor.tessellationOutputWindingOrder = MTLWindingCounterClockwise;
		renderPipelineStateDescriptor.tessellationPartitionMode = MTLTessellationPartitionModePow2;
#endif

		// This is set for all attachments so pick it off the first one.
		renderPipelineStateDescriptor.alphaToCoverageEnabled = m_blendState[0].alphaCoverageEnable ? YES : NO;

		size_t sampleCount = 0;
		for( uint32_t i = 0; i < m_numRenderAttachments; ++i )
		{
			if( m_blendState[0].blendType == METAL_BLENDING_ENABLED )
			{
				renderPipelineStateDescriptor.colorAttachments[i].blendingEnabled = YES;
				renderPipelineStateDescriptor.colorAttachments[i].writeMask = m_blendState[0].writeMask;
				renderPipelineStateDescriptor.colorAttachments[i].rgbBlendOperation = m_blendState[0].rgbBlendOp;
				renderPipelineStateDescriptor.colorAttachments[i].sourceRGBBlendFactor = m_blendState[0].srcColorFactor;
				renderPipelineStateDescriptor.colorAttachments[i].destinationRGBBlendFactor =
					m_blendState[0].destColorFactor;
				renderPipelineStateDescriptor.colorAttachments[i].alphaBlendOperation = m_blendState[0].rgbBlendOp;
				renderPipelineStateDescriptor.colorAttachments[i].sourceAlphaBlendFactor =
					m_blendState[0].srcColorFactor;
				renderPipelineStateDescriptor.colorAttachments[i].destinationAlphaBlendFactor =
					m_blendState[0].destColorFactor;
			}
			else if( m_blendState[0].blendType == METAL_BLENDING_ENABLED_SEPARATE_ALPHA )
			{
				renderPipelineStateDescriptor.colorAttachments[i].blendingEnabled = YES;
				renderPipelineStateDescriptor.colorAttachments[i].writeMask = m_blendState[0].writeMask;
				renderPipelineStateDescriptor.colorAttachments[i].rgbBlendOperation = m_blendState[0].rgbBlendOp;
				renderPipelineStateDescriptor.colorAttachments[i].sourceRGBBlendFactor = m_blendState[0].srcColorFactor;
				renderPipelineStateDescriptor.colorAttachments[i].destinationRGBBlendFactor =
					m_blendState[0].destColorFactor;
				renderPipelineStateDescriptor.colorAttachments[i].alphaBlendOperation = m_blendState[0].alphaBlendOp;
				renderPipelineStateDescriptor.colorAttachments[i].sourceAlphaBlendFactor =
					m_blendState[0].srcAlphaFactor;
				renderPipelineStateDescriptor.colorAttachments[i].destinationAlphaBlendFactor =
					m_blendState[0].destAlphaFactor;
			}
			else
			{
				renderPipelineStateDescriptor.colorAttachments[i].blendingEnabled = NO;
				renderPipelineStateDescriptor.colorAttachments[i].writeMask = m_blendState[0].writeMask;
			}

			if( m_currentRenderPassDescriptor.colorAttachments[i].texture )
			{
				renderPipelineStateDescriptor.colorAttachments[i].pixelFormat =
					m_currentRenderPassDescriptor.colorAttachments[i].texture.pixelFormat;
				// All sample counts should match so doesn't matter if we pick this up multiple times
				sampleCount = m_currentRenderPassDescriptor.colorAttachments[i].texture.sampleCount;
			}
			else
			{
				renderPipelineStateDescriptor.colorAttachments[i].pixelFormat = MTLPixelFormatInvalid;
			}
		}

		if( m_currentRenderPassDescriptor.depthAttachment.texture )
		{
			renderPipelineStateDescriptor.depthAttachmentPixelFormat =
				m_currentRenderPassDescriptor.depthAttachment.texture.pixelFormat;
			sampleCount = m_currentRenderPassDescriptor.depthAttachment.texture.sampleCount;
		}
		else
		{
			renderPipelineStateDescriptor.depthAttachmentPixelFormat = MTLPixelFormatInvalid;
		}

		if( m_currentRenderPassDescriptor.stencilAttachment.texture )
		{
			renderPipelineStateDescriptor.stencilAttachmentPixelFormat =
				m_currentRenderPassDescriptor.stencilAttachment.texture.pixelFormat;
		}
		else
		{
			renderPipelineStateDescriptor.stencilAttachmentPixelFormat = MTLPixelFormatInvalid;
		}

		// Sample count can be derived from multiple sources - they should all be the same however (validation layer will complain if not).
		renderPipelineStateDescriptor.rasterSampleCount = sampleCount;
		renderPipelineStateDescriptor.sampleCount = sampleCount;

		NSError* error = nil;

		pipelineState = [m_device newRenderPipelineStateWithDescriptor:renderPipelineStateDescriptor error:&error];

#if !__has_feature( objc_arc )
		[renderPipelineStateDescriptor release];
#endif

		if( !pipelineState )
		{
			CCP_AL_LOGERR( "Failed to created pipeline state. Error: %s", error.localizedDescription.UTF8String );
#if !__has_feature( objc_arc )
			[error release];
#endif
			return false;
		}

#if METAL_ENABLE_RENDER_STATE_CACHING
		m_context->AddRenderPipelineStateToCache( hashVal, pipelineState );
#endif
	}

	if( pipelineState != m_currentRenderPipelineState )
	{
		[m_currentRenderEncoder setRenderPipelineState:pipelineState];
		m_currentRenderPipelineState = pipelineState;
	}

	return true;
}

bool MetalWorkQueue::EmitRenderEncoderState()
{
	if( m_currentEncoderType != MTLENCODERTYPE_RENDER || !m_encoderInUse || !m_currentRenderEncoder )
	{
		if( m_currentEncoderType != MTLENCODERTYPE_RENDER )
		{
			CCP_AL_LOGERR( "Current command encoder is not render type - failed to change render state." );
		}

		if( !m_encoderInUse )
		{
			CCP_AL_LOGERR( "Current render encoder is not in use - failed to change render state." );
		}

		if( !m_currentRenderEncoder )
		{
			CCP_AL_LOGERR( "Current render encoder is nil - failed to change render state." );
		}

		return false;
	}

	// Flush pending render states.

	if( m_dirtyRenderEncoderState & METAL_RENDERENCODERDIRTYSTATE_CULLMODE )
	{
		[m_currentRenderEncoder setCullMode:m_cullMode];
		m_dirtyRenderEncoderState &= ~METAL_RENDERENCODERDIRTYSTATE_CULLMODE;
	}

	if( m_dirtyRenderEncoderState & METAL_RENDERENCODERDIRTYSTATE_FILLMODE )
	{
		[m_currentRenderEncoder setTriangleFillMode:m_fillMode];
		m_dirtyRenderEncoderState &= ~METAL_RENDERENCODERDIRTYSTATE_FILLMODE;
	}

	if( m_dirtyRenderEncoderState & METAL_RENDERENCODERDIRTYSTATE_VIEWPORT )
	{
		if( m_validViewport )
		{
			[m_currentRenderEncoder setViewport:m_viewport];
			[m_currentRenderEncoder setScissorRect:m_scissorRect];
		}

		m_dirtyRenderEncoderState &= ~METAL_RENDERENCODERDIRTYSTATE_VIEWPORT;
	}

	if( m_dirtyRenderEncoderState & METAL_RENDERENCODERDIRTYSTATE_BLENDCOLOR )
	{
		[m_currentRenderEncoder setBlendColorRed:m_blendState[0].blendColor.red
										   green:m_blendState[0].blendColor.green
											blue:m_blendState[0].blendColor.blue
										   alpha:m_blendState[0].blendColor.alpha];
		m_dirtyRenderEncoderState &= ~METAL_RENDERENCODERDIRTYSTATE_BLENDCOLOR;
	}

	if( m_dirtyRenderEncoderState & METAL_RENDERENCODERDIRTYSTATE_DEPTHBIAS )
	{
		[m_currentRenderEncoder setDepthBias:m_depthBias.depthBias
								  slopeScale:m_depthBias.slopeScale
									   clamp:m_depthBias.clamp];
		m_dirtyRenderEncoderState &= ~METAL_RENDERENCODERDIRTYSTATE_DEPTHBIAS;
	}

	if( m_dirtyRenderEncoderState & METAL_RENDERENCODERDIRTYSTATE_DEPTHCLIP )
	{
		[m_currentRenderEncoder setDepthClipMode:m_depthClipMode];
		m_dirtyRenderEncoderState &= ~METAL_RENDERENCODERDIRTYSTATE_DEPTHCLIP;
	}

	if( m_dirtyRenderEncoderState & METAL_RENDERENCODERDIRTYSTATE_DEPTHSTENCILSTATE )
	{
		// Make sure we don't enable depth write when the depth attachment is not set.
		BOOL depthWriteEnabled = m_depthStencilDescriptor.depthWriteEnabled;
		MTLCompareFunction depthCompareFunction = m_depthStencilDescriptor.depthCompareFunction;

		if( !m_currentRenderPassDescriptor.depthAttachment.texture )
		{
			m_depthStencilDescriptor.depthWriteEnabled = NO;
			m_depthStencilDescriptor.depthCompareFunction = MTLCompareFunctionAlways;
		}

		id<MTLDepthStencilState> depthStencilState = GetCachedDepthStencilState( m_depthStencilDescriptor );
		[m_currentRenderEncoder setDepthStencilState:depthStencilState];

		// Restore state set before this fuction call.
		m_depthStencilDescriptor.depthWriteEnabled = depthWriteEnabled;
		m_depthStencilDescriptor.depthCompareFunction = depthCompareFunction;

		m_dirtyRenderEncoderState &= ~METAL_RENDERENCODERDIRTYSTATE_DEPTHSTENCILSTATE;
	}

	if( m_dirtyRenderEncoderState & METAL_RENDERENCODERDIRTYSTATE_VERTEXDESCRIPTOR )
	{
		if( m_currentVertexDescriptor )
		{
			for( int i = 0; i < METAL_VERTEX_STREAM_BUFFER_COUNT; ++i )
			{
				if( m_currentVertexStreamMask & ( 1 << i ) )
				{
					m_currentVertexDescriptor.layouts[i].stride = m_boundVertexStreams[i].stride;
				}
				else
				{
					m_currentVertexDescriptor.layouts[i].stride = 0;
				}
			}
		}

#if METAL_ENABLE_RENDER_STATE_CACHING
		m_currentVertexDescriptorHash = CalculateVertexDescriptorHash();
#endif

		m_dirtyRenderEncoderState &= ~METAL_RENDERENCODERDIRTYSTATE_VERTEXDESCRIPTOR;
	}

	if( m_dirtyRenderEncoderState & METAL_RENDERENCODERDIRTYSTATE_ATTACHMENTS )
	{
#if METAL_ENABLE_RENDER_STATE_CACHING
		m_currentAttachmentsHash = CalculateAttachmentsHash();
#endif

		m_dirtyRenderEncoderState &= ~METAL_RENDERENCODERDIRTYSTATE_ATTACHMENTS;
	}

	// Make sure we updated all dirty render states.
	CCP_ASSERT( m_dirtyRenderEncoderState == METAL_RENDERENCODERDIRTYSTATE_NONE );

	// Bind the required vertex and fragment buffers.
	SetVertexBufferBindings();
	SetFragmentBufferBindings();

	// Create and set a new pipelinestate if required.
	bool result = EmitRenderPipelineState();

	return result;
}

bool MetalWorkQueue::EmitComputePipelineState()
{
	CCP_ASSERT( m_isPrimary );
	if( m_currentEncoderType != MTLENCODERTYPE_COMPUTE || !m_encoderInUse || !m_currentComputeEncoder )
	{
		CCP_AL_LOGERR( "Not valid to call EmitComputePipelineState() without an active compute encoder." );
		return false;
	}

	id<MTLComputePipelineState> pipelineState = nil;

#if METAL_ENABLE_COMPUTE_STATE_CACHING
	size_t hashVal = 0;

	hash_combine( hashVal, (__bridge void*)m_device );
	hash_combine( hashVal, (__bridge void*)m_computeFunction );

	// If this matches the current pipeline state then we should already have the correct pipeline bound.
	if( hashVal == m_currentComputePipelineDescriptorHash && m_currentComputePipelineState != nil )
	{
		return true;
	}

	pipelineState = m_context->GetCachedComputePipelineState( hashVal );
	m_currentComputePipelineDescriptorHash = hashVal;
#endif

	if( !pipelineState )
	{
		NSError* error = NULL;
		pipelineState = [m_device newComputePipelineStateWithFunction:m_computeFunction error:&error];

		if( !pipelineState )
		{
			CCP_AL_LOGERR( "Failed to create compute pipeline state. Error: %s",
						   error.localizedDescription.UTF8String );
#if !__has_feature( objc_arc )
			[error release];
#endif
			return false;
		}

#if METAL_ENABLE_COMPUTE_STATE_CACHING
		m_context->AddComputePipelineStateToCache( hashVal, pipelineState );
#endif
	}

	if( pipelineState != m_currentComputePipelineState )
	{
		[m_currentComputeEncoder setComputePipelineState:pipelineState];
		m_currentComputePipelineState = pipelineState;
	}

	return true;
}

bool MetalWorkQueue::EmitComputeEncoderState()
{
	CCP_ASSERT( m_isPrimary );
	if( m_currentEncoderType != MTLENCODERTYPE_COMPUTE || !m_encoderInUse || !m_currentComputeEncoder )
	{
		if( m_currentEncoderType != MTLENCODERTYPE_COMPUTE )
		{
			CCP_AL_LOGERR( "Current command encoder is not compute type - failed to change compute state." );
		}

		if( !m_encoderInUse )
		{
			CCP_AL_LOGERR( "Current compute encoder is not in use - failed to change compute state." );
		}

		if( !m_currentComputeEncoder )
		{
			CCP_AL_LOGERR( "Current compute encoder is nil - failed to change compute state." );
		}

		return false;
	}

	// Bind the required buffers and textures.
	SetComputeBufferBindings();

	// Create and set a new pipelinestate if required.
	if( !EmitComputePipelineState() )
	{
		return false;
	}

	return true;
}

void MetalWorkQueue::ResetClearState()
{
	CCP_ASSERT( m_isPrimary );
	for( uint32_t i = 0; i < m_numRenderAttachments; ++i )
	{
		if( m_currentRenderPassDescriptor.colorAttachments[i].loadAction == MTLLoadActionClear )
		{
			m_currentRenderPassDescriptor.colorAttachments[i].loadAction = MTLLoadActionLoad;
		}
	}
	if( m_currentRenderPassDescriptor.depthAttachment.loadAction == MTLLoadActionClear )
	{
		m_currentRenderPassDescriptor.depthAttachment.loadAction = MTLLoadActionLoad;
	}
	if( m_currentRenderPassDescriptor.stencilAttachment.loadAction == MTLLoadActionClear )
	{
		m_currentRenderPassDescriptor.stencilAttachment.loadAction = MTLLoadActionLoad;
	}
	m_pendingClear = false;
}

void MetalWorkQueue::SetRenderAttachments( id<MTLTexture> texture, uint32_t index, uint32_t slice )
{
	CCP_ASSERT( m_isPrimary );
	CCP_ASSERT( index < METAL_MAX_RENDER_TARGETS );

	// Check if there's soemthing to actually do.
	if( texture == m_currentRenderPassDescriptor.colorAttachments[index].texture )
	{
		return;
	}

	// Changing an attachment requires a new render encoder so we must flush any outstanding work.
	FlushOutstandingOperations();

	m_currentRenderPassDescriptor.colorAttachments[index].texture = texture;
	m_currentRenderPassDescriptor.colorAttachments[index].slice = slice;

	if( texture )
	{
		m_currentRenderPassDescriptor.colorAttachments[index].loadAction = MTLLoadActionLoad;
		m_currentRenderPassDescriptor.colorAttachments[index].storeAction = MTLStoreActionStore;
		METAL_LOG(
			@"Log:SetRenderAttachments [%u] %lux%lu (%lu)", index, texture.width, texture.height, texture.sampleCount );

		m_numRenderAttachments = MAX( m_numRenderAttachments, index + 1 );
	}
	else
	{
		m_currentRenderPassDescriptor.colorAttachments[index].loadAction = MTLLoadActionDontCare;
		m_currentRenderPassDescriptor.colorAttachments[index].storeAction = MTLStoreActionDontCare;
		METAL_LOG( @"Log:SetRenderAttachments [%u] to nil", index );

		uint32_t i = m_numRenderAttachments;

		// Search back looking for next valid render attachment.
		while( i > 0 )
		{
			if( m_currentRenderPassDescriptor.colorAttachments[i - 1].texture )
			{
				m_numRenderAttachments = i;
				break;
			}
			--i;
		}
	}

	SetRenderTargetSizedScissorRect();

	m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_ATTACHMENTS;
}

void MetalWorkQueue::SetDepthStencilAttachment( id<MTLTexture> texture )
{
	CCP_ASSERT( m_isPrimary );
	id<MTLTexture> depthTexture = nil;
	id<MTLTexture> stencilTexture = nil;

	if( texture )
	{
		switch( texture.pixelFormat )
		{
		case MTLPixelFormatDepth16Unorm:
		case MTLPixelFormatDepth32Float:
			depthTexture = texture;
			break;

		case MTLPixelFormatStencil8:
		case MTLPixelFormatX32_Stencil8:
		case MTLPixelFormatX24_Stencil8:
			stencilTexture = texture;
			break;

		case MTLPixelFormatDepth24Unorm_Stencil8:
		case MTLPixelFormatDepth32Float_Stencil8:
			depthTexture = texture;
			stencilTexture = texture;
			break;

		default:
			break;
		}
	}

	// Don't do anything if nothing has changed.
	if( m_currentRenderPassDescriptor.depthAttachment.texture == depthTexture &&
		m_currentRenderPassDescriptor.stencilAttachment.texture == stencilTexture )
	{
		return;
	}

	// Changing an attachment effectively means a new render pass so we need to terminate the current one (if we have one).
	FlushOutstandingOperations();

	// Change depth attachment.
	if( m_currentRenderPassDescriptor.depthAttachment.texture != depthTexture )
	{
		m_currentRenderPassDescriptor.depthAttachment.texture = depthTexture;

		if( depthTexture )
		{
			m_currentRenderPassDescriptor.depthAttachment.loadAction = MTLLoadActionLoad;
			m_currentRenderPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;

			METAL_LOG( @"Log:SetDepthStencilAttachment - depth %lux%lu (%lu)",
					   texture.width,
					   texture.height,
					   texture.sampleCount );
		}
		else
		{
			m_currentRenderPassDescriptor.depthAttachment.loadAction = MTLLoadActionDontCare;
			m_currentRenderPassDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;

			METAL_LOG( @"Log:SetDepthStencilAttachment - depth to nil" );
		}

		// Make sure depthWriteEnabled state will be recalculated.
		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_DEPTHSTENCILSTATE;

		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_ATTACHMENTS;
	}

	// Change stencil attachment.
	if( m_currentRenderPassDescriptor.stencilAttachment.texture != stencilTexture )
	{
		m_currentRenderPassDescriptor.stencilAttachment.texture = stencilTexture;

		if( stencilTexture )
		{
			m_currentRenderPassDescriptor.stencilAttachment.loadAction = MTLLoadActionLoad;
			m_currentRenderPassDescriptor.stencilAttachment.storeAction = MTLStoreActionStore;

			METAL_LOG( @"Log:SetDepthStencilAttachment - stencil %lux%lu (%lu)",
					   texture.width,
					   texture.height,
					   texture.sampleCount );
		}
		else
		{
			m_currentRenderPassDescriptor.stencilAttachment.loadAction = MTLLoadActionDontCare;
			m_currentRenderPassDescriptor.stencilAttachment.storeAction = MTLStoreActionDontCare;

			SetStencilState( false );

			METAL_LOG( @"Log:SetDepthStencilAttachment - stencil to nil" );
		}

		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_ATTACHMENTS;
	}

	SetRenderTargetSizedScissorRect();
}

void MetalWorkQueue::SetShaders( id<MTLFunction> vertexFunction,
								 id<MTLFunction> fragmentFunction,
								 id<MTLFunction> computeFunction,
								 MTLSize threadGroupSize,
								 const ShaderResourceMask* resourceMasks )
{
	m_vertexFunction = vertexFunction;
	m_fragmentFunction = fragmentFunction;
	m_computeFunction = computeFunction;
	m_threadGroupSize = threadGroupSize;
	m_shaderResourceMasks = resourceMasks;
}

void MetalWorkQueue::SetViewport( float originX, float originY, float width, float height, float znear, float zfar )
{
	m_viewport = { originX, originY, width, height, znear, zfar };
	m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_VIEWPORT;
	m_validViewport = true;
}

void MetalWorkQueue::SetCullMode( MTLCullMode cullMode )
{
	if( m_cullMode != cullMode )
	{
		m_cullMode = cullMode;
		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_CULLMODE;
	}
}

void MetalWorkQueue::SetFillMode( MTLTriangleFillMode fillMode )
{
	if( m_fillMode != fillMode )
	{
		m_fillMode = fillMode;
		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_FILLMODE;
	}
}

void MetalWorkQueue::SetDepthBias( float* depthBias, float* slopeScale, float* clamp )
{
	if( depthBias )
		m_depthBias.depthBias = *depthBias;
	if( slopeScale )
		m_depthBias.slopeScale = *slopeScale;
	if( clamp )
		m_depthBias.clamp = *clamp;
	m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_DEPTHBIAS;
}

void MetalWorkQueue::SetDepthClipEnable( bool enable )
{
	m_depthClipMode = enable ? MTLDepthClipModeClip : MTLDepthClipModeClamp;
	m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_DEPTHCLIP;
}

void MetalWorkQueue::SetDepthState( bool enabled )
{
	if( m_depthStencilDescriptor.depthWriteEnabled != enabled )
	{
		m_depthStencilDescriptor.depthWriteEnabled = enabled;
		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_DEPTHSTENCILSTATE;
	}
}

void MetalWorkQueue::SetDepthCompareFn( MTLCompareFunction fn )
{
	if( m_depthStencilDescriptor.depthCompareFunction != fn )
	{
		m_depthStencilDescriptor.depthCompareFunction = fn;
		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_DEPTHSTENCILSTATE;
	}
}

void MetalWorkQueue::SetStencilState( bool enabled )
{
	if( enabled )
	{
		m_depthStencilDescriptor.frontFaceStencil = m_frontFaceStencilDescriptor;
		m_depthStencilDescriptor.backFaceStencil = m_backFaceStencilDescriptor;
	}
	else
	{
		m_depthStencilDescriptor.frontFaceStencil = nil;
		m_depthStencilDescriptor.backFaceStencil = nil;
	}

	m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_DEPTHSTENCILSTATE;
}

void MetalWorkQueue::SetStencilDepthFailOp( MTLStencilOperation op )
{
	if( m_frontFaceStencilDescriptor.depthFailureOperation != op ||
		m_backFaceStencilDescriptor.depthFailureOperation != op )
	{
		m_frontFaceStencilDescriptor.depthFailureOperation = op;
		m_backFaceStencilDescriptor.depthFailureOperation = op;

		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_DEPTHSTENCILSTATE;
	}
}

void MetalWorkQueue::SetStencilFailOp( MTLStencilOperation op )
{
	if( m_frontFaceStencilDescriptor.stencilFailureOperation != op ||
		m_backFaceStencilDescriptor.stencilFailureOperation != op )
	{
		m_frontFaceStencilDescriptor.stencilFailureOperation = op;
		m_backFaceStencilDescriptor.stencilFailureOperation = op;

		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_DEPTHSTENCILSTATE;
	}
}

void MetalWorkQueue::SetStencilPassOp( MTLStencilOperation op )
{
	if( m_frontFaceStencilDescriptor.depthStencilPassOperation != op ||
		m_backFaceStencilDescriptor.depthStencilPassOperation != op )
	{
		m_frontFaceStencilDescriptor.depthStencilPassOperation = op;
		m_backFaceStencilDescriptor.depthStencilPassOperation = op;

		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_DEPTHSTENCILSTATE;
	}
}

void MetalWorkQueue::SetStencilCompareFn( MTLCompareFunction fn )
{
	if( m_frontFaceStencilDescriptor.stencilCompareFunction != fn ||
		m_backFaceStencilDescriptor.stencilCompareFunction != fn )
	{
		m_frontFaceStencilDescriptor.stencilCompareFunction = fn;
		m_backFaceStencilDescriptor.stencilCompareFunction = fn;

		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_DEPTHSTENCILSTATE;
	}
}

void MetalWorkQueue::SetStencilRefValue( uint32_t value )
{
	if( m_stencilReferenceValue != value )
	{
		m_stencilReferenceValue = value;
		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_DEPTHSTENCILSTATE;
	}
}

void MetalWorkQueue::SetStencilMask( uint32_t mask )
{
	if( m_frontFaceStencilDescriptor.readMask != mask || m_frontFaceStencilDescriptor.writeMask != mask ||
		m_backFaceStencilDescriptor.readMask != mask || m_backFaceStencilDescriptor.writeMask != mask )
	{
		m_frontFaceStencilDescriptor.readMask = mask;
		m_frontFaceStencilDescriptor.writeMask = mask;

		m_backFaceStencilDescriptor.readMask = mask;
		m_backFaceStencilDescriptor.writeMask = mask;

		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_DEPTHSTENCILSTATE;
	}
}

void MetalWorkQueue::SetBlendColor( uint32_t attachmentIndex, uint32_t blendColor )
{
	m_blendState[attachmentIndex].blendColor.red = float( ( blendColor >> 0 ) & 0xff ) / 255.f;
	m_blendState[attachmentIndex].blendColor.green = float( ( blendColor >> 8 ) & 0xff ) / 255.f;
	m_blendState[attachmentIndex].blendColor.blue = float( ( blendColor >> 16 ) & 0xff ) / 255.f;
	m_blendState[attachmentIndex].blendColor.alpha = float( ( blendColor >> 24 ) & 0xff ) / 255.f;

	m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_BLENDCOLOR;
}

void MetalWorkQueue::SetSrcBlend( uint32_t attachmentIndex, MTLBlendFactor factor )
{
	if( m_blendState[attachmentIndex].srcColorFactor != factor )
	{
		m_blendState[attachmentIndex].srcColorFactor = factor;
		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_ATTACHMENTS;
	}
}

void MetalWorkQueue::SetDestBlend( uint32_t attachmentIndex, MTLBlendFactor factor )
{
	if( m_blendState[attachmentIndex].destColorFactor != factor )
	{
		m_blendState[attachmentIndex].destColorFactor = factor;
		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_ATTACHMENTS;
	}
}

void MetalWorkQueue::SetSrcBlendAlpha( uint32_t attachmentIndex, MTLBlendFactor factor )
{
	if( m_blendState[attachmentIndex].srcAlphaFactor != factor )
	{
		m_blendState[attachmentIndex].srcAlphaFactor = factor;
		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_ATTACHMENTS;
	}
}

void MetalWorkQueue::SetDestBlendAlpha( uint32_t attachmentIndex, MTLBlendFactor factor )
{
	if( m_blendState[attachmentIndex].destAlphaFactor != factor )
	{
		m_blendState[attachmentIndex].destAlphaFactor = factor;
		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_ATTACHMENTS;
	}
}

void MetalWorkQueue::SetBlendOp( uint32_t attachmentIndex, MTLBlendOperation op )
{
	if( m_blendState[attachmentIndex].rgbBlendOp != op )
	{
		m_blendState[attachmentIndex].rgbBlendOp = op;
		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_ATTACHMENTS;
	}
}

void MetalWorkQueue::SetBlendOpAlpha( uint32_t attachmentIndex, MTLBlendOperation op )
{
	if( m_blendState[attachmentIndex].alphaBlendOp != op )
	{
		m_blendState[attachmentIndex].alphaBlendOp = op;
		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_ATTACHMENTS;
	}
}

void MetalWorkQueue::SetBlendType( uint32_t attachmentIndex, MetalBlendType blendType )
{
	if( m_blendState[attachmentIndex].blendType != blendType )
	{
		m_blendState[attachmentIndex].blendType = blendType;
		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_ATTACHMENTS;
	}
}

void MetalWorkQueue::SetColorWriteMask( uint32_t attachmentIndex, MTLColorWriteMask writeMask )
{
	if( m_blendState[attachmentIndex].writeMask != writeMask )
	{
		m_blendState[attachmentIndex].writeMask = writeMask;
		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_ATTACHMENTS;
	}
}

void MetalWorkQueue::SetConstants( Tr2RenderContextEnum::ShaderType shaderType,
								   const void* constantBuffer,
								   uint32_t size,
								   ConstantBufferToken& token,
								   uint32_t bufferIndex )
{
	CCP_ASSERT( bufferIndex < METAL_CONST_BUFFER_COUNT );

	ConstantBuffer& bufferSlot = m_constBuffers[shaderType][bufferIndex];
	uint32_t flag = ( 1 << bufferIndex );

	UploadConstants( constantBuffer, size, token );
	uint32_t page, offset;
	uint64_t offs = token.offset;

	page = uint32_t( offs >> 32 );
	offset = uint32_t( offs & 0xffffffff );

	if( bufferSlot.offset != offset || bufferSlot.page != page )
	{
		if( bufferSlot.page != page )
		{
			m_dirtyConstBufferPageMask[shaderType] |= flag;
		}
		bufferSlot = { page, offset };
		m_dirtyConstBuffersMask[shaderType] |= flag;
	}
	m_activeConstBuffersMask[shaderType] |= flag;
}

void MetalWorkQueue::UploadConstants( const void* constantBuffer, uint32_t size, ConstantBufferToken& token )
{
	auto frameNo = m_context->GetRecordingFrameNumber();
	if( token.frame != frameNo )
	{
		auto& allocator = m_context->GetConstantBufferAllocator();
		auto entry = allocator.Allocate( constantBuffer, size );
		token.offset = uint64_t( entry.offset ) | ( uint64_t( entry.page ) << 32 );
		token.frame = frameNo;
	}
}

void MetalWorkQueue::SetBuffers( Tr2RenderContextEnum::ShaderType shaderType,
								 const id<MTLBuffer>* buffers,
								 uint32_t buffersMask,
								 id<MTLBuffer> heapView,
								 uint32_t heapViewMask )
{
	static_assert( sizeof( buffersMask ) * 8 >= METAL_MAX_BOUND_BUFFERS );
	CCP_ASSERT( ( buffersMask & METAL_VERTEX_STREAM_BUFFER_MASK ) == 0 );

	static_assert( sizeof( heapViewMask ) * 8 >= METAL_MAX_BOUND_BUFFERS );
	CCP_ASSERT( ( heapViewMask & METAL_VERTEX_STREAM_BUFFER_MASK ) == 0 );

	if( buffersMask == 0 && heapViewMask == 0 )
	{
		return;
	}

	uint32_t mask = buffersMask;
	while( mask )
	{
		int i = __builtin_ctz( mask );
		uint32_t flag = ( 1 << i );

		mask &= ~flag;

		MetalBuffer& bufferSlot = m_buffers[shaderType][i];
		if( bufferSlot.buffer != buffers[i] )
		{
			bufferSlot = { buffers[i], 0 };
			m_dirtyBuffersMask[shaderType] |= flag;
		}
	}
	mask = heapViewMask;
	while( mask )
	{
		int i = __builtin_ctz( mask );
		uint32_t flag = ( 1 << i );

		mask &= ~flag;

		MetalBuffer& bufferSlot = m_buffers[shaderType][i];
		if( bufferSlot.buffer != heapView )
		{
			bufferSlot = { heapView, 0 };
			m_dirtyBuffersMask[shaderType] |= flag;
		}
	}

	// Override all active buffers except stream buffers.
	uint32_t oldActiveBuffersMask = m_activeBuffersMask[shaderType];
	m_activeBuffersMask[shaderType] =
		buffersMask | heapViewMask | ( m_activeBuffersMask[shaderType] & METAL_VERTEX_STREAM_BUFFER_MASK );
}

void MetalWorkQueue::SetTextures( Tr2RenderContextEnum::ShaderType shaderType,
								  const id<MTLTexture>* textures,
								  NSRange texturesRange )
{
	CCP_ASSERT( NSMaxRange( texturesRange ) <= METAL_MAX_BOUND_TEXTURES );

	if( texturesRange.length == 0 )
	{
		return;
	}

	for( NSUInteger i = texturesRange.location, n = NSMaxRange( texturesRange ); i < n; ++i )
	{
		if( m_textures[shaderType][i] != textures[i] )
		{
			m_textures[shaderType][i] = textures[i];
			m_dirtyTexturesMask[shaderType] |= ( 1 << i );
		}
	}
}

void MetalWorkQueue::SetSamplers( Tr2RenderContextEnum::ShaderType shaderType,
								  const id<MTLSamplerState>* samplers,
								  NSRange samplersRange )
{
	CCP_ASSERT( NSMaxRange( samplersRange ) <= METAL_MAX_BOUND_SAMPLERS );

	if( samplersRange.length == 0 )
	{
		return;
	}

	for( NSUInteger i = samplersRange.location, n = NSMaxRange( samplersRange ); i < n; ++i )
	{
		if( m_samplers[shaderType][i] != samplers[i] )
		{
			m_samplers[shaderType][i] = samplers[i];
			m_dirtySamplersMask[shaderType] |= ( 1 << i );
		}
	}
}

void MetalWorkQueue::ResetBuffers( Tr2RenderContextEnum::ShaderType shaderType )
{
	for( size_t i = 0, n = sizeof( m_buffers[shaderType] ) / sizeof( *m_buffers[shaderType] ); i < n; ++i )
	{
		m_buffers[shaderType][i] = { nil, 0 };
	}
	for( size_t i = 0, n = sizeof( m_constBuffers[shaderType] ) / sizeof( *m_constBuffers[shaderType] ); i < n; ++i )
	{
		m_constBuffers[shaderType][i] = { 0, 0 };
	}

	m_activeConstBuffersMask[shaderType] = 0;
	m_activeBuffersMask[shaderType] = 0;
	m_dirtyConstBuffersMask[shaderType] = ~0u;
	m_dirtyConstBufferPageMask[shaderType] = ~0u;
	m_dirtyBuffersMask[shaderType] = ~0u;
}

void MetalWorkQueue::ResetTextures( Tr2RenderContextEnum::ShaderType shaderType )
{
	const size_t size = sizeof( m_textures[shaderType] ) / sizeof( *m_textures[shaderType] );
	for( size_t i = 0; i < size; ++i )
	{
		m_textures[shaderType][i] = nil;
	}

	m_dirtyTexturesMask[shaderType] = ~0u;
}

void MetalWorkQueue::ResetSamplers( Tr2RenderContextEnum::ShaderType shaderType )
{
	const size_t size = sizeof( m_samplers[shaderType] ) / sizeof( *m_samplers[shaderType] );
	for( size_t i = 0; i < size; ++i )
	{
		m_samplers[shaderType][i] = nil;
	}

	m_dirtySamplersMask[shaderType] = ~0u;
}

void MetalWorkQueue::SetVertexStream( uint32 stream, id<MTLBuffer> buffer, uint32 stride, uint32 offset )
{
	CCP_ASSERT( stream < METAL_VERTEX_STREAM_BUFFER_COUNT );
	const uint32_t bufferIndex = METAL_VERTEX_STREAM_BUFFER_OFFSET + stream;

	auto& boundBuffer = m_buffers[VERTEX_SHADER][bufferIndex];

	if( boundBuffer.buffer == buffer && boundBuffer.offset == offset && m_boundVertexStreams[stream].stride == stride )
	{
		return;
	}

	boundBuffer.buffer = buffer;
	boundBuffer.offset = offset;

	uint32_t flag = ( 1 << bufferIndex );
	m_activeBuffersMask[VERTEX_SHADER] |= flag;
	m_dirtyBuffersMask[VERTEX_SHADER] |= flag;

	if( m_boundVertexStreams[stream].stride != stride )
	{
		m_boundVertexStreams[stream].stride = stride;
		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_VERTEXDESCRIPTOR;
	}
}

void MetalWorkQueue::SetCurrentVertexDescriptor( MTLVertexDescriptor* vertexDescriptor,
												 uint8_t vertexStreamMask,
												 size_t baseHash )
{
	if( m_currentVertexDescriptor != vertexDescriptor || m_currentVertexStreamMask != vertexStreamMask )
	{
		m_currentVertexDescriptor = vertexDescriptor;
		m_currentVertexStreamMask = vertexStreamMask;
		m_currentVertexDescriptorBaseHash = baseHash;

		m_dirtyRenderEncoderState |= METAL_RENDERENCODERDIRTYSTATE_VERTEXDESCRIPTOR;
	}
}

void MetalWorkQueue::SetVertexBufferBindings()
{
	CCP_ASSERT( m_shaderResourceMasks != nullptr );

	// Bind buffers (excluding constant buffers).
	uint32_t dirtyMask = m_dirtyBuffersMask[VERTEX_SHADER];
	uint32_t mask = dirtyMask & m_activeBuffersMask[VERTEX_SHADER] &
		( m_shaderResourceMasks[VERTEX_SHADER].bufferMask | METAL_VERTEX_STREAM_BUFFER_MASK );
	while( mask )
	{
		int i = __builtin_ctz( mask );
		uint32_t flag = ( 1 << i );

		mask &= ~flag;
		m_dirtyBuffersMask[VERTEX_SHADER] &= ~flag;
		m_dirtyConstBuffersMask[VERTEX_SHADER] |= flag;
		m_dirtyConstBufferPageMask[VERTEX_SHADER] |= flag;

		[m_currentRenderEncoder setVertexBuffer:m_buffers[VERTEX_SHADER][i].buffer
										 offset:m_buffers[VERTEX_SHADER][i].offset
										atIndex:i];
	}

	// Bind constant buffers.
	dirtyMask = m_dirtyConstBuffersMask[VERTEX_SHADER];
	mask =
		dirtyMask & m_activeConstBuffersMask[VERTEX_SHADER] & m_shaderResourceMasks[VERTEX_SHADER].constantBufferMask;
	auto& allocator = m_context->GetConstantBufferAllocator();
	while( mask )
	{
		int i = __builtin_ctz( mask );
		uint32_t flag = ( 1 << i );

		mask &= ~flag;
		m_dirtyConstBuffersMask[VERTEX_SHADER] &= ~flag;
		m_dirtyBuffersMask[VERTEX_SHADER] |= flag;

		auto& cbd = m_constBuffers[VERTEX_SHADER][i];
		if( ( m_dirtyConstBufferPageMask[VERTEX_SHADER] & flag ) != 0 )
		{
			m_dirtyConstBufferPageMask[VERTEX_SHADER] &= ~flag;
			[m_currentRenderEncoder setVertexBuffer:allocator.GetPage( cbd.page ) offset:cbd.offset atIndex:i];
		}
		else
		{
			[m_currentRenderEncoder setVertexBufferOffset:cbd.offset atIndex:i];
		}
	}

	dirtyMask = m_dirtyTexturesMask[VERTEX_SHADER];
	mask = dirtyMask & m_shaderResourceMasks[VERTEX_SHADER].textureMask;
	while( mask )
	{
		int i = __builtin_ctz( mask );
		uint32_t flag = ( 1 << i );

		mask &= ~flag;
		dirtyMask &= ~flag;

		[m_currentRenderEncoder setVertexTexture:m_textures[VERTEX_SHADER][i] atIndex:i];
	}
	m_dirtyTexturesMask[VERTEX_SHADER] = dirtyMask;

	dirtyMask = m_dirtySamplersMask[VERTEX_SHADER];
	mask = dirtyMask & m_shaderResourceMasks[VERTEX_SHADER].samplerMask;
	while( mask )
	{
		int i = __builtin_ctz( mask );
		uint32_t flag = ( 1 << i );

		mask &= ~flag;
		dirtyMask &= ~flag;

		[m_currentRenderEncoder setVertexSamplerState:m_samplers[VERTEX_SHADER][i] atIndex:i];
	}
	m_dirtySamplersMask[VERTEX_SHADER] = dirtyMask;
}

void MetalWorkQueue::SetFragmentBufferBindings()
{
	CCP_ASSERT( m_shaderResourceMasks != nullptr );

	// Bind buffers (excluding constant buffers).
	uint32_t dirtyMask = m_dirtyBuffersMask[PIXEL_SHADER];
	uint32_t mask = dirtyMask & m_activeBuffersMask[PIXEL_SHADER] & m_shaderResourceMasks[PIXEL_SHADER].bufferMask;
	while( mask )
	{
		int i = __builtin_ctz( mask );
		uint32_t flag = ( 1 << i );

		mask &= ~flag;
		m_dirtyBuffersMask[PIXEL_SHADER] &= ~flag;
		m_dirtyConstBuffersMask[PIXEL_SHADER] |= flag;
		m_dirtyConstBufferPageMask[PIXEL_SHADER] |= flag;

		[m_currentRenderEncoder setFragmentBuffer:m_buffers[PIXEL_SHADER][i].buffer
										   offset:m_buffers[PIXEL_SHADER][i].offset
										  atIndex:i];
	}

	// Bind constant buffers.
	dirtyMask = m_dirtyConstBuffersMask[PIXEL_SHADER];
	mask = dirtyMask & m_activeConstBuffersMask[PIXEL_SHADER] & m_shaderResourceMasks[PIXEL_SHADER].constantBufferMask;
	auto& allocator = m_context->GetConstantBufferAllocator();
	while( mask )
	{
		int i = __builtin_ctz( mask );
		uint32_t flag = ( 1 << i );

		mask &= ~flag;
		m_dirtyConstBuffersMask[PIXEL_SHADER] &= ~flag;
		m_dirtyBuffersMask[PIXEL_SHADER] |= flag;

		auto& cbd = m_constBuffers[PIXEL_SHADER][i];
		if( ( m_dirtyConstBufferPageMask[PIXEL_SHADER] & flag ) != 0 )
		{
			m_dirtyConstBufferPageMask[PIXEL_SHADER] &= ~flag;
			[m_currentRenderEncoder setFragmentBuffer:allocator.GetPage( cbd.page ) offset:cbd.offset atIndex:i];
		}
		else
		{
			[m_currentRenderEncoder setFragmentBufferOffset:cbd.offset atIndex:i];
		}
	}

	dirtyMask = m_dirtyTexturesMask[PIXEL_SHADER];
	mask = dirtyMask & m_shaderResourceMasks[PIXEL_SHADER].textureMask;
	while( mask )
	{
		int i = __builtin_ctz( mask );
		uint32_t flag = ( 1 << i );

		mask &= ~flag;
		dirtyMask &= ~flag;

		[m_currentRenderEncoder setFragmentTexture:m_textures[PIXEL_SHADER][i] atIndex:i];
	}
	m_dirtyTexturesMask[PIXEL_SHADER] = dirtyMask;

	dirtyMask = m_dirtySamplersMask[PIXEL_SHADER];
	mask = dirtyMask & m_shaderResourceMasks[PIXEL_SHADER].samplerMask;
	while( mask )
	{
		int i = __builtin_ctz( mask );
		uint32_t flag = ( 1 << i );

		mask &= ~flag;
		dirtyMask &= ~flag;

		[m_currentRenderEncoder setFragmentSamplerState:m_samplers[PIXEL_SHADER][i] atIndex:i];
	}
	m_dirtySamplersMask[PIXEL_SHADER] = dirtyMask;
}

void MetalWorkQueue::SetComputeBufferBindings()
{
	CCP_ASSERT( m_isPrimary );
	CCP_ASSERT( m_shaderResourceMasks != nullptr );

	// Bind buffers (excluding constant buffers).
	uint32_t dirtyMask = m_dirtyBuffersMask[COMPUTE_SHADER];
	uint32_t mask = dirtyMask & m_activeBuffersMask[COMPUTE_SHADER] & m_shaderResourceMasks[COMPUTE_SHADER].bufferMask;
	while( mask )
	{
		int i = __builtin_ctz( mask );
		uint32_t flag = ( 1 << i );

		mask &= ~flag;
		m_dirtyBuffersMask[COMPUTE_SHADER] &= ~flag;
		m_dirtyConstBuffersMask[COMPUTE_SHADER] |= flag;
		m_dirtyConstBufferPageMask[COMPUTE_SHADER] |= flag;

		[m_currentComputeEncoder setBuffer:m_buffers[COMPUTE_SHADER][i].buffer
									offset:m_buffers[COMPUTE_SHADER][i].offset
								   atIndex:i];
	}

	// Bind constant buffers.
	dirtyMask = m_dirtyConstBuffersMask[COMPUTE_SHADER];
	mask =
		dirtyMask & m_activeConstBuffersMask[COMPUTE_SHADER] & m_shaderResourceMasks[COMPUTE_SHADER].constantBufferMask;
	auto& allocator = m_context->GetConstantBufferAllocator();
	while( mask )
	{
		int i = __builtin_ctz( mask );
		uint32_t flag = ( 1 << i );

		mask &= ~flag;
		m_dirtyConstBuffersMask[COMPUTE_SHADER] &= ~flag;
		m_dirtyBuffersMask[COMPUTE_SHADER] |= flag;

		auto& cbd = m_constBuffers[COMPUTE_SHADER][i];
		if( ( m_dirtyConstBufferPageMask[COMPUTE_SHADER] & flag ) != 0 )
		{
			m_dirtyConstBufferPageMask[COMPUTE_SHADER] &= ~flag;
			[m_currentComputeEncoder setBuffer:allocator.GetPage( cbd.page ) offset:cbd.offset atIndex:i];
		}
		else
		{
			[m_currentComputeEncoder setBufferOffset:cbd.offset atIndex:i];
		}
	}

	dirtyMask = m_dirtyTexturesMask[COMPUTE_SHADER];
	mask = dirtyMask & m_shaderResourceMasks[COMPUTE_SHADER].textureMask;
	while( mask )
	{
		int i = __builtin_ctz( mask );
		uint32_t flag = ( 1 << i );

		mask &= ~flag;
		dirtyMask &= ~flag;

		[m_currentComputeEncoder setTexture:m_textures[COMPUTE_SHADER][i] atIndex:i];
	}
	m_dirtyTexturesMask[COMPUTE_SHADER] = dirtyMask;

	dirtyMask = m_dirtySamplersMask[COMPUTE_SHADER];
	mask = dirtyMask & m_shaderResourceMasks[COMPUTE_SHADER].samplerMask;
	while( mask )
	{
		int i = __builtin_ctz( mask );
		uint32_t flag = ( 1 << i );

		mask &= ~flag;
		dirtyMask &= ~flag;

		[m_currentComputeEncoder setSamplerState:m_samplers[COMPUTE_SHADER][i] atIndex:i];
	}
	m_dirtySamplersMask[COMPUTE_SHADER] = dirtyMask;
}

void MetalWorkQueue::ClearAttachment( MTLClearColor* clearColor,
									  float* clearDepth,
									  uint32_t* clearStencil,
									  uint32_t attachmentIndex )
{
	CCP_ASSERT( m_isPrimary );
	if( m_hasPendingRenderPassHint )
	{
		return;
	}

	// If we have a current encoder release it and end it (so we can set some new clear state)
	FlushOutstandingOperations();

	CCP_ASSERT( attachmentIndex < METAL_MAX_RENDER_TARGETS );

	if( clearColor )
	{
		m_currentRenderPassDescriptor.colorAttachments[attachmentIndex].clearColor = *clearColor;
		m_currentRenderPassDescriptor.colorAttachments[attachmentIndex].loadAction = MTLLoadActionClear;
	}

	if( clearDepth )
	{
		m_currentRenderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
		m_currentRenderPassDescriptor.depthAttachment.clearDepth = *clearDepth;
	}

	if( clearStencil )
	{
		m_currentRenderPassDescriptor.stencilAttachment.loadAction = MTLLoadActionClear;
		m_currentRenderPassDescriptor.stencilAttachment.clearStencil = *clearStencil;
	}

	m_pendingClear = true;
}

void MetalWorkQueue::DrawPrimitives( MTLPrimitiveType primitiveType,
									 uint32_t numVertices,
									 uint32_t startVertex,
									 uint32_t numInstances,
									 uint32_t startInstance )
{
	id<MTLRenderCommandEncoder> renderEncoder = GetRenderEncoder();
	if( EmitRenderEncoderState() )
	{
		if( numInstances <= 1 )
		{
			[renderEncoder drawPrimitives:primitiveType vertexStart:startVertex vertexCount:numVertices];
		}
		else
		{
			[renderEncoder drawPrimitives:primitiveType
							  vertexStart:startVertex
							  vertexCount:numVertices
							instanceCount:numInstances
							 baseInstance:startInstance];
		}
	}
	ReleaseEncoder( false );
}

void MetalWorkQueue::DrawPrimitives( MTLPrimitiveType primitiveType,
									 id<MTLBuffer> indirectBuffer,
									 uint32_t indirectBufferOffset )
{
	id<MTLRenderCommandEncoder> renderEncoder = GetRenderEncoder();
	if( EmitRenderEncoderState() )
	{
		[renderEncoder drawPrimitives:primitiveType
					   indirectBuffer:indirectBuffer
				 indirectBufferOffset:indirectBufferOffset];
	}
	ReleaseEncoder( false );
}

void MetalWorkQueue::DrawIndexedPrimitives( MTLPrimitiveType primitiveType,
											uint32_t numIndices,
											MTLIndexType indexType,
											id<MTLBuffer> indexBuffer,
											uint32_t startIndex,
											uint32_t numInstances,
											int32_t baseVertex,
											uint32_t baseInstance )
{
	id<MTLRenderCommandEncoder> renderEncoder = GetRenderEncoder();
	if( EmitRenderEncoderState() )
	{
		[renderEncoder drawIndexedPrimitives:primitiveType
								  indexCount:numIndices
								   indexType:indexType
								 indexBuffer:indexBuffer
						   indexBufferOffset:startIndex * ( indexType == MTLIndexTypeUInt16 ? 2 : 4 )
							   instanceCount:MAX( 1, numInstances )
								  baseVertex:baseVertex
								baseInstance:baseInstance];
	}
	ReleaseEncoder( false );
}

void MetalWorkQueue::DrawIndexedPrimitives( MTLPrimitiveType primitiveType,
											MTLIndexType indexType,
											id<MTLBuffer> indexBuffer,
											uint32_t indexBufferOffset,
											id<MTLBuffer> indirectBuffer,
											uint32_t indirectBufferOffset )
{
	id<MTLRenderCommandEncoder> renderEncoder = GetRenderEncoder();
	if( EmitRenderEncoderState() )
	{
		[renderEncoder drawIndexedPrimitives:primitiveType
								   indexType:indexType
								 indexBuffer:indexBuffer
						   indexBufferOffset:indexBufferOffset
							  indirectBuffer:indirectBuffer
						indirectBufferOffset:indirectBufferOffset];
	}
	ReleaseEncoder( false );
}

void MetalWorkQueue::CreateVisibilityQueryBuffer( uint64_t maxNumQueries )
{
	CCP_ASSERT( m_isPrimary );
	if( m_visibilityBuffer == nil )
	{
		// We'll create a circular buffer to store all queries that is 3 (likely max number of back buffers) times the same of the max query number.
		m_maxVisibilityQueries = maxNumQueries * 3;
		uint64_t bufferSizeInBytes = m_maxVisibilityQueries * METAL_VISIBILITY_RESULT_SIZE_IN_BYTES;
		m_visibilityBuffer = [m_device newBufferWithLength:bufferSizeInBytes options:MTLResourceStorageModeShared];
		m_currentVisibilityQueryIndex = 0;
		m_drawRenderPassDescriptor.visibilityResultBuffer = m_visibilityBuffer;
	}
}

uint64_t MetalWorkQueue::StartVisibilityQuery()
{
	CCP_ASSERT( m_isPrimary );
	// METAL_LOG( @"Begin query %llu", m_currentVisibilityQueryIndex );

	// Wrap buffer index.
	uint64_t bufferIndex = m_currentVisibilityQueryIndex % m_maxVisibilityQueries;
	uint64_t bufferOffset = bufferIndex * METAL_VISIBILITY_RESULT_SIZE_IN_BYTES;

	CCP_ASSERT( m_visibilityBuffer != nil );
	CCP_ASSERT( bufferOffset < m_visibilityBuffer.length );

	uint64_t* data = (uint64_t*)m_visibilityBuffer.contents;
	data[bufferIndex] = 0;

	id<MTLRenderCommandEncoder> encoder = GetRenderEncoder();
	[encoder setVisibilityResultMode:MTLVisibilityResultModeCounting offset:bufferOffset];
	m_visibilityQueryInProgress = true;
	ReleaseEncoder( false );

	++m_visibilityQueriesInThisEncoder;
	CCP_ASSERT( m_visibilityQueriesInThisEncoder < m_maxVisibilityQueries );

	return m_currentVisibilityQueryIndex++;
}

void MetalWorkQueue::EndVisibilityQuery( uint64_t queryNumber )
{
	CCP_ASSERT( m_isPrimary );
	// METAL_LOG( @"End query %llu", queryNumber );

	uint64_t bufferIndex = queryNumber % m_maxVisibilityQueries;
	uint64_t bufferOffset = bufferIndex * METAL_VISIBILITY_RESULT_SIZE_IN_BYTES;

	CCP_ASSERT( m_visibilityBuffer != nil );
	CCP_ASSERT( bufferOffset < m_visibilityBuffer.length );

	id<MTLRenderCommandEncoder> encoder = GetRenderEncoder();
	[encoder setVisibilityResultMode:MTLVisibilityResultModeDisabled offset:bufferOffset];
	m_visibilityQueryInProgress = false;
	ReleaseEncoder( false );
}

bool MetalWorkQueue::GetVisibilityQueryPixelCount( uint64_t queryNumber,
												   uint64_t* pixelCount,
												   bool finishOutstandingWork )
{
	CCP_ASSERT( m_isPrimary );
	if( finishOutstandingWork )
	{
		FlushOutstandingOperations();
		[m_commandBuffer addCompletedHandler:^( id<MTLCommandBuffer> commandBuffer ) {
			if( @available( macOS 10.15, * ) )
			{
				auto duration = commandBuffer.GPUEndTime - commandBuffer.GPUStartTime;
				s_gpuFrameTime += duration;
			}
		}];
		CommitCommandBuffer( MTLCBCOMMIT_WAITUNTILCOMPLETE );
	}

	// See if this query has completed yet.
	// We use a 64 bit value to provide a unique query index so when that wraps this may provide an incorrect
	// result for that frame but given this will only happen once every 1.8*10^19 queries we should be... OK.
	if( queryNumber < m_numCompletedVisibilityQueries )
	{
		uint64_t bufferIndex = queryNumber % m_maxVisibilityQueries;
		uint64_t bufferOffset = bufferIndex * METAL_VISIBILITY_RESULT_SIZE_IN_BYTES;

		CCP_ASSERT( m_visibilityBuffer != nil );
		CCP_ASSERT( bufferOffset < m_visibilityBuffer.length );

		uint64_t* data = (uint64_t*)m_visibilityBuffer.contents;

		// METAL_LOG(@"Get Query count %llu (%llu)", queryNumber, data[bufferIndex]);
		*pixelCount = data[bufferIndex];
		return true;
	}
	else
	{
		// METAL_LOG(@"Get Query count %llu not ready", queryNumber);
		return false;
	}
}

void MetalWorkQueue::Dispatch( uint32_t groupDimX, uint32_t groupDimY, uint32_t groupDimZ )
{
	CCP_ASSERT( m_isPrimary );
	id<MTLComputeCommandEncoder> computeEncoder = GetComputeEncoder();
	if( EmitComputeEncoderState() )
	{
		[computeEncoder dispatchThreadgroups:MTLSizeMake( groupDimX, groupDimY, groupDimZ )
					   threadsPerThreadgroup:m_threadGroupSize];
	}
	ReleaseEncoder( false );
}

void MetalWorkQueue::Dispatch( id<MTLBuffer> indirectBuffer, uint32_t indirectBufferOffset )
{
	CCP_ASSERT( m_isPrimary );
	id<MTLComputeCommandEncoder> computeEncoder = GetComputeEncoder();
	if( EmitComputeEncoderState() )
	{
		[computeEncoder dispatchThreadgroupsWithIndirectBuffer:indirectBuffer
										  indirectBufferOffset:indirectBufferOffset
										 threadsPerThreadgroup:m_threadGroupSize];
	}
	ReleaseEncoder( false );
}

ConstantBufferAllocator::Entry
	MetalWorkQueue::UploadArgumentBuffer( const Tr2ShaderSignatureAL& signature,
										  std::vector<id<MTLResource>>& readResources,
										  std::vector<id<MTLResource>>& writeResources ) const
{
	if( @available( macOS 13.0, * ) )
	{
		auto& allocator = m_context->GetConstantBufferAllocator();
		std::vector<uint64_t> data;

		for( auto& reg : signature.registers )
		{
			switch( reg.registerType )
			{
			case Tr2ShaderRegisterAL::CONSTANT_BUFFER: {
				auto& cbd = m_constBuffers[COMPUTE_SHADER][METAL_CONST_BUFFER_OFFSET + reg.registerIndex];
				data.push_back( allocator.GetPage( cbd.page ).gpuAddress + cbd.offset );
				readResources.push_back( allocator.GetPage( cbd.page ) );
			}
			break;
			case Tr2ShaderRegisterAL::SAMPLER:
				data.push_back( m_samplers[COMPUTE_SHADER][reg.registerIndex].gpuResourceID._impl );
				break;
			case Tr2ShaderRegisterAL::SRV_BUFFER:
			case Tr2ShaderRegisterAL::SRV_STRUCTURED_BUFFER: {
				auto& buffer = m_buffers[COMPUTE_SHADER][METAL_SRV_BUFFER_OFFSET + reg.registerIndex];
				data.push_back( buffer.buffer.gpuAddress + buffer.offset );
				readResources.push_back( buffer.buffer );
			}
			break;
			case Tr2ShaderRegisterAL::UAV_BUFFER:
			case Tr2ShaderRegisterAL::UAV_STRUCTURED_BUFFER: {
				auto& buffer = m_buffers[COMPUTE_SHADER][METAL_UAV_BUFFER_OFFSET + reg.registerIndex];
				data.push_back( buffer.buffer.gpuAddress + buffer.offset );
				writeResources.push_back( buffer.buffer );
			}
			break;
			default:
				if( reg.IsUav() )
				{
					auto& texture = m_textures[COMPUTE_SHADER][METAL_UAV_TEXTURE_OFFSET + reg.registerIndex];
					data.push_back( texture.gpuResourceID._impl );
					writeResources.push_back( texture );
				}
				else
				{
					auto& texture = m_textures[COMPUTE_SHADER][METAL_SRV_TEXTURE_OFFSET + reg.registerIndex];
					data.push_back( texture.gpuResourceID._impl );
					readResources.push_back( texture );
				}
				break;
			}
		}
		return allocator.Allocate( data.data(), uint32_t( data.size() * sizeof( uint64_t ) ) );
	}
	else
	{
		return {};
	}
}

void MetalWorkQueue::DispatchRays( Tr2RtPipelineStateAL* pipeline,
								   Tr2RtShaderTableAL* shaderTable,
								   uint32_t rayGenIndex,
								   uint32_t width,
								   uint32_t height,
								   uint32_t depth )
{
	CCP_ASSERT( m_isPrimary );
	if( @available( macOS 13.0, * ) )
	{
		id<MTLComputeCommandEncoder> computeEncoder = GetComputeEncoder();

		auto& allocator = m_context->GetConstantBufferAllocator();

		std::vector<id<MTLResource>> readResources;
		std::vector<id<MTLResource>> writeResources;


		[computeEncoder setBuffer:shaderTable->GetMaterialBuffer()
						   offset:shaderTable->GetRayGenMaterialOffset( rayGenIndex )
						  atIndex:METAL_SRV_BUFFER_OFFSET + 0];


		auto globalInputGpu = UploadArgumentBuffer( pipeline->m_globalSignature, readResources, writeResources );

		[computeEncoder setBuffer:allocator.GetPage( globalInputGpu.page )
						   offset:globalInputGpu.offset
						  atIndex:METAL_SRV_BUFFER_OFFSET + 1];
		readResources.push_back( allocator.GetPage( globalInputGpu.page ) );

		auto shaderTableData = shaderTable->GetShaderTableData(
			rayGenIndex, allocator.GetPage( globalInputGpu.page ).gpuAddress + globalInputGpu.offset );
		auto shaderTableGpu = allocator.Allocate( &shaderTableData, sizeof( shaderTableData ) );
		[computeEncoder setBuffer:allocator.GetPage( shaderTableGpu.page )
						   offset:shaderTableGpu.offset
						  atIndex:METAL_SRV_BUFFER_OFFSET + 2];
		readResources.push_back( allocator.GetPage( shaderTableGpu.page ) );

		shaderTable->AddUsedResources( rayGenIndex, readResources );

		shaderTable->SetGlobalInputBuffer(
			rayGenIndex, allocator.GetPage( globalInputGpu.page ), globalInputGpu.offset );

		[computeEncoder useResources:readResources.data() count:readResources.size() usage:MTLResourceUsageRead];
		[computeEncoder useResources:writeResources.data() count:writeResources.size() usage:MTLResourceUsageWrite];
		[computeEncoder setComputePipelineState:pipeline->GetRtPipeline( rayGenIndex )];

		// Launch a rectangular grid of threads on the GPU to perform ray tracing, with one thread per
		// pixel. The sample needs to align the number of threads to a multiple of the threadgroup
		// size, because earlier, when it created the pipeline objects, it declared that the pipeline
		// would always use a threadgroup size that's a multiple of the thread execution width
		// (SIMD group size). An 8x8 threadgroup is a safe threadgroup size and small enough to be
		// supported on most devices. A more advanced app would choose the threadgroup size dynamically.
		MTLSize threadsPerThreadgroup = MTLSizeMake( 8, 8, 1 );
		if( depth > 1 )
		{
			threadsPerThreadgroup = MTLSizeMake( 4, 4, 4 );
		}

		MTLSize threadgroups =
			MTLSizeMake( ( width + threadsPerThreadgroup.width - 1 ) / threadsPerThreadgroup.width,
						 ( height + threadsPerThreadgroup.height - 1 ) / threadsPerThreadgroup.height,
						 ( depth + threadsPerThreadgroup.depth - 1 ) / threadsPerThreadgroup.depth );

		// Dispatch the compute kernel to perform ray tracing.
		[computeEncoder dispatchThreadgroups:threadgroups threadsPerThreadgroup:threadsPerThreadgroup];

		ReleaseEncoder( false );
	}
}


bool MetalWorkQueue::SampleCounter( id buffer, NSUInteger index, CounterType counter )
{
	CCP_ASSERT( m_isPrimary );
	if( @available( macOS 10.15, * ) )
	{
		switch( m_currentEncoderType )
		{
		case MTLENCODERTYPE_RENDER: {
			[m_currentRenderEncoder sampleCountersInBuffer:buffer atSampleIndex:index withBarrier:YES];
			return true;
		}
		case MTLENCODERTYPE_COMPUTE: {
			if( counter != COUNTER_PIPELINE_STATS )
			{
				[m_currentComputeEncoder sampleCountersInBuffer:buffer atSampleIndex:index withBarrier:YES];
				return true;
			}
			break;
		}
		case MTLENCODERTYPE_BLIT: {
			if( counter != COUNTER_PIPELINE_STATS )
			{
				[m_currentBlitEncoder sampleCountersInBuffer:buffer atSampleIndex:index withBarrier:YES];
				return true;
			}
			break;
		}
		default: {
			id<MTLRenderCommandEncoder> renderEncoder = GetRenderEncoder();
			[renderEncoder sampleCountersInBuffer:buffer atSampleIndex:index withBarrier:YES];
			ReleaseEncoder( false );
			return true;
		}
		}
	}
	return false;
}

void MetalWorkQueue::PushDebugGroup( const char* name )
{
	CCP_ASSERT( m_isPrimary );
	if( !m_commandBuffer )
	{
		CreateCommandBuffer();
	}
	NSString* nsName = [[NSString alloc] initWithCString:name encoding:NSUTF8StringEncoding];
	[m_commandBuffer pushDebugGroup:nsName];
#if !__has_feature( objc_arc )
	[nsName release];
#endif
}

void MetalWorkQueue::PopDebugGroup()
{
	CCP_ASSERT( m_isPrimary );
	ReleaseEncoder( true );
	[m_commandBuffer popDebugGroup];
}

void MetalWorkQueue::RenderPassHint( const MetalRenderPassHint& hint )
{
	if( m_hasPendingRenderPassHint )
	{
		GetRenderEncoder();
		ReleaseEncoder( true );
	}
	m_pendingRenderPassHint = hint;
	m_hasPendingRenderPassHint = true;
}

void MetalWorkQueue::EndRenderPassHint()
{
	if( m_hasPendingRenderPassHint || m_pendingClear )
	{
		GetRenderEncoder();
		ReleaseEncoder( true );
	}
}

void MetalWorkQueue::BeginParallelEncoding( MetalWorkQueue* primaryQueue )
{
	m_isPrimary = false;

	id<MTLParallelRenderCommandEncoder> masterEncoder = primaryQueue->GetParallelEncoder();
	m_currentRenderEncoder = [masterEncoder renderCommandEncoder];

	m_device = primaryQueue->m_device;
	m_commandBuffer = primaryQueue->m_commandBuffer;
	m_currentRenderPassDescriptor = primaryQueue->m_currentRenderPassDescriptor;
	m_numRenderAttachments = primaryQueue->m_numRenderAttachments;
	m_currentAttachmentsHash = primaryQueue->m_currentAttachmentsHash;

	m_vertexFunction = primaryQueue->m_vertexFunction;
	m_fragmentFunction = primaryQueue->m_fragmentFunction;

	for( int i = 0; i < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
	{
		m_activeConstBuffersMask[i] = primaryQueue->m_activeConstBuffersMask[i];
		m_activeBuffersMask[i] = primaryQueue->m_activeBuffersMask[i];

		for( int b = 0; b < METAL_MAX_BOUND_BUFFERS; ++b )
		{
			m_constBuffers[i][b] = primaryQueue->m_constBuffers[i][b];
			m_buffers[i][b] = primaryQueue->m_buffers[i][b];
		}

		for( int t = 0; t < METAL_MAX_BOUND_TEXTURES; ++t )
		{
			m_textures[i][t] = primaryQueue->m_textures[i][t];
		}

		for( int s = 0; s < METAL_MAX_BOUND_SAMPLERS; ++s )
		{
			m_samplers[i][s] = primaryQueue->m_samplers[i][s];
		}
	}

	m_cullMode = primaryQueue->m_cullMode;
	m_validViewport = primaryQueue->m_validViewport;
	m_viewport = primaryQueue->m_viewport;
	m_scissorRect = primaryQueue->m_scissorRect;

	for( uint32_t i = 0; i < METAL_MAX_RENDER_TARGETS; ++i )
	{
		m_blendState[i] = primaryQueue->m_blendState[i];
	}

	m_depthStencilDescriptor = [primaryQueue->m_depthStencilDescriptor copy];
	m_frontFaceStencilDescriptor = [primaryQueue->m_frontFaceStencilDescriptor copy];
	m_backFaceStencilDescriptor = [primaryQueue->m_backFaceStencilDescriptor copy];

	m_depthBias = primaryQueue->m_depthBias;

	m_currentVertexDescriptor = [primaryQueue->m_currentVertexDescriptor copy];
	m_currentVertexDescriptorHash = primaryQueue->m_currentVertexDescriptorHash;
	m_currentVertexDescriptorBaseHash = primaryQueue->m_currentVertexDescriptorBaseHash;
	m_currentVertexStreamMask = primaryQueue->m_currentVertexStreamMask;

	for( int i = 0; i < METAL_VERTEX_STREAM_BUFFER_COUNT; ++i )
	{
		m_boundVertexStreams[i] = primaryQueue->m_boundVertexStreams[i];
	}

	// Since the encoder is new we'll need to emit all the state again
	SetRenderStatesDirty();

	// Mark that this curent encoder is in use
	m_currentEncoderType = MTLENCODERTYPE_RENDER;
	m_encoderInUse = false;
	m_encoderEnded = false;
	m_encoderHasWork = false;

	m_numCommands = 0;
}

void MetalWorkQueue::EndParallelEncoding()
{
	FlushCachedVertexDescriptors();

	if( m_isPrimary )
	{
		if( m_currentParallelEncoder )
		{
			[m_currentParallelEncoder endEncoding];
			m_currentParallelEncoder = nil;
		}
	}
	else
	{
		[m_currentRenderEncoder endEncoding];
		m_currentRenderEncoder = nil;

		ResetWorkQueue();

		m_device = nil;
		m_commandBuffer = nil;

		m_currentRenderPassDescriptor = nil;
		m_vertexFunction = nil;
		m_fragmentFunction = nil;
		m_depthStencilDescriptor = nil;
		m_frontFaceStencilDescriptor = nil;
		m_backFaceStencilDescriptor = nil;
		m_currentVertexDescriptor = nil;

		const ShaderType shaderTypes[] = { VERTEX_SHADER, PIXEL_SHADER, COMPUTE_SHADER };
		for( auto shaderType : shaderTypes )
		{
			ResetBuffers( shaderType );
			ResetTextures( shaderType );
			ResetSamplers( shaderType );
		}

		SetRenderStatesDirty();

		m_vertexFunction = nil;
		m_fragmentFunction = nil;
		m_computeFunction = nil;
		m_threadGroupSize = MTLSizeMake( 1, 1, 1 );
		m_shaderResourceMasks = nullptr;

		m_currentVertexDescriptor = nil;

		m_currentRenderPipelineState = nullptr;
		m_pipelineQueriesInProgress.clear();
	}
}

bool MetalWorkQueue::CanBeginParallelEncoding() const
{
	return !m_visibilityQueryInProgress;
}

void MetalWorkQueue::PipelineQueryStarted( Tr2PipelineStatsQueryAL* query )
{
	m_pipelineQueriesInProgress.push_back( query );
}

void MetalWorkQueue::PipelineQueryEnded( Tr2PipelineStatsQueryAL* query )
{
	auto found = std::find( begin( m_pipelineQueriesInProgress ), end( m_pipelineQueriesInProgress ), query );
	if( found != end( m_pipelineQueriesInProgress ) )
	{
		m_pipelineQueriesInProgress.erase( found );
	}
}

MTLVertexDescriptor* MetalWorkQueue::GetCachedVertexDescriptor( Tr2VertexLayoutAL* layout,
																size_t inputHash,
																uint8_t& streamMask,
																bool& needsDummyStream ) const
{
	for( auto& desc : m_cachedVertexLayouts )
	{
		if( desc.layout == layout && desc.inputHash == inputHash )
		{
			streamMask = desc.streamMask;
			needsDummyStream = desc.needsDummyStream;
			return desc.descriptor;
		}
	}
	return nullptr;
}

void MetalWorkQueue::CacheVertexDescriptor( Tr2VertexLayoutAL* layout,
											size_t inputHash,
											MTLVertexDescriptor* descriptor,
											uint8_t streamMask,
											bool needsDummyStream )
{
	m_cachedVertexLayouts.push_back( { layout, inputHash, descriptor, streamMask, needsDummyStream } );
}

void MetalWorkQueue::FlushCachedVertexDescriptors()
{
	for( auto& desc : m_cachedVertexLayouts )
	{
		desc.layout->AddVertexDescriptor( desc.inputHash, desc.descriptor, desc.streamMask, desc.needsDummyStream );
	}
	m_cachedVertexLayouts.clear();
}

uint64_t MetalWorkQueue::GetCurrentEncoderIndex() const
{
	return m_encoderIndex;
}

bool MetalWorkQueue::IsSilicon() const
{
	return m_isAppleSilicon;
}

}


#endif // TrinityALImpl
