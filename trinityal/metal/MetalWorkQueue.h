// Copyright © 2020 CCP ehf.

#pragma once
#if TRINITY_PLATFORM == TRINITY_METAL

#include <Metal/Metal.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

#include "MetalUtils.h"
#include "../include/Tr2ShaderAL.h"

namespace TrinityALImpl
{

class MetalContext;
class Tr2PipelineStatsQueryAL;
class Tr2VertexLayoutAL;
class Tr2RtPipelineStateAL;
class Tr2RtShaderTableAL;


// Values below must be synchronized with (propagated to) ShaderCompiler/EffectCompilerMetal.cpp
// and TrinityALTest/Shaders.metal/MetalDefines.h (CBUFFER, SRV, UAV, UAVT).
// See https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf
#define METAL_MAX_BOUND_BUFFERS 31
#define METAL_MAX_BOUND_TEXTURES 31 // 31+ on iOS, 128 on macOS
#define METAL_MAX_BOUND_SAMPLERS 16
// #define METAL_MAX_VERTEX_STREAMS 31
#define METAL_MAX_VERTEX_ATTRIBUTES 31
#define METAL_MAX_RENDER_TARGETS 4 // Matches DX12 define (RENDER_TARGET_COUNT)

// If you change any of those values all shaders must be recompiled.
#define METAL_VERTEX_STREAM_BUFFER_OFFSET 0
#define METAL_VERTEX_STREAM_BUFFER_COUNT 4
#define METAL_VERTEX_STREAM_BUFFER_MASK 0b1111 // Must take into account the value of METAL_VERTEX_STREAM_BUFFER_OFFSET
#define METAL_VERTEX_STREAM_DUMMY 3
#define METAL_CONST_BUFFER_OFFSET ( METAL_VERTEX_STREAM_BUFFER_OFFSET + METAL_VERTEX_STREAM_BUFFER_COUNT )
#define METAL_CONST_BUFFER_COUNT 20
// #define METAL_SRV_BUFFER_OFFSET (METAL_CONST_BUFFER_OFFSET + METAL_CONST_BUFFER_COUNT)
#define METAL_SRV_BUFFER_OFFSET 0
#define METAL_SRV_BUFFER_COUNT 31
#define METAL_UAV_BUFFER_OFFSET 0
#define METAL_UAV_BUFFER_COUNT 31

#define METAL_SRV_TEXTURE_OFFSET 0
#define METAL_SRV_TEXTURE_COUNT 31
#define METAL_UAV_TEXTURE_OFFSET 0
#define METAL_UAV_TEXTURE_COUNT 31

// 8 (depth compare functions) * 2 (depth on / off) + 1 (temporary state)
#define METAL_DEPTHSTENCIL_CACHE_SIZE 17

enum MetalEncoderType
{
	MTLENCODERTYPE_NONE,
	MTLENCODERTYPE_RENDER,
	MTLENCODERTYPE_COMPUTE,
	MTLENCODERTYPE_BLIT,
	MTLENCODERTYPE_ACCELERATION_STRUCTURE
};

enum MetalWorkQueueType
{
	METALWORKQUEUE_INVALID = -1,
	METALWORKQUEUE_RENDER = 0,

	METALWORKQUEUE_MAX = 1, // This should always be last
};

enum MetalCBCommitFlags
{
	MTLCBCOMMIT_NOFLAGS = 0x0,
	MTLCBCOMMIT_WAITUNTILSCHEDULED = 0x1,
	MTLCBCOMMIT_WAITUNTILCOMPLETE = 0x2,
	MTLCBCOMMIT_PRESENTDRAWABLE = 0x4,
};

enum MetalRenderEncoderDirtyState : uint32_t
{
	METAL_RENDERENCODERDIRTYSTATE_NONE = 0u,
	METAL_RENDERENCODERDIRTYSTATE_CULLMODE = 1u << 0,
	METAL_RENDERENCODERDIRTYSTATE_VIEWPORT = 1u << 1,
	METAL_RENDERENCODERDIRTYSTATE_BLENDCOLOR = 1u << 2,
	METAL_RENDERENCODERDIRTYSTATE_DEPTHBIAS = 1u << 3,
	METAL_RENDERENCODERDIRTYSTATE_DEPTHSTENCILSTATE = 1u << 4,
	METAL_RENDERENCODERDIRTYSTATE_VERTEXDESCRIPTOR = 1u << 5,
	METAL_RENDERENCODERDIRTYSTATE_ATTACHMENTS = 1u << 6,
	METAL_RENDERENCODERDIRTYSTATE_FILLMODE = 1u << 7,
	METAL_RENDERENCODERDIRTYSTATE_DEPTHCLIP = 1u << 8,

