#pragma once
#ifndef Tr2PrimaryRenderContextDx11_h_
#define Tr2PrimaryRenderContextDx11_h_

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2RenderContextDx11.h"
#include "Tr2CapsALDx11.h"

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

	//void	ReleaseDeviceResources();

	static void DestroyPrimaryRenderContext();

	Tr2RenderContextEnum::PixelFormat GetBackBufferFormat() const;
	
	bool	IsSupportedRenderTargetFormat(	Tr2RenderContextEnum::PixelFormat format, 
											bool withAutoGenMipmap = false );
	ALResult GetAFRGroupCount( uint32_t& count );

	static const uint32_t SHADER_TYPE_MASK = 
		( 1 << Tr2RenderContextEnum::VERTEX_SHADER ) |
		( 1 << Tr2RenderContextEnum::PIXEL_SHADER ) |
		( 1 << Tr2RenderContextEnum::COMPUTE_SHADER ) |
		( 1 << Tr2RenderContextEnum::GEOMETRY_SHADER ) |
		( 1 << Tr2RenderContextEnum::HULL_SHADER ) |
		( 1 << Tr2RenderContextEnum::DOMAIN_SHADER );

public:
	bool m_usingEXDevice;

	CComPtr<ID3D11Device>			m_d3dDevice11;
	CComPtr<IDXGISwapChain>			m_swapChain;
	CComPtr<IDXGIFactory>			m_dxgiFactory;
	CComPtr<IDXGIOutput>			m_dxgiOutput;
		
	std::shared_ptr<Tr2RenderTargetAL> m_defaultBackBuffer;
		
private:
	ALResult CreateBackBuffers( const Tr2PresentParametersAL& presentationParameters );

	Tr2DepthStencilAL				m_defaultDepthStencil;	// default depthstencil for the backbuffer

	uint32_t			m_vsyncInterval;

	// Device statistics
	//CComPtr<ID3D11Query> m_deviceStatistics;
	//bool m_deviceStatisticsQueryEmpty;

	Tr2VertexBufferAL m_zeroVertexBuffer;

	// Device statistics
	CComPtr<ID3D11Query> m_deviceStatistics;
	bool m_deviceStatisticsQueryEmpty;

	uint32_t m_adapterVendorId;

	Tr2CapsAL m_caps;

private:
	Tr2PrimaryRenderContextAL( const Tr2PrimaryRenderContextAL& ) /* = delete */;
	Tr2PrimaryRenderContextAL& operator=( const Tr2PrimaryRenderContextAL& ) /* = delete */;
};

#endif // ( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#endif //Tr2PrimaryRenderContextDx11_h_
