////////////////////////////////////////////////////////////
//
//    Created:   April 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"

#if TRINITY_PLATFORM==TRINITY_DIRECTX11

#include "Tr2SwapChainALDx11.h"
#include "Tr2TextureALDx11.h"
#include "ALLog.h"
#include "Tr2PrimaryRenderContextDx11.h"


namespace TrinityALImpl
{
	// --------------------------------------------------------------------------------------
	// Description:
	//   Tr2SwapChainAL default constructor
	// --------------------------------------------------------------------------------------
	Tr2SwapChainAL::Tr2SwapChainAL()
		:m_width( 0 ),
		m_height( 0 )
	{
		memset( &m_description, 0, sizeof( m_description ) );
		m_backBuffer.m_texture = std::make_shared<TrinityALImpl::Tr2TextureAL>();
	}

	// --------------------------------------------------------------------------------------
	// Description:
	//   Creates swap chain and associated buffers.
	// Arguments:
	//   windowHandle - Handle to window object (Tr2WindowHandle) for which the swap chain is created
	//   renderContext - Current render context
	// Return value:
	//   HRESULT value
	// --------------------------------------------------------------------------------------
	ALResult Tr2SwapChainAL::Create( Tr2WindowHandle windowHandle, Tr2PrimaryRenderContextAL &renderContext )
	{
		Destroy();

		m_description.OutputWindow = windowHandle;

		if( !renderContext.m_d3dDevice11 )
		{
			return E_FAIL;
		}

		if( windowHandle )
		{
			m_width = 0;
			m_height = 0;

			m_description.BufferCount = 1;
			m_description.Windowed = true;
			m_description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

			m_description.SampleDesc.Count = /*msaa*/ 1;
			m_description.SampleDesc.Quality = 0;

			m_description.BufferDesc.Width = 0;
			m_description.BufferDesc.Height = 0;
			m_description.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			m_description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

			CComPtr<IDXGIDevice> pDXGIDevice;
			renderContext.m_d3dDevice11->QueryInterface( __uuidof( IDXGIDevice ), (void **)&pDXGIDevice );

			CComPtr<IDXGIAdapter> pDXGIAdapter;
			pDXGIDevice->GetParent( __uuidof( IDXGIAdapter ), (void **)&pDXGIAdapter );

			CComPtr<IDXGIFactory> pIDXGIFactory;
			pDXGIAdapter->GetParent( __uuidof( IDXGIFactory ), (void **)&pIDXGIFactory );

			CR_RETURN_HR( pIDXGIFactory->CreateSwapChain( renderContext.m_d3dDevice11, &m_description, &m_swapChain ) );

			CComPtr<ID3D11Texture2D> pBackBuffer;
			CR_RETURN_HR( m_swapChain->GetBuffer( 0, __uuidof ( ID3D11Texture2D ), (LPVOID*)&pBackBuffer ) );

			CR_RETURN_HR( m_backBuffer.m_texture->Attach( pBackBuffer, renderContext ) );
			m_width = m_backBuffer.GetWidth();
			m_height = m_backBuffer.GetHeight();

			return S_OK;
		}
		else
		{
			return E_FAIL;
		}
	}

	// --------------------------------------------------------------------------------------
	// Description:
	//   Destroys device resources (swap chain, buffers).
	// --------------------------------------------------------------------------------------
	void Tr2SwapChainAL::Destroy()
	{
		m_swapChain = nullptr;
		m_backBuffer.m_texture->Destroy();
	}

	// --------------------------------------------------------------------------------------
	// Description:
	//   Checks if the swap chain was successfully created.
	// Return value:
	//   true If the swap chain is valid and can be used
	//   false Otherwise
	// --------------------------------------------------------------------------------------
	bool Tr2SwapChainAL::IsValid() const
	{
		return m_swapChain != nullptr;
	}

	// --------------------------------------------------------------------------------------
	// Description:
	//   Presents the swap chain back buffer to the window.
	// Return value:
	//   HRESULT value
	// --------------------------------------------------------------------------------------
	ALResult Tr2SwapChainAL::Present( Tr2RenderContextAL& /* renderContext */ )
	{
		if( !m_swapChain )
		{
			return E_FAIL;
		}

		HRESULT hr = m_swapChain->Present( 0, 0 );
		if( !SUCCEEDED( hr ) )
		{
			CCP_AL_LOGERR( "Tr2SwapChainAL::Present failed" );
		}

		return hr;
	}

	// --------------------------------------------------------------------------------------
	// Description:
	//   Returns width of the swap chain back buffer (if swap chain is not valid the result
	//   is undefined).
	// Return value:
	//   Width of the back buffer
	// --------------------------------------------------------------------------------------
	uint32_t Tr2SwapChainAL::GetWidth() const
	{
		return m_width;
	}

	// --------------------------------------------------------------------------------------
	// Description:
	//   Returns height of the swap chain back buffer (if swap chain is not valid the result
	//   is undefined).
	// Return value:
	//   Height of the back buffer
	// --------------------------------------------------------------------------------------
	uint32_t Tr2SwapChainAL::GetHeight() const
	{
		return m_height;
	}

	void Tr2SwapChainAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2SwapChainAL";
		description["width"] = std::to_string( long long( m_width ) );
		description["height"] = std::to_string( long long( m_height ) );
		description["name"] = m_name;
	}

	ALResult Tr2SwapChainAL::SetName( const char* name )
	{
		if( !m_swapChain )
		{
			return E_INVALIDCALL;
		}
		m_name = name;
		return m_swapChain->SetPrivateData( WKPDID_D3DDebugObjectName, UINT( strlen( name ) ), name );
	}

	}
#endif