	METAL_RENDERENCODERDIRTYSTATE_ALL = 0b0111111111u
};

enum MetalBlendType
{
	METAL_BLENDING_DISABLED,
	METAL_BLENDING_ENABLED,
	METAL_BLENDING_ENABLED_SEPARATE_ALPHA
};

struct MetalDepthBias
{
	float depthBias;
	float slopeScale;
	float clamp;
};

struct MetalBlendState
{
	MetalBlendType blendType;
	bool alphaCoverageEnable;
	MTLBlendOperation rgbBlendOp;
	MTLBlendOperation alphaBlendOp;
	MTLBlendFactor srcColorFactor;
	MTLBlendFactor destColorFactor;
	MTLBlendFactor srcAlphaFactor;
	MTLBlendFactor destAlphaFactor;
	MetalColor blendColor;
	MTLColorWriteMask writeMask;
	size_t hashValue;
};

struct MetalClearState
{
	MTLLoadAction colorLoadAction[METAL_MAX_RENDER_TARGETS];
	MTLStoreAction colorStoreAction[METAL_MAX_RENDER_TARGETS];
	MTLClearColor clearColorValue[METAL_MAX_RENDER_TARGETS];

	MTLLoadAction depthLoadAction;
	MTLStoreAction depthStoreAction;
	float clearDepthValue;

	MTLLoadAction stencilLoadAction;
	MTLStoreAction stencilStoreAction;
	uint32_t clearStencilValue;
};

struct ShaderResourceMask
{
	uint32_t constantBufferMask;
	uint32_t bufferMask;
	uint32_t textureMask;
	uint32_t samplerMask;
	uint8_t textureTypes[METAL_MAX_BOUND_TEXTURES];

	void ResetMasks()
	{
		constantBufferMask = 0;
		bufferMask = 0;
		textureMask = 0;
		samplerMask = 0;
	}
};

struct MetalRenderPassHint
{
	struct DepthAttachment
	{
		MTLLoadAction load;
		MTLStoreAction store;
		float clearValue;
	} depth;

	struct ColorAttachment
	{
		MTLLoadAction load;
		MTLStoreAction store;
		MTLClearColor clearColor;
	} color[METAL_MAX_RENDER_TARGETS];
};

class MetalWorkQueue
{
public:
	MetalWorkQueue();
	~MetalWorkQueue();
	void SetMetalContext( MetalContext* metalContext );
	void SetCommandQueue( id<MTLCommandQueue> commandQueue );

	void CommitCommandBuffer( MetalCBCommitFlags flags );
	bool BlitToDrawableAndPresent( id<MTLTexture> srcTexture, NSView* view, uint64_t* renderedFrameNumber );
	void BeginFrame();
	void EndFrame();
	void EndCurrentRenderPass();
	void RenderTargetBarrier();

	id<MTLBlitCommandEncoder> GetBlitEncoder( NSString* encoderLabel = nil );
	id<MTLComputeCommandEncoder> GetComputeEncoder( NSString* encoderLabel = nil );
	id<MTLRenderCommandEncoder> GetRenderEncoder( NSString* encoderLabel = nil );
	id<MTLParallelRenderCommandEncoder> GetParallelEncoder( NSString* encoderLabel = nil );
	API_AVAILABLE( macos( 11.0 ) )
	id<MTLAccelerationStructureCommandEncoder> GetAccelerationStructureEncoder( NSString* encoderLabel = nil );

	void EndEncoder();
	void ReleaseEncoder( bool endEncoding );

	void CopyDataToBuffer( id<MTLBuffer> buffer, const void* data, size_t offset, size_t sizeInBytes );
	void CopyBufferToBuffer( id<MTLBuffer> dest, size_t destOffset, id<MTLBuffer> src, size_t srcOffset, size_t size );
	void ReadBackBufferToCPU( id<MTLBuffer> buffer, bool waitForData );

