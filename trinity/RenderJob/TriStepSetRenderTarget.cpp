#include "StdAfx.h"
#include "TriStepSetRenderTarget.h"
#include "Tr2Renderer.h"


TriStepSetRenderTarget::TriStepSetRenderTarget( IRoot* lockobj )
{
}

TriStepSetRenderTarget::~TriStepSetRenderTarget(void)
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer. 
// --------------------------------------------------------------------------------------
void TriStepSetRenderTarget::py__init__( Tr2RenderTarget* renderTarget )
{
	m_renderTarget = renderTarget;
}

TriStepResult TriStepSetRenderTarget::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( m_renderTarget )
	{
		renderContext.m_esm.SetRenderTarget( 0, *m_renderTarget );
	}
	return RS_OK;
}