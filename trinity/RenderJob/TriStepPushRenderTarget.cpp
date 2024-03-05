#include "StdAfx.h"
#include "TriStepPushRenderTarget.h"
#include "Tr2Renderer.h"


TriStepPushRenderTarget::TriStepPushRenderTarget( IRoot* lockobj ) :
	m_slot( 0 )
{
}

TriStepResult TriStepPushRenderTarget::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( m_renderTarget )
	{
		renderContext.m_esm.PushRenderTarget( *m_renderTarget, m_slot );
	}
	else
	{
		renderContext.m_esm.PushRenderTarget( m_slot );
	}
	return RS_OK;
}

void TriStepPushRenderTarget::py__init__( Tr2RenderTarget* rt, uint32_t slot )
{
	m_renderTarget = rt;
	m_slot = slot;
}