	void CopyTextureToMTLBuffer( id<MTLTexture> texture,
								 id<MTLBuffer> destBuffer,
								 uint32_t bytesPerRow,
								 uint32_t bytesPerSlice,
								 MTLOrigin origin,
								 MTLSize size,
								 uint32_t mipLevel,
								 bool waitForData );

	void CopyTextureToTexture( id<MTLTexture> srcTexture,
							   uint32_t srcSlice,
							   uint32_t srcMip,
							   MTLOrigin srcOrigin,
							   MTLSize srcSize,
							   id<MTLTexture> dstTexture,
							   uint32_t dstSlice,
							   uint32_t dstMip,
							   MTLOrigin dstOrigin );

	void UploadTexture( id<MTLTexture> texture,
						id<MTLBuffer> srcBuffer,
						uint32_t slice,
						uint32_t mip,
						uint32_t bytesPerRow,
						uint32_t bytesPerSlice,
						uint32_t srcOffset,
						bool syncOnBuffer );
	void UploadTexture( id<MTLTexture> texture,
						const void* srcData,
						uint32_t slice,
						uint32_t mip,
						uint32_t bytesPerRow,
						uint32_t bytesPerSlice,
						uint32_t srcOffset,
						bool syncOnBuffer );
	void UploadTexture( id<MTLTexture> texture,
						id<MTLBuffer> srcBuffer,
						uint32_t slice,
						uint32_t mip,
						const MTLOrigin& origin,
						const MTLSize& size,
						uint32_t bytesPerRow,
						uint32_t bytesPerSlice,
						uint32_t srcOffset,
						bool syncOnBuffer );
	void GenerateMipMaps( id<MTLTexture> texture );

	void ClearBuffer( id<MTLBuffer> buffer, const float values[4] );
	void ClearBuffer( id<MTLBuffer> buffer, const uint32_t values[4] );
	void ClearTexture( id<MTLTexture> texture, uint32_t mipLevel, const float values[4] );
	void ClearTexture( id<MTLTexture> texture, uint32_t mipLevel, const uint32_t values[4] );

	void ResolveMsaa( id<MTLTexture> source, id<MTLTexture> destination );

	void SetVertexDescriptor( MTLVertexDescriptor& vertexDescriptor );
	void SetShaders( id<MTLFunction> vertexFunction,
					 id<MTLFunction> fragmentFunction,
					 id<MTLFunction> computeFunction,
					 MTLSize threadGroupSize,
					 const ShaderResourceMask* resourceMasks );

	void SetViewport( float originX, float originY, float width, float height, float znear, float zfar );
	void SetCullMode( MTLCullMode cullMode );
	void SetFillMode( MTLTriangleFillMode fillMode );
	void SetDepthBias( float* depthBias, float* slopeScale, float* clamp );
	void SetDepthClipEnable( bool enable );
	void SetBlendColor( uint32_t attachmentIndex, uint32_t blendColor );
	void SetSrcBlend( uint32_t attachmentIndex, MTLBlendFactor factor );
	void SetDestBlend( uint32_t attachmentIndex, MTLBlendFactor factor );
	void SetSrcBlendAlpha( uint32_t attachmentIndex, MTLBlendFactor factor );
	void SetDestBlendAlpha( uint32_t attachmentIndex, MTLBlendFactor factor );
	void SetBlendOp( uint32_t attachmentIndex, MTLBlendOperation op );
	void SetBlendOpAlpha( uint32_t attachmentIndex, MTLBlendOperation op );
	void SetBlendType( uint32_t attachmentIndex, MetalBlendType blendType );
	void SetColorWriteMask( uint32_t attachmentIndex, MTLColorWriteMask writeMask );
	void SetDepthState( bool enabled );
	void SetDepthCompareFn( MTLCompareFunction fn );
	void SetStencilState( bool enabled );
	void SetStencilDepthFailOp( MTLStencilOperation op );
	void SetStencilFailOp( MTLStencilOperation op );
	void SetStencilPassOp( MTLStencilOperation op );
	void SetStencilCompareFn( MTLCompareFunction fn );
	void SetStencilRefValue( uint32_t value );
	void SetStencilMask( uint32_t mask );

