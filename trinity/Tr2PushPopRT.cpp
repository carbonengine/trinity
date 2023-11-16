#include "StdAfx.h"
#include "Tr2PushPopRT.h"
#include "Tr2Renderer.h"

Tr2PushPopRT::Tr2PushPopRT( uint32_t slot )
: m_renderContext( nullptr )
, m_slot( slot )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	renderContext.m_esm.PushRenderTarget( slot );
}

Tr2PushPopRT::Tr2PushPopRT( const Tr2TextureAL& rt, uint32_t slot )
: m_renderContext( nullptr )
, m_slot( slot )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	renderContext.m_esm.PushRenderTarget( rt, slot );
}

Tr2PushPopRT::Tr2PushPopRT( Tr2RenderContext& renderContext, uint32_t slot )
: m_renderContext( &renderContext )
, m_slot( slot )
{
	renderContext.m_esm.PushRenderTarget( slot );
}

Tr2PushPopRT::Tr2PushPopRT( const Tr2TextureAL& rt, Tr2RenderContext& renderContext, uint32_t slot )
: m_renderContext( &renderContext )
, m_slot( slot )
{
	renderContext.m_esm.PushRenderTarget( rt, slot );
}

Tr2PushPopRT::~Tr2PushPopRT()
{
	if( m_renderContext )
	{
		m_renderContext->m_esm.PopRenderTarget( m_slot );
	}
	else
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		renderContext.m_esm.PopRenderTarget( m_slot );
	}
}
