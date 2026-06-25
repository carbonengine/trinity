// Copyright © 2019 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2SwapChainALDx12.h"
#include "Tr2TextureALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "Utilities.h"


extern bool g_requestDeviceDebugLayer;

using namespace Tr2RenderContextEnum;

namespace Tr2SwapChainUtils
{

DXGI_FORMAT SafeConvertD3DBackBufferFormat( Tr2RenderContextEnum::PixelFormat bbFormat )
{
	DXGI_FORMAT out = static_cast<DXGI_FORMAT>( bbFormat );
	if( out == DXGI_FORMAT_B8G8R8X8_UNORM )
	{
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	}
	if( out == Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN )
	{
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	}
	return out;
}

ALResult CreateSwapChain(
	CComPtr<IDXGISwapChain4>& swapChain,
	Tr2WindowHandle focusWindow,
	const Tr2PresentParametersAL& presentationParameters,
	ID3D12CommandQueue* commandQueue,
	IDXGIOutput* output,
	Tr2PrimaryRenderContextAL& renderContext )
{
	CComPtr<IDXGIFactory4> factory;

	UINT createFactoryFlags = 0;
	if( g_requestDeviceDebugLayer )
	{
		createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
	}

	CR_RETURN_HR( renderContext.CreateFactory2( createFactoryFlags, factory ) );

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = presentationParameters.mode.width;
	swapChainDesc.Height = presentationParameters.mode.height;
	swapChainDesc.Format = SafeConvertD3DBackBufferFormat( presentationParameters.mode.format );
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc.Count = std::max( presentationParameters.msaaType, 1u );
	swapChainDesc.SampleDesc.Quality = presentationParameters.msaaQuality;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = Tr2SwapChainUtils::BACK_BUFFER_COUNT;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH; // DXGI_MODE_SCALING( presentationParameters.mode.scaling );
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = 0;

	if( presentationParameters.variableRefreshRateSupported )
	{
		swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	}

	auto wnd = Tr2WindowHandle( presentationParameters.outputWindow );
	if( !presentationParameters.outputWindow )
	{
		wnd = focusWindow;
	}

	CR_RETURN_HR( renderContext.CreateSwapChainForHwnd( factory, commandQueue, wnd, &swapChainDesc, nullptr, output, swapChain ) );

	CR_RETURN_HR( factory->MakeWindowAssociation( wnd, DXGI_MWA_NO_ALT_ENTER ) );
	return S_OK;
}

ALResult FillPresentationParameters( Tr2PresentParametersAL& presentationParameters, Tr2WindowHandle windowHandle )
{
	RECT rect;
	if( !GetClientRect( reinterpret_cast<HWND>( windowHandle ), &rect ) )
	{
		return E_INVALIDARG;
	}
	if( rect.left >= rect.right || rect.top >= rect.bottom )
	{
		return E_INVALIDCALL;
	}
	Tr2PresentParametersAL pp = {
		{ uint32_t( rect.right - rect.left ), uint32_t( rect.bottom - rect.top ), 0, 0, PIXEL_FORMAT_B8G8R8X8_UNORM, SCANLINE_ORDER_UNSPECIFIED, DISPLAY_SCALING_UNSPECIFIED },
		Tr2SwapChainUtils::BACK_BUFFER_COUNT,
		1,
		0,
		SWAP_EFFECT_DISCARD,
		windowHandle,
		true,
		false,
		PRESENT_INTERVAL_IMMEDIATE,
		false
	};
	presentationParameters = pp;
	return S_OK;
}
}