	void SetRenderAttachments( id<MTLTexture> texture, uint32_t index, uint32_t slice = 0 );
	void SetDepthStencilAttachment( id<MTLTexture> texture );

	void RenderPassHint( const MetalRenderPassHint& hint );
	void EndRenderPassHint();

	void SetConstants( Tr2RenderContextEnum::ShaderType shaderType,
					   const void* constantBuffer,
					   uint32_t size,
					   ConstantBufferToken& token,
					   uint32_t constantIndex );
	void UploadConstants( const void* constantBuffer, uint32_t size, ConstantBufferToken& token );

	void SetBuffers( Tr2RenderContextEnum::ShaderType shaderType,
					 const id<MTLBuffer>* buffers,
					 uint32_t buffersMask,
					 id<MTLBuffer> heapView,
					 uint32_t heapViewMask );
	void SetTextures( Tr2RenderContextEnum::ShaderType shaderType,
					  const id<MTLTexture>* textures,
					  NSRange texturesRange );
	void SetSamplers( Tr2RenderContextEnum::ShaderType shaderType,
					  const id<MTLSamplerState>* samplers,
					  NSRange samplersRange );
	void ResetBuffers( Tr2RenderContextEnum::ShaderType shaderType );
	void ResetTextures( Tr2RenderContextEnum::ShaderType shaderType );
	void ResetSamplers( Tr2RenderContextEnum::ShaderType shaderType );

	void SetVertexStream( uint32 stream, id<MTLBuffer> buffer, uint32 stride, uint32 offset );
	void SetCurrentVertexDescriptor( MTLVertexDescriptor* vertexDescriptor, uint8_t vertexStreamMask, size_t baseHash );

	void ClearAttachment( MTLClearColor* clearColor,
						  float* clearDepth,
						  uint32_t* clearStencil,
						  uint32_t attachmentIndex );

	void DrawPrimitives( MTLPrimitiveType primitiveType,
						 uint32_t numVertices,
						 uint32_t startVertex,
						 uint32_t numInstances,
						 uint32_t startInstance = 0 );
	void DrawPrimitives( MTLPrimitiveType primitiveType, id<MTLBuffer> indirectBuffer, uint32_t indirectBufferOffset );

	void DrawIndexedPrimitives( MTLPrimitiveType primitiveType,
								uint32_t numIndices,
								MTLIndexType indexType,
								id<MTLBuffer> indexBuffer,
								uint32_t startIndex,
								uint32_t numInstances,
								int32_t baseVertex = 0,
								uint32_t baseInstance = 0 );
	void DrawIndexedPrimitives( MTLPrimitiveType primitiveType,
								MTLIndexType indexType,
								id<MTLBuffer> indexBuffer,
								uint32_t indexBufferOffset,
								id<MTLBuffer> indirectBuffer,
								uint32_t indirectBufferOffset );

	void Dispatch( uint32_t groupDimX, uint32_t groupDimY, uint32_t groupDimZ );
	void Dispatch( id<MTLBuffer> indirectBuffer, uint32_t indirectBufferOffset );
	API_AVAILABLE( macos( 11.0 ) )
	void DispatchRays( Tr2RtPipelineStateAL* pipeline,
					   Tr2RtShaderTableAL* shaderTable,
					   uint32_t rayGenIndex,
					   uint32_t width,
					   uint32_t height,
					   uint32_t depth );

	void CreateVisibilityQueryBuffer( uint64_t maxNumQueries );
	uint64_t StartVisibilityQuery();
	void EndVisibilityQuery( uint64_t queryNumber );
	bool GetVisibilityQueryPixelCount( uint64_t queryNumber, uint64_t* pixelCount, bool finishOutstandingWork );

	enum CounterType
	{
		COUNTER_TIMER,
		COUNTER_PIPELINE_STATS,
	};
	bool SampleCounter( id buffer, NSUInteger index, CounterType counter );

	void PushDebugGroup( const char* name );
	void PopDebugGroup();

	void BeginParallelEncoding( MetalWorkQueue* primaryQueue );
	void EndParallelEncoding();

	bool CanBeginParallelEncoding() const;

