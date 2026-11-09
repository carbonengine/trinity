// Copyright © 2026 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_METAL

#include "../Tr2AsyncCreationAL.h"
#include "../include/Tr2BufferAL.h"
#include "../include/Tr2TextureAL.h"
#include <Metal/Metal.h>

namespace TrinityALImpl
{
class Tr2BufferAL;
class Tr2TextureAL;

// --------------------------------------------------------------------------------------
// Description:
//   Where asynchronous resource creation records uploads of initial data. Uploads are
//   encoded on a worker thread into a command buffer of a separate command queue.
// --------------------------------------------------------------------------------------
class Tr2AsyncUploadMetal
{
public:
	Tr2AsyncUploadMetal( id<MTLDevice> device, id<MTLCommandBuffer> commandBuffer );
	~Tr2AsyncUploadMetal();

	id<MTLDevice> GetDevice() const;
	id<MTLBlitCommandEncoder> GetBlitEncoder();

	// Creates a shared buffer with a copy of data; the buffer is kept alive until the upload completes
	id<MTLBuffer> CreateStagingBuffer( const void* data, size_t size );

	void EndEncoding();
	void ReleaseStagingBuffers();

private:
	id<MTLDevice> m_device;
	id<MTLCommandBuffer> m_commandBuffer;
	id<MTLBlitCommandEncoder> m_blitEncoder;
	std::vector<id<MTLBuffer>> m_stagingBuffers;
};

// --------------------------------------------------------------------------------------
// Description:
//   Backend for Tr2RenderContextAL::CreateBufferAsync/CreateTextureAsync. A worker thread
//   creates resources and encodes uploads of initial data into a command buffer of its
//   own command queue. Once the upload completes, creation is finished on the main thread
//   (allocating resource heap indices) and the future is resolved.
// --------------------------------------------------------------------------------------
class Tr2AsyncCreatorAL : public Tr2AsyncWorkerAL
{
public:
	typedef std::function<::Tr2BufferAL( std::shared_ptr<Tr2BufferAL> )> WrapBufferFunction;
	typedef std::function<::Tr2TextureAL( std::shared_ptr<Tr2TextureAL> )> WrapTextureFunction;

	Tr2AsyncCreatorAL( Tr2PrimaryRenderContextAL& renderContext, std::shared_ptr<Tr2AsyncDispatcherAL> dispatcher );
	~Tr2AsyncCreatorAL();

	// Main thread: creates the upload queue and starts the worker thread
	ALResult Initialize();

	// Main thread: stops the worker thread and cancels unfinished creations with E_ABORT
	void Shutdown();

	// Main thread: finishes creations whose uploads have completed
	void ProcessCompleted();

	Tr2FutureAL<::Tr2BufferAL> CreateBuffer(
		const Tr2BufferDescriptionAL& desc,
		const void* initialData,
		WrapBufferFunction wrap );

	Tr2FutureAL<::Tr2TextureAL> CreateTexture(
		const Tr2BitmapDimensions& desc,
		const Tr2MsaaDesc& msaa,
		Tr2GpuUsage::Type gpuUsage,
		Tr2CpuUsage::Type cpuUsage,
		Tr2SubresourceData* initialData,
		WrapTextureFunction wrap );

protected:
	void ExecuteBatch( const std::vector<std::shared_ptr<Tr2AsyncJobAL>>& jobs ) override;

private:
	Tr2PrimaryRenderContextAL& m_renderContext;
	std::shared_ptr<Tr2AsyncDispatcherAL> m_dispatcher;
	id<MTLDevice> m_device;
	id<MTLCommandQueue> m_queue;
};
}

#endif
