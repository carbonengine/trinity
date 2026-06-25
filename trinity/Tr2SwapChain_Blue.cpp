// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2SwapChain.h"
#include "Tr2RenderTarget.h"
#include "Tr2DepthStencil.h"

BLUE_DEFINE( Tr2SwapChain );

const Be::ClassInfo* Tr2SwapChain::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2SwapChain, "" )
		MAP_INTERFACE( Tr2SwapChain )

		MAP_ATTRIBUTE( "backBuffer", m_backBuffer, "Render target for the swap chain.", Be::READ );
		MAP_ATTRIBUTE( "depthStencilBuffer", m_depthStencil, "Depth-stencil target for the swap chain; can be None.", Be::READ );

		MAP_PROPERTY_READONLY( "width", GetWidth, "Width of the backBuffer" )
		MAP_PROPERTY_READONLY( "height", GetHeight, "Height of the backBuffer" )

		MAP_METHOD_AND_WRAP(
			"CreateForWindow",
			CreateForWindow,
			"Creates a swap chain and associates with the given window\n"
			":param hwnd: window handle" )

	EXPOSURE_END()
}
