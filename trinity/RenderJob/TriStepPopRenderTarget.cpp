#include "StdAfx.h"
#include "TriStepPopRenderTarget.h"
#include "Tr2Renderer.h"


TriStepPopRenderTarget::TriStepPopRenderTarget( IRoot* lockobj ) :
	m_slot( 0 )
{
}

TriStepResult TriStepPopRenderTarget::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	renderContext.m_esm.PopRenderTarget( m_slot );
	return RS_OK;
}

void TriStepPopRenderTarget::py__init__( uint32_t slot )
{
	m_slot = slot;
}