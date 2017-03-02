////////////////////////////////////////////////////////////
//
//    Created:   April 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"

#if TRINITY_PLATFORM==TRINITY_DIRECTX9

#include "Tr2SwapChainALDx9.h"
#include "ALLog.h"


// --------------------------------------------------------------------------------------
// Description:
//   Tr2SwapChainAL default constructor
// --------------------------------------------------------------------------------------
Tr2SwapChainAL::Tr2SwapChainAL()
{
	memset( &m_presentParam, 0, sizeof( m_presentParam ) );
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriDeviceResource interface. Destroys device resources (swap chain, 
//   buffers).
// Arguments:
//   s - Resource types to destroy
// --------------------------------------------------------------------------------------
void Tr2SwapChainAL::ReleaseALResource()
{
	Destroy();
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriDeviceResource interface. Re-creates swap chain and associated 
//   buffers.
// Return value:
//   true If swap chain and buffers were successfully created
//   false On error
// --------------------------------------------------------------------------------------
void Tr2SwapChainAL::PrepareALResource( Tr2PrimaryRenderContextAL& renderContext )
{
	CR( Create( m_presentParam.hDeviceWindow, renderContext ) );
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
ALResult Tr2SwapChainAL::Create( Tr2WindowHandle windowHandle, Tr2RenderContextAL& renderContext )
{
	Destroy();

	m_presentParam.hDeviceWindow = windowHandle;

	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	if( windowHandle )
	{
		m_presentParam.BackBufferWidth		= 0;
		m_presentParam.BackBufferHeight		= 0;
		m_presentParam.BackBufferCount		= 1;
		m_presentParam.BackBufferFormat		= D3DFMT_UNKNOWN;

		m_presentParam.MultiSampleType		= D3DMULTISAMPLE_NONE;		
		m_presentParam.MultiSampleQuality	= 0;		

		m_presentParam.SwapEffect			= D3DSWAPEFFECT_DISCARD;
		m_presentParam.Windowed				= TRUE;

		m_presentParam.EnableAutoDepthStencil = FALSE;
		m_presentParam.AutoDepthStencilFormat = D3DFMT_D24S8;

		m_presentParam.Flags				= 0;
		m_presentParam.FullScreen_RefreshRateInHz = 0;
		m_presentParam.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

		CR_RETURN_HR( renderContext.m_d3dDevice9->CreateAdditionalSwapChain( &m_presentParam, &m_swapChain ) );


		CComPtr<IDirect3DSurface9> backBuffer;
		CR_RETURN_HR( m_swapChain->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer ) );
		m_backBuffer.Attach( backBuffer, renderContext );
		ChangeObjectId();
	}
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Destroys device resources (swap chain, buffers).
// --------------------------------------------------------------------------------------
void Tr2SwapChainAL::Destroy()
{
	m_swapChain = NULL;
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
ALResult Tr2SwapChainAL::Present( Tr2RenderContextAL& renderContext )
{
	if( !m_swapChain || !renderContext.IsValid() )
	{
		return E_FAIL;
	}
	
	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	renderContext.m_d3dDevice9->EndScene();
	HRESULT hr = m_swapChain->Present( NULL, NULL, NULL, NULL, NULL );
	renderContext.m_d3dDevice9->BeginScene();
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
	return (int)m_presentParam.BackBufferWidth;
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
	return (int)m_presentParam.BackBufferHeight;
}
#endif // TRINITY_PLATFORM==TRINITY_DIRECTX9
