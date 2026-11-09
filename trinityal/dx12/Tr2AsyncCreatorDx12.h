// Copyright © 2026 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../Tr2AsyncCreationAL.h"
#include "../include/Tr2BufferAL.h"
#include "../include/Tr2TextureAL.h"
#include "Tr2ResourceHelper.h"

// Set to false to make asynchronous creation upload initial data on the direct queue instead of
// a copy queue. Takes effect when the render context device is created.
extern bool g_asyncCreateUseCopyQueueDx12;

namespace TrinityALImpl
{
class Tr2BufferAL;
class Tr2TextureAL;

// --------------------------------------------------------------------------------------
// Description:
//   Backend for Tr2PrimaryRenderContextAL::CreateBufferAsync/CreateTextureAsync. A worker
//   thread creates resources and records uploads of initial data into its own command list
//   executing on a copy queue (or on the direct queue if a copy queue is not available).
//   Once the upload completes, creation is finished on the main thread (creating views and
//   recording state transitions on the main command list) and the future is resolved.
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

	bool UsesCopyQueue() const;

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
	HRESULT WaitForUpload( uint64_t fenceValue );

	Tr2PrimaryRenderContextAL& m_renderContext;
	std::shared_ptr<Tr2AsyncDispatcherAL> m_dispatcher;

	CComPtr<ID3D12CommandQueue> m_queue;
	bool m_isCopyQueue;
	CComPtr<ID3D12CommandAllocator> m_commandAllocator;
	CComPtr<ID3D12GraphicsCommandList> m_commandList;
	CComPtr<ID3D12Fence> m_fence;
	uint64_t m_fenceValue;
	HANDLE m_fenceEvent;
};
}

#endif