	void PipelineQueryStarted( Tr2PipelineStatsQueryAL* query );
	void PipelineQueryEnded( Tr2PipelineStatsQueryAL* query );

	MTLVertexDescriptor* GetCachedVertexDescriptor( Tr2VertexLayoutAL* layout,
													size_t inputHash,
													uint8_t& streamMask,
													bool& needsDummyStream ) const;
	void CacheVertexDescriptor( Tr2VertexLayoutAL* layout,
								size_t inputHash,
								MTLVertexDescriptor* descriptor,
								uint8_t streamMask,
								bool needsDummyStream );
	void FlushCachedVertexDescriptors();

	id<MTLCommandBuffer> GetCommandBuffer()
	{
		return m_commandBuffer;
	}

	bool EmitRenderEncoderState();

	uint64_t GetCurrentEncoderIndex() const;
	void MarkConstantBuffersDirty();
	bool IsSilicon() const;

private:
	void CreateClearFunctions();
	void ResetWorkQueue();
	void ResetFrame();
	void ResetClearState();
	void InitRenderPassDescriptor();
	void CreateCommandBuffer();
	void FlushOutstandingOperations();
	void SetCurrentEncoder( MetalEncoderType encoderType, NSString* encoderLabel = nil );
	void SetupVertexDescriptors();
	bool EmitRenderPipelineState();
	bool EmitComputePipelineState();
	bool EmitComputeEncoderState();
	void SetVertexBufferBindings();
	void SetFragmentBufferBindings();
	void SetComputeBufferBindings();
	void SetupPresentBlitResources();
	ConstantBufferAllocator::Entry UploadArgumentBuffer( const Tr2ShaderSignatureAL& signature,
														 std::vector<id<MTLResource>>& readResources,
														 std::vector<id<MTLResource>>& writeResources ) const;

	size_t CalculateVertexDescriptorHash();
	size_t CalculateAttachmentsHash();

	void SetRenderTargetSizedScissorRect();

	id<MTLDepthStencilState> GetCachedDepthStencilState( MTLDepthStencilDescriptor* descriptor );
	void ApplyRenderPassHint();
	void SetRenderStatesDirty();

	MetalContext* m_context;
	bool m_isPrimary;

	id<MTLCommandQueue> m_commandQueue;
	id<MTLDevice> m_device;
	id<MTLCommandBuffer> m_commandBuffer;

	MetalEncoderType m_currentEncoderType;
	id<MTLBlitCommandEncoder> m_currentBlitEncoder;
	id<MTLRenderCommandEncoder> m_currentRenderEncoder;
	id<MTLParallelRenderCommandEncoder> m_currentParallelEncoder;
	id<MTLComputeCommandEncoder> m_currentComputeEncoder;
	API_AVAILABLE( macos( 11.0 ) )
	id<MTLAccelerationStructureCommandEncoder> m_currentAccelerationStructureEncoder;
	MTLRenderPipelineDescriptor* m_presentBlitPipelineDesciptor;
	id<MTLRenderPipelineState> m_presentBlitPipeline;
	MTLRenderPassDescriptor* m_drawRenderPassDescriptor;
	MTLRenderPassDescriptor* m_blitAndPresentRenderPassDescriptor;

	MTLRenderPassDescriptor* m_currentRenderPassDescriptor;

	id<MTLCaptureScope> m_captureScopeFullFrame;

	uint64_t m_encoderIndex;

	uint32_t m_numRenderAttachments;
	uint32_t m_dirtyRenderEncoderState;
	uint32_t m_numCommands;
	bool m_encoderInUse;
	bool m_encoderEnded;
	bool m_encoderHasWork;
	bool m_generatesEndOfQueueEvent;
	bool m_isIntelGpu;
	bool m_isAppleSilicon;

	MTLViewport m_viewport;
	MTLScissorRect m_scissorRect;
	bool m_validViewport;
	MTLCullMode m_cullMode;
	MTLTriangleFillMode m_fillMode;
	MetalDepthBias m_depthBias;
	MetalBlendState m_blendState[METAL_MAX_RENDER_TARGETS];
	MTLDepthClipMode m_depthClipMode;
	MTLDepthStencilDescriptor* m_depthStencilDescriptor;
	MTLStencilDescriptor* m_frontFaceStencilDescriptor;
	MTLStencilDescriptor* m_backFaceStencilDescriptor;
	uint32_t m_stencilReferenceValue;

