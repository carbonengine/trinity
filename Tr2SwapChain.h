#pragma once

#ifndef Tr2SwapChain_h
#define Tr2SwapChain_h


#include "Tr2DeviceResource.h"

BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE( Tr2DepthStencil );

BLUE_DECLARE( Tr2SwapChain );

class Tr2SwapChain:
     public IRoot,
     public Tr2DeviceResource
{
public:
    EXPOSE_TO_BLUE();

	Tr2SwapChain( IRoot* lockobj = NULL );

	bool CreateForWindow( size_t windowHandle );
	bool Present( Tr2RenderContext& renderContext );

	int GetWidth() const;
	int GetHeight() const;

    /////////////////////////////////////////////////////////////////////////////
    // ITriDeviceResource
    void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();

	Tr2WindowHandle m_windowHandle;
	Tr2SwapChainAL m_swapChain;
	Tr2RenderTargetPtr m_backBuffer;
	Tr2DepthStencilPtr m_depthStencil;
};

TYPEDEF_BLUECLASS( Tr2SwapChain );

#endif //Tr2SwapChain_h
