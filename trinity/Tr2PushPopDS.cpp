// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2PushPopDS.h"
#include "Tr2Renderer.h"

Tr2PushPopDS::Tr2PushPopDS() :
	m_renderContext( nullptr )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	renderContext.m_esm.PushDepthStencilBuffer();
}

Tr2PushPopDS::Tr2PushPopDS( const Tr2TextureAL& ds ) :
	m_renderContext( nullptr )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	renderContext.m_esm.PushDepthStencilBuffer( ds );
}

Tr2PushPopDS::Tr2PushPopDS( Tr2RenderContext& renderContext ) :
	m_renderContext( &renderContext )
{
	renderContext.m_esm.PushDepthStencilBuffer();
}

Tr2PushPopDS::Tr2PushPopDS( const Tr2TextureAL& ds, Tr2RenderContext& renderContext ) :
	m_renderContext( &renderContext )
{
	renderContext.m_esm.PushDepthStencilBuffer( ds );
}

Tr2PushPopDS::~Tr2PushPopDS()
{
	if( m_renderContext )
	{
		m_renderContext->m_esm.PopDepthStencilBuffer();
	}
	else
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		renderContext.m_esm.PopDepthStencilBuffer();
	}
}
