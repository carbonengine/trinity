////////////////////////////////////////////////////////////
//
//    Created:   April 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"

#if TRINITY_PLATFORM==TRINITY_DIRECTX11

#include "Tr2SwapChainALDx11.h"
#include "ALLog.h"


// --------------------------------------------------------------------------------------
// Description:
//   Tr2SwapChainAL default constructor
// --------------------------------------------------------------------------------------
Tr2SwapChainAL::Tr2SwapChainAL()
{
	memset( &m_description, 0, sizeof( m_description ) );
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
		m_description.BufferCount = 1;
		m_description.Windowed = true;
		m_description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	
		m_description.SampleDesc.Count = /*msaa*/ 1;
		m_description.SampleDesc.Quality = 0;
	
		m_description.BufferDesc.Width  = 0;
		m_description.BufferDesc.Height = 0;
		m_description.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		m_description.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;	

		CComPtr<IDXGIDevice> pDXGIDevice;
		renderContext.m_d3dDevice11->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice);
      
		CComPtr<IDXGIAdapter> pDXGIAdapter;
		pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&pDXGIAdapter);

		CComPtr<IDXGIFactory> pIDXGIFactory;
		pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), (void **)&pIDXGIFactory);

		CR_RETURN_HR( pIDXGIFactory->CreateSwapChain( renderContext.m_d3dDevice11, &m_description, &m_swapChain ) );

		CComPtr<ID3D11Texture2D> pBackBuffer;
		CR_RETURN_HR( m_swapChain->GetBuffer( 0, __uuidof (ID3D11Texture2D), (LPVOID*)&pBackBuffer ) );

		CR_RETURN_HR( m_backBuffer.Attach( pBackBuffer, renderContext ) );
		ChangeObjectId();

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
	m_backBuffer.Destroy();
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
	
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
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
int Tr2SwapChainAL::GetWidth() const
{
	return m_backBuffer.GetWidth();
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns height of the swap chain back buffer (if swap chain is not valid the result
//   is undefined).
// Return value:
//   Height of the back buffer
// --------------------------------------------------------------------------------------
int Tr2SwapChainAL::GetHeight() const
{
	return m_backBuffer.GetHeight();
}

#endif // TRINITY_PLATFORM==TRINITY_DIRECTX9
