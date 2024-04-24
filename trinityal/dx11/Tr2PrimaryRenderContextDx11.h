#pragma once
#ifndef Tr2PrimaryRenderContextDx11_h_
#define Tr2PrimaryRenderContextDx11_h_


#include "../Tr2MemoryCounterAL.h"
#include "../include/Tr2RenderContextAL.h"
#include "../include/Tr2CapsAL.h"
#include "../include/Tr2SamplerStateAL.h"
#include "../include/Tr2TextureAL.h"
#include "../include/Tr2GpuTimerAL.h"
#include "upscaling/Tr2UpscalingAL.h"


struct Tr2PresentParametersAL;


#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

class Tr2PrimaryRenderContextAL : public Tr2RenderContextAL
{
public:
	Tr2PrimaryRenderContextAL();
	~Tr2PrimaryRenderContextAL();
	void	Destroy();

	ALResult CreateDevice(	
		uint32_t Adapter, 
		Tr2WindowHandle  hFocusWindow, 
		const Tr2PresentParametersAL& pPresentationParameters );

	ALResult SetPresentParameters( unsigned adapter, const Tr2PresentParametersAL& pPresentationParameters );

	const Tr2CapsAL& GetCaps() const;
		
	ALResult Present();

	bool IsValid();

	Tr2RenderContextEnum::PixelFormat GetBackBufferFormat() const;

	Tr2UpscalingAL::Result EnableUpscaling( Tr2UpscalingAL::Technique tech, Tr2UpscalingAL::Setting setting, bool framegeneration, uint32_t adapter );
	Tr2UpscalingContextAL* GetUpscalingContext( uint32_t displayWidth, uint32_t displayHeight );
	Tr2UpscalingContextAL* CreateUpscalingContext( uint32_t displayWidth, uint32_t displayHeight, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat );
	std::vector<std::tuple<Tr2UpscalingAL::Technique, uint32_t, bool>> GetSupportedUpscalingTechniques( uint32_t adapter );
	void GetUpscalingSetup( Tr2UpscalingAL::Technique& technique, Tr2UpscalingAL::Setting& setting, bool& framegeneration );
	Tr2UpscalingAL::UpscalingInfo GetUpscalingInfo( uint32_t displayWidth, uint32_t displayHeight );

	void MarkFrameEvent( Tr2RenderContextEnum::FrameEvent frameEvent );

public:
	bool m_usingEXDevice;

	CComPtr<ID3D11Device>			m_d3dDevice11;

	CComPtr<IDXGISwapChain>			m_swapChain;
	CComPtr<IDXGIFactory>			m_dxgiFactory;
	CComPtr<IDXGIOutput>			m_dxgiOutput;
		
	Tr2TextureAL m_defaultBackBuffer;
		
private:
	ALResult CreateBackBuffers( const Tr2PresentParametersAL& presentationParameters );

	uint32_t			m_vsyncInterval;

	// Device statistics
	//CComPtr<ID3D11Query> m_deviceStatistics;
	//bool m_deviceStatisticsQueryEmpty;

	Tr2BufferAL m_zeroVertexBuffer;

	// Device statistics
	CComPtr<ID3D11Query> m_deviceStatistics;
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