namespace TrinityALImpl
{

Tr2SwapChainAL::Tr2SwapChainAL() :
	m_owner( nullptr )
{
	m_backBuffer.m_texture = std::make_shared<TrinityALImpl::Tr2TextureAL>();
	memset( &m_presentParameters, 0, sizeof( m_presentParameters ) );
}

Tr2SwapChainAL::~Tr2SwapChainAL()
{
	Destroy();
}

ALResult Tr2SwapChainAL::Create( Tr2WindowHandle windowHandle, Tr2PrimaryRenderContextAL& renderContext )
{
	Destroy();
	if( !renderContext.IsValid() )
	{
		return E_INVALIDCALL;
	}

	FORWARD_HR( renderContext.FlushAndSyncDx12( renderContext ) );
	renderContext.FlushPendingRelease(); // Make sure we have no references to the old swap chain lingering around

	Tr2PresentParametersAL presentationParameters;
	CR_RETURN_HR( Tr2SwapChainUtils::FillPresentationParameters( presentationParameters, windowHandle ) );

	return CreateDx12( presentationParameters, nullptr, renderContext.m_commandQueue, renderContext );
}

ALResult Tr2SwapChainAL::CreateDx12( const Tr2PresentParametersAL& presentationParameters, IDXGIOutput* output, ID3D12CommandQueue* commandQueue, Tr2PrimaryRenderContextAL& renderContext )
{
	CComPtr<IDXGISwapChain4> swapChain;
	FORWARD_HR( Tr2SwapChainUtils::CreateSwapChain( swapChain, presentationParameters.outputWindow, presentationParameters, commandQueue, output, renderContext ) );

	std::vector<std::shared_ptr<RenderTargetViewDx12>> rtvs;
	std::vector<CComPtr<ID3D12Resource>> backBuffers;
	FORWARD_HR( GetBackBuffers( &renderContext, backBuffers, rtvs, swapChain ) );

	m_swapChain = swapChain;
	m_presentParameters = presentationParameters;
	m_owner = &renderContext;

	m_backBuffer.m_texture->AssignFromSwapChainDx12( backBuffers, rtvs, renderContext );
	m_backBuffer.m_texture->SetSwapChainBufferIndexDx12( swapChain->GetCurrentBackBufferIndex() );

	renderContext.ResourceBarrierDx12( TrinityALImpl::Transition( m_backBuffer.m_texture->GetResourceDx12(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET ) );

	return S_OK;
}

void Tr2SwapChainAL::Destroy()
{
	if( m_owner )
	{
		m_backBuffer.m_texture->Destroy();

		m_owner = nullptr;
		m_swapChain = nullptr;

		m_presentParameters = Tr2PresentParametersAL();
	}
}

bool Tr2SwapChainAL::IsValid() const
{
	return m_swapChain != nullptr;
}

ALResult Tr2SwapChainAL::Present( Tr2RenderContextAL& renderContext )
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}

	m_owner->ScheduleSwapchainPresentDx12( m_swapChain, m_backBuffer, m_presentParameters.presentInterval & 0xf );
	return S_OK;
}

uint32_t Tr2SwapChainAL::GetWidth() const
{
	return m_presentParameters.mode.width;
}

uint32_t Tr2SwapChainAL::GetHeight() const
{
	return m_presentParameters.mode.height;
}

Tr2ALMemoryType Tr2SwapChainAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}

/** Gather backbuffer textures and RTVs (JB: Make this public-static since it was duplicated) */
ALResult Tr2SwapChainAL::GetBackBuffers(
	Tr2PrimaryRenderContextAL* primaryContext,
	std::vector<CComPtr<ID3D12Resource>>& backBuffers,
	std::vector<std::shared_ptr<RenderTargetViewDx12>>& rtvs,
	IDXGISwapChain1* swapChain )
{
	DXGI_SWAP_CHAIN_DESC scDesc;
	swapChain->GetDesc( &scDesc );

	for( uint32_t i = 0; i < Tr2SwapChainUtils::BACK_BUFFER_COUNT; ++i )
	{
		CComPtr<ID3D12Resource> backBuffer;
		CR_RETURN_HR( swapChain->GetBuffer( i, IID_PPV_ARGS( &backBuffer ) ) );
		backBuffers.push_back( backBuffer );

		std::shared_ptr<RenderTargetViewDx12> view;
		primaryContext->CreateRenderTargetView( backBuffer, nullptr, view );
		rtvs.push_back( view );


		D3D12_RENDER_TARGET_VIEW_DESC rtv = { DXGI_FORMAT( Tr2RenderContextEnum::MakeSrgb( Tr2RenderContextEnum::PixelFormat( scDesc.BufferDesc.Format ) ) ), D3D12_RTV_DIMENSION_TEXTURE2D };
		rtv.Texture2D.MipSlice = rtv.Texture2D.PlaneSlice = 0;

		primaryContext->CreateRenderTargetView( backBuffer, &rtv, view );
		rtvs.push_back( view );
	}
	return S_OK;
}

void Tr2SwapChainAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2SwapChainAL";
	description["width"] = std::to_string( m_presentParameters.mode.width );
	description["height"] = std::to_string( m_presentParameters.mode.height );
	description["name"] = m_name;
}

ALResult Tr2SwapChainAL::SetName( const char* name )
{
	m_name = name;
	if( m_swapChain )
	{
		m_swapChain->SetPrivateData( WKPDID_D3DDebugObjectName, UINT( strlen( name ) ), name );
	}
	return m_backBuffer.SetName( name );
}

}
#endif