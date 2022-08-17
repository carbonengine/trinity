//
//  Copyright © 2020 CCP. All rights reserved.
//
#pragma once

#if TRINITY_PLATFORM == TRINITY_METAL
#include "MetalWorkQueue.h"
#include "MetalUtils.h"

// Macros to cut down on code for passing through of work queue functions

namespace TrinityALImpl
{

#define METAL_NUM_DUMMY_TEXTURES 6

/*
  A Metal context is implicitly per-device. As such resources exist at the top level as they can be shared
 across work-queues (with appropriate syncronsiation if required). It's useful to collect this state together
 into a seperate context instead of putting it into the render context as it'll make any multi-GPU support easier
 (just duplicate the context per-device).
 
  In order to be able to include this file the XCode project has been modified to add the '-x objective-c++'
 compiler flag to force all C++ files to be compiled as Objective C++ (since this file includes objective C headers).
 If this is ultimately not palatable then we could bind this opaquely inside the RenderContext and then re-cast it
 appropriately inside any of the Tr2<thing>ALMetal.mm code.
 */
class MetalContext
{
public:
	MetalContext();
	~MetalContext();

	MetalUtils *m_utils;

	id<MTLBuffer> CreateMetalBuffer( MetalWorkQueue* workQueue, size_t sizeInBytes, MTLResourceOptions options, const void *data );
	void DestroyMetalBuffer(id<MTLBuffer> buffer);
	void IndicateBufferModified(id<MTLBuffer> buffer, size_t offset, size_t length);
	
	void DestroyConstantBuffer( void* buffer );

	id<MTLDevice>        GetDevice();

	MTLSamplerDescriptor* CreateSamplerDescriptor();
	void DestroySamplerDescriptor(MTLSamplerDescriptor* samplerDescriptor);
	id<MTLSamplerState> CreateSamplerState(MTLSamplerDescriptor* samplerDescriptor);
	void DestroySamplerState(id<MTLSamplerState> samplerState);

	id<MTLTexture> CreateMetalTexture(MTLTextureType metalTextureType,
			MTLPixelFormat  pixelFormat,
			size_t          width,
			size_t          height,
			size_t          depth,
			uint32          mipMapCount,
			MTLStorageMode  storageMode,
			MTLTextureUsage textureUsage,
			uint32          sampleCount = 1,
			uint32          arrayLength = 1);

	id<MTLTexture> CreateSRGBViewOfMetalTexture( id<MTLTexture> texture );
	id<MTLTexture> CreateUAVOfMetalTexture( id<MTLTexture> texture, uint32_t mipLevel );

	void DestroyMetalTexture( id<MTLTexture> texture );

	MTLVertexDescriptor* CreateVertexLayout();
	void DestroyVertexLayout( MTLVertexDescriptor* vertexDescriptor );

	id<MTLTexture> GetDummyTexture( MTLTextureType textureType );
	id<MTLSamplerState> GetDummySampler();
	// Optional fields will contain info about the buffer
	id<MTLBuffer> GetDummyBuffer( NSUInteger *outSize = nil, MTLVertexFormat *outFormat = nil );

	bool IsResourceInUse( uint64 resourceLastAccessedFrame ) const;
	uint64_t GetRecordingFrameNumber() const;
	uint64_t GetRenderedFrameNumber() const;

	void BlitToDrawableAndPresent( id<MTLTexture> srcTexture, NSView* view );
	
	double GetGpuTimerRate() const;

	id<MTLRenderPipelineState> GetCachedRenderPipelineState( size_t renderPipelineDescriptorHash );
	void AddRenderPipelineStateToCache( size_t renderPipelineDescriptorHash, id<MTLRenderPipelineState> pipelineState );
	id<MTLComputePipelineState> GetCachedComputePipelineState( size_t computePipelineDescriptorHash );
	void AddComputePipelineStateToCache( size_t computePipelineDescriptorHash, id<MTLComputePipelineState> pipelineState );

	uint32_t BeginParallelEncoding(uint32_t requestedEncodersCount);
	void EndParallelEncoding();
	bool IsInParallelEncoding() const;

	MetalWorkQueue* GetSecondaryWorkQueue( uint32_t index );
	MetalWorkQueue* GetPrimaryWorkQueue();
private:
	id<MTLDevice>       m_device;
	id<MTLCommandQueue> m_commandQueue;
	TrinityALImpl::MetalWorkQueue m_primaryWorkQueue;
	std::vector<TrinityALImpl::MetalWorkQueue> m_secondaryWorkQueues;
	uint32_t m_parallelEncodersCount;
	uint64_t m_recordingFrameNumber;
	uint64_t m_renderedFrameNumber;
	double m_gpuTimerRate;
#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 110000
	typedef MTLTimestamp DeviceTimestamp;
#else
	typedef NSUInteger DeviceTimestamp;
#endif
	DeviceTimestamp m_beginCpuTime;
	DeviceTimestamp m_beginGpuTime;
	bool m_gpuTimerRateMeasured;

	id<MTLTexture> m_dummyTexture[METAL_NUM_DUMMY_TEXTURES];
	id<MTLSamplerState> m_dummySampler;
	id<MTLBuffer> m_dummyBuffer;
	

	std::unordered_map<size_t, id<MTLRenderPipelineState>>  m_renderPipelineStateMap;
	std::unordered_map<size_t, id<MTLRenderPipelineState>>  m_newRenderPipelineStateMap;
	std::unordered_map<size_t, id<MTLComputePipelineState>> m_computePipelineStateMap;
	std::mutex m_pipelineCacheMutex;

	std::vector<void*> m_destroyedConstantBuffers;

	void GenerateDummyTexture();
	void GenerateDummyBuffer();
};

} // namespace TrinityALImpl

#endif