	// These should come from the attachment direectly
	MTLPixelFormat m_outputPixelFormat;
	MTLPixelFormat m_outputDepthFormat;

	MTLRenderPipelineColorAttachmentDescriptor* m_renderPipelineColorAttachmentDescriptor;
	MTLRenderPipelineDescriptor* m_renderPipelineDescriptor;

	size_t m_currentVertexDescriptorHash;
	size_t m_currentAttachmentsHash;
	size_t m_currentRenderPipelineDescriptorHash;
	size_t m_currentComputePipelineDescriptorHash;
	size_t m_currentDepthStencilDescriptorHash;

	id<MTLRenderPipelineState> m_currentRenderPipelineState;
	id<MTLComputePipelineState> m_currentComputePipelineState;
	id<MTLDepthStencilState> m_currentDepthStencilState;

	id<MTLDepthStencilState> m_depthStencilStateCache[METAL_DEPTHSTENCIL_CACHE_SIZE];

	id<MTLFunction> m_vertexFunction;
	id<MTLFunction> m_fragmentFunction;
	id<MTLFunction> m_computeFunction;
	id<MTLFunction> m_clearBufferComputeFunctions[2];
	id<MTLFunction> m_clearTextureComputeFunctions[2];
	MTLSize m_threadGroupSize;
	const ShaderResourceMask* m_shaderResourceMasks;

	id<MTLDrawable> m_drawable;

	struct MetalBuffer
	{
		id<MTLBuffer> buffer;
		NSUInteger offset;
	};

	struct ConstantBuffer
	{
		uint32_t page;
		uint32_t offset;
	};

	ConstantBuffer m_constBuffers[Tr2RenderContextEnum::SHADER_TYPE_COUNT][METAL_MAX_BOUND_BUFFERS];
	MetalBuffer m_buffers[Tr2RenderContextEnum::SHADER_TYPE_COUNT][METAL_MAX_BOUND_BUFFERS];
	id<MTLTexture> m_textures[Tr2RenderContextEnum::SHADER_TYPE_COUNT][METAL_MAX_BOUND_TEXTURES];
	id<MTLSamplerState> m_samplers[Tr2RenderContextEnum::SHADER_TYPE_COUNT][METAL_MAX_BOUND_SAMPLERS];

	uint32_t m_activeConstBuffersMask[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
	uint32_t m_activeBuffersMask[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
	uint32_t m_dirtyConstBuffersMask[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
	uint32_t m_dirtyConstBufferPageMask[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
	uint32_t m_dirtyBuffersMask[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
	uint32_t m_dirtyTexturesMask[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
	uint32_t m_dirtySamplersMask[Tr2RenderContextEnum::SHADER_TYPE_COUNT];

	struct VertexStream
	{
		uint32_t stride;
	};
	VertexStream m_boundVertexStreams[METAL_VERTEX_STREAM_BUFFER_COUNT];
	MTLVertexDescriptor* m_currentVertexDescriptor;
	uint8_t m_currentVertexStreamMask;
	size_t m_currentVertexDescriptorBaseHash;


	MetalRenderPassHint m_pendingRenderPassHint;
	bool m_hasPendingRenderPassHint;
	bool m_hasPendingRenderTargetBarrier;

	bool m_pendingClear;
	bool m_visibilityQueryInProgress;
	MetalClearState m_clearState;
	id<MTLBuffer> m_visibilityBuffer;
	uint64_t m_maxVisibilityQueries;
	uint64_t m_currentVisibilityQueryIndex;
	uint64_t m_numCompletedVisibilityQueries;
	uint64_t m_visibilityQueriesInThisEncoder;

	std::vector<Tr2PipelineStatsQueryAL*> m_pipelineQueriesInProgress;

	struct CachedVertexLayout
	{
		Tr2VertexLayoutAL* layout;
		size_t inputHash;
		MTLVertexDescriptor* descriptor;
		uint8_t streamMask;
		bool needsDummyStream;
	};
	std::vector<CachedVertexLayout> m_cachedVertexLayouts;

	dispatch_semaphore_t m_frameSemaphore;
};

} // namespace TrinityALImpl

#endif
