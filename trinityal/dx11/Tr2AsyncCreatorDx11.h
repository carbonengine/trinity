// Copyright © 2026 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX11

#include "../Tr2AsyncCreationAL.h"
#include "../include/Tr2BufferAL.h"
#include "../include/Tr2TextureAL.h"

// Set to false to make asynchronous creation behave as if the driver didn't support concurrent
// resource creation. Takes effect when the render context device is created.
extern bool g_asyncCreateAllowConcurrentCreatesDx11;

namespace TrinityALImpl
{
class Tr2BufferAL;
class Tr2TextureAL;

// --------------------------------------------------------------------------------------
// Description:
//   Backend for Tr2PrimaryRenderContextAL::CreateBufferAsync/CreateTextureAsync. Resources
//   are created synchronously on the calling thread if the driver supports concurrent
//   resource creation or if the calling thread is the main thread. Otherwise creation
//   requests are queued and executed on the main thread.
// --------------------------------------------------------------------------------------
class Tr2AsyncCreatorAL
{
public:
	typedef std::function<::Tr2BufferAL( std::shared_ptr<Tr2BufferAL> )> WrapBufferFunction;
	typedef std::function<::Tr2TextureAL( std::shared_ptr<Tr2TextureAL> )> WrapTextureFunction;

	Tr2AsyncCreatorAL( Tr2PrimaryRenderContextAL& renderContext, std::shared_ptr<Tr2AsyncDispatcherAL> dispatcher );
	~Tr2AsyncCreatorAL();

	// Main thread: checks driver support for concurrent resource creation
	ALResult Initialize();

	// Main thread: waits for creations in progress and cancels queued ones with E_ABORT
	void Shutdown();

	// Main thread: executes queued creation requests
	void ProcessCompleted();

	bool SupportsConcurrentCreates() const;

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

private:
	struct Empty
	{
	};

	void Run( std::shared_ptr<Tr2AsyncContextJobAL<Empty>> job );

	Tr2PrimaryRenderContextAL& m_renderContext;
	std::shared_ptr<Tr2AsyncDispatcherAL> m_dispatcher;
	bool m_concurrentCreates;
	Tr2AsyncShutdownGuardAL m_guard;
	std::mutex m_queueMutex;
	std::vector<std::shared_ptr<Tr2AsyncContextJobAL<Empty>>> m_queue;
};
}

#endif
