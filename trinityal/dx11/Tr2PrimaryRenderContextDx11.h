// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2PrimaryRenderContextDx11_h_
#define Tr2PrimaryRenderContextDx11_h_


#include "../Tr2MemoryCounterAL.h"
#include "../include/Tr2RenderContextAL.h"
#include "../include/Tr2CapsAL.h"
#include "../include/Tr2SamplerStateAL.h"
#include "../include/Tr2TextureAL.h"
#include "../include/Tr2GpuTimerAL.h"
#include "../include/Tr2BufferAL.h"
#include "../include/Tr2FutureAL.h"
#include "upscaling/Tr2UpscalingAL.h"


struct Tr2PresentParametersAL;

namespace TrinityALImpl
{
class Tr2AsyncCreatorAL;
}


#if ( TRINITY_PLATFORM == TRINITY_DIRECTX11 )

class Tr2PrimaryRenderContextAL : public Tr2RenderContextAL
{
public:
	Tr2PrimaryRenderContextAL();
	~Tr2PrimaryRenderContextAL();
	void Destroy();

	ALResult CreateDevice(
		uint32_t Adapter,
		Tr2WindowHandle hFocusWindow,
		const Tr2PresentParametersAL& pPresentationParameters );

	ALResult SetPresentParameters( unsigned adapter, const Tr2PresentParametersAL& pPresentationParameters );

	const Tr2CapsAL& GetCaps() const;

	ALResult Present();

	bool IsValid();

	Tr2RenderContextEnum::PixelFormat GetBackBufferFormat() const;

	bool SupportsBindlessTextures() const;

	uint64_t GetRecordingFrameNumber() const;
	uint64_t GetRenderedFrameNumber() const;

	Tr2UpscalingAL::Result EnableUpscaling( Tr2UpscalingAL::Technique tech, Tr2UpscalingAL::Setting setting, bool framegeneration, uint32_t adapter );
	Tr2UpscalingContextAL* GetUpscalingContext( uint32_t upscalingContextID ) const;
	Tr2UpscalingContextAL* CreateUpscalingContext( Tr2UpscalingAL::UpscalingContextParams params, uint32_t existingContext = Tr2UpscalingAL::INVALID_CONTEXT_ID );
	void DeleteUpscalingContext( uint32_t contextID );
	std::vector<std::tuple<Tr2UpscalingAL::Technique, uint32_t, bool>> GetSupportedUpscalingTechniques( uint32_t adapter );
	void GetUpscalingSetup( Tr2UpscalingAL::Technique& technique, Tr2UpscalingAL::Setting& setting, bool& framegeneration, bool& temporal ) const;
	Tr2UpscalingAL::UpscalingInfo GetUpscalingInfo( uint32_t upscalingContextID ) const;

	void MarkFrameEvent( Tr2RenderContextEnum::FrameEvent frameEvent );

	// Non-blocking versions of Tr2BufferAL::Create and Tr2TextureAL::Create; see Tr2FutureAL for how
	// to get the created object. Thread-safe. Initial data must stay alive until the future completes.
	Tr2FutureAL<Tr2BufferAL> CreateBufferAsync(
		const Tr2BufferDescriptionAL& desc,
		const void* initialData );
	Tr2FutureAL<Tr2BufferAL> CreateBufferAsync(
		Tr2RenderContextEnum::PixelFormat format,
		uint32_t count,
		Tr2GpuUsage::Type gpuUsage,
		Tr2CpuUsage::Type cpuUsage,
		const void* initialData );
	Tr2FutureAL<Tr2BufferAL> CreateBufferAsync(
		uint32_t stride,
		uint32_t count,
		Tr2GpuUsage::Type gpuUsage,
		Tr2CpuUsage::Type cpuUsage,
		const void* initialData );

	Tr2FutureAL<Tr2TextureAL> CreateTextureAsync( const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage );
	Tr2FutureAL<Tr2TextureAL> CreateTextureAsync( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage );
	Tr2FutureAL<Tr2TextureAL> CreateTextureAsync( const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage, Tr2SubresourceData* initialData );
	Tr2FutureAL<Tr2TextureAL> CreateTextureAsync( const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage );
	Tr2FutureAL<Tr2TextureAL> CreateTextureAsync( const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData );
	Tr2FutureAL<Tr2TextureAL> CreateTextureAsync( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData );

	// Returns true if called on the thread that owns the immediate context
	bool IsMainThreadDx11() const;

public:
	bool m_usingEXDevice;

	CComPtr<ID3D11Device> m_d3dDevice11;

	CComPtr<IDXGISwapChain> m_swapChain;
	CComPtr<IDXGIFactory> m_dxgiFactory;
	CComPtr<IDXGIOutput> m_dxgiOutput;
	bool m_softwareAdapter;

	Tr2TextureAL m_defaultBackBuffer;

private:
	ALResult CreateBackBuffers( const Tr2PresentParametersAL& presentationParameters );

	void StartAsyncCreation();
	void StopAsyncCreation();
	void DispatchAsyncCallbacks();
	std::shared_ptr<TrinityALImpl::Tr2AsyncCreatorAL> GetAsyncCreator();

	std::shared_ptr<TrinityALImpl::Tr2AsyncDispatcherAL> m_asyncDispatcher = std::make_shared<TrinityALImpl::Tr2AsyncDispatcherAL>();
	std::shared_ptr<TrinityALImpl::Tr2AsyncCreatorAL> m_asyncCreator;
	std::mutex m_asyncCreatorMutex;

	uint64_t m_recodingFrame;
	uint64_t m_renderedFrame;

	uint32_t m_vsyncInterval;

	// Device statistics
	//CComPtr<ID3D11Query> m_deviceStatistics;
	//bool m_deviceStatisticsQueryEmpty;

	Tr2BufferAL m_zeroVertexBuffer;

	// Device statistics
	CComPtr<ID3D11Query> m_deviceStatistics;

	struct FrameFence
	{
		uint64_t recordedFrame;
		CComPtr<ID3D11Query> query;
	};
	std::vector<FrameFence> m_frameFences;
	bool m_deviceStatisticsQueryEmpty;
	Tr2GpuTimerAL m_frameTimer;

	uint32_t m_adapterVendorId;

	Tr2CapsAL m_caps;

	Tr2MemoryCounterAL m_memory;
	TrinityALImpl::Tr2UpscalingTechniqueDx11* m_upscalingTechnique;

public:
	TrinityALImpl::Tr2SamplerStateALFactory m_samplerStateFactory;

private:
	Tr2PrimaryRenderContextAL( const Tr2PrimaryRenderContextAL& ) /* = delete */;
	Tr2PrimaryRenderContextAL& operator=( const Tr2PrimaryRenderContextAL& ) /* = delete */;
};

#endif // ( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#endif //Tr2PrimaryRenderContextDx11_h_
