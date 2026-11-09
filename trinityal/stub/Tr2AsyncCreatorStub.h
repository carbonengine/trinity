// Copyright © 2026 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_STUB

#include "../Tr2AsyncCreationAL.h"
#include "../include/Tr2BufferAL.h"
#include "../include/Tr2TextureAL.h"

namespace TrinityALImpl
{
class Tr2BufferAL;
class Tr2TextureAL;

// --------------------------------------------------------------------------------------
// Description:
//   Backend for Tr2RenderContextAL::CreateBufferAsync/CreateTextureAsync: creates
//   resources synchronously on the calling thread.
// --------------------------------------------------------------------------------------
class Tr2AsyncCreatorAL
{
public:
	typedef std::function<::Tr2BufferAL( std::shared_ptr<Tr2BufferAL> )> WrapBufferFunction;
	typedef std::function<::Tr2TextureAL( std::shared_ptr<Tr2TextureAL> )> WrapTextureFunction;

	Tr2AsyncCreatorAL( Tr2PrimaryRenderContextAL& renderContext, std::shared_ptr<Tr2AsyncDispatcherAL> dispatcher );

	ALResult Initialize();
	void Shutdown();
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

private:
	template <typename Public, typename Impl>
	Tr2FutureAL<Public> Create( std::function<HRESULT( Impl& )> create, std::function<Public( std::shared_ptr<Impl> )> wrap );

	Tr2PrimaryRenderContextAL& m_renderContext;
	std::shared_ptr<Tr2AsyncDispatcherAL> m_dispatcher;
	Tr2AsyncShutdownGuardAL m_guard;
};
}

#endif
