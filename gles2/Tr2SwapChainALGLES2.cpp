////////////////////////////////////////////////////////////
//
//    Created:   April 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"

#if TRINITY_PLATFORM==TRINITY_OPENGLES2

#include "Tr2SwapChainALGLES2.h"
#include "Tr2RenderContextGLES2.h"

#if !defined(_WIN32) && !defined(TRINITY_AL_MOBILE)
#include "GLFW/glfw3.h"
#endif

using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------------
// Description:
//   Tr2SwapChainAL default constructor
// --------------------------------------------------------------------------------------
Tr2SwapChainAL::Tr2SwapChainAL()
	:
#ifdef _WIN32
	m_hDC( 0 ),
#endif
	m_hWnd( 0 ),
	m_width( 0 ),
	m_height( 0 )
{
}

Tr2SwapChainAL::~Tr2SwapChainAL()
{
	Destroy();
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
	m_backBuffer.Destroy();
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
	if( !m_hWnd || m_backBuffer.IsValid() )
	{
		return;
	}
	CR( CreateFramebuffer( renderContext ) );
}

ALResult Tr2SwapChainAL::CreateFramebuffer( Tr2RenderContextAL& renderContext )
{
#ifdef _WIN32
	m_width = 0;
	m_height = 0;

	RECT rect = { 0 };
	GetClientRect( (Tr2WindowHandle)m_hWnd, &rect );

	if( rect.right > rect.left )
	{
		m_width = uint32_t( rect.right - rect.left );
	}
	else
	{
		m_width = 8;
	}
	if( rect.bottom > rect.top )
	{
		m_height = uint32_t( rect.bottom - rect.top );
	}
	else
	{
		m_height = 8;
	}
#elif defined(TRINITY_AL_MOBILE)
    return E_FAIL;
#else
    int width = 16, height = 16;
    glfwGetWindowSize( reinterpret_cast<GLFWwindow*>( m_hWnd ), &width, &height );
    m_width = width;
    m_height = height;
#endif
	CR_RETURN_HR( m_backBuffer.Create(
		m_width,
		m_height,
		1,
		PIXEL_FORMAT_B8G8R8A8_UNORM,
		Tr2MsaaDesc(),
		0,
		EX_NONE,
		renderContext ) );

	return S_OK;
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

	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
#if defined(TRINITY_AL_MOBILE)
    return E_FAIL;
#endif
	m_hWnd = windowHandle;
#ifdef _WIN32
	m_hDC = GetDC( (Tr2WindowHandle)m_hWnd );

	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory( &pfd, sizeof( pfd ) );
	pfd.nSize = sizeof( pfd );
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;
	int iFormat = ChoosePixelFormat( m_hDC, &pfd );
	SetPixelFormat( m_hDC, iFormat, &pfd );
#endif
	CreateFramebuffer( renderContext );
	ChangeObjectId();
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Destroys device resources (swap chain, buffers).
// --------------------------------------------------------------------------------------
void Tr2SwapChainAL::Destroy()
{
	m_backBuffer.Destroy();
#ifdef _WIN32
	::ReleaseDC( (Tr2WindowHandle)m_hWnd, m_hDC );
	m_hDC = 0;
#endif
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
	return m_backBuffer.IsValid();
}

// --------------------------------------------------------------------------------------
// Description:
//   Presents the swap chain back buffer to the window.
// Return value:
//   HRESULT value
// --------------------------------------------------------------------------------------
ALResult Tr2SwapChainAL::Present( Tr2RenderContextAL& renderContext )
{
#ifdef TRINITY_AL_MOBILE
    return E_FAIL;
#else
#ifdef _WIN32
#pragma warning( disable: 4189 )	// scopeguard

	if( !renderContext.IsValid() || !m_backBuffer.IsValid() )
	{
		return E_FAIL;
	}
	wglMakeCurrent( m_hDC, renderContext.m_hRC );
#else
    glfwMakeContextCurrent( reinterpret_cast<GLFWwindow*>( m_hWnd ) );
#endif

	AL_UPDATE_RESOURCE_FRAME_USAGE( *this );
	renderContext.InternalBlitToBackBuffer( m_backBuffer.GetTexture() );

#ifdef _WIN32
	SwapBuffers( m_hDC );
	wglMakeCurrent( renderContext.m_hDC, renderContext.m_hRC );
#else
    glfwSwapBuffers( reinterpret_cast<GLFWwindow*>( m_hWnd ) );
    glfwMakeContextCurrent( reinterpret_cast<GLFWwindow*>( renderContext.m_hWnd ) );
#endif
	return S_OK;
#endif
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
	return m_width;
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
	return m_height;
}

#endif // TRINITY_PLATFORM==TRINITY_DIRECTX9
