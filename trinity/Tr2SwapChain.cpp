// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2SwapChain.h"
#include "Tr2DepthStencil.h"
#include "Tr2RenderTarget.h"

Tr2SwapChain::Tr2SwapChain( IRoot* lockobj ) :
	m_windowHandle( 0 )
{
}

bool Tr2SwapChain::CreateForWindow( size_t windowHandle )
{
#if __APPLE__
	void* windowHandleAsPtr = (void*)windowHandle;
	m_windowHandle = (__bridge Tr2WindowHandle)windowHandleAsPtr;
#else
	m_windowHandle = (Tr2WindowHandle)windowHandle;
#endif
	return PrepareResources();
}

void Tr2SwapChain::ReleaseResources( TriStorage s )
{
	m_swapChain = Tr2SwapChainAL();
}

bool Tr2SwapChain::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( SUCCEEDED( m_swapChain.Create( m_windowHandle, renderContext ) ) )
	{
		if( !m_depthStencil )
		{
			m_depthStencil.Attach( new OTr2DepthStencil );
		}
		CR_RETURN_VAL( HRESULT( m_depthStencil->Create( m_swapChain.GetWidth(), m_swapChain.GetHeight(), Tr2RenderContextEnum::DSFMT_D24S8, 0, 0 ) ), false );
		if( !m_backBuffer )
		{
			m_backBuffer.CreateInstance();
			m_backBuffer->SetName( "swapchain backbuffer" );
		}
		m_backBuffer->Attach( m_swapChain.GetBackBuffer(), this );
		return true;
	}
	return false;
}

bool Tr2SwapChain::Present( Tr2RenderContext& renderContext )
{
	return SUCCEEDED( m_swapChain.Present( renderContext ) );
}

int Tr2SwapChain::GetWidth() const
{
	return m_swapChain.GetWidth();
}

int Tr2SwapChain::GetHeight() const
{
	return m_swapChain.GetHeight();
}
