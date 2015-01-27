#include "StdAfx.h"
#include "Tr2SwapChain.h"
#include "Tr2DepthStencil.h"
#include "Tr2RenderTarget.h"

Tr2SwapChain::Tr2SwapChain( IRoot* lockobj )
	:m_windowHandle( 0 )
{
}

bool Tr2SwapChain::CreateForWindow( int windowHandle )
{
	m_windowHandle = (Tr2WindowHandle)windowHandle;
	return PrepareResources();
}

void Tr2SwapChain::ReleaseResources( TriStorage s )
{
	m_swapChain.Destroy();
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
			m_backBuffer->m_name = "swapchain backbuffer";
			m_backBuffer->Attach( &m_swapChain.m_backBuffer, this );
		}
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