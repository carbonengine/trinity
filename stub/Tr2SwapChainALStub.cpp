#include "StdAfx.h"

#if TRINITY_PLATFORM==TRINITY_STUB

#include "Tr2SwapChainALStub.h"
#include "ALLog.h"


// --------------------------------------------------------------------------------------
// Description:
//   Tr2SwapChainAL default constructor
// --------------------------------------------------------------------------------------
Tr2SwapChainAL::Tr2SwapChainAL()
	:m_isValid(false)
{
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
	m_isValid = false;
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
	CR( Create( 0, renderContext ) );
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
	if ( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	Destroy();
	m_isValid = true;
	Tr2RenderTargetAL& defaultRenderTarget = renderContext.GetDefaultBackBuffer();
#ifdef _WIN32
	RECT sizeRect;
	GetClientRect(windowHandle, &sizeRect);
	uint32_t width = sizeRect.right - sizeRect.left;
	uint32_t height = sizeRect.bottom - sizeRect.top;
#elif defined(TRINITY_AL_MOBILE)
    return E_FAIL;
#else
    uint32_t width = ( windowHandle & 0xffff );
    uint32_t height = ( windowHandle >> 16 ) & 0xffff;
#endif
	uint32_t one = 1;
	width = std::max(width, one);
	height = std::max(height, one);
	m_backBuffer.Create(width, height, defaultRenderTarget.GetMipCount(), defaultRenderTarget.GetFormat(), renderContext);
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Destroys device resources (swap chain, buffers).
// --------------------------------------------------------------------------------------
void Tr2SwapChainAL::Destroy()
{
	m_isValid = false;
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
	return m_isValid;
}

// --------------------------------------------------------------------------------------
// Description:
//   Presents the swap chain back buffer to the window.
// Return value:
//   HRESULT value
// --------------------------------------------------------------------------------------
ALResult Tr2SwapChainAL::Present( Tr2RenderContextAL& renderContext )
{
	return S_OK;
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
#endif // TRINITY_PLATFORM==TRINITY_STUB
