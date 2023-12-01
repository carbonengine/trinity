#include "StdAfx.h"
#include "TriStepPopDepthStencil.h"
#include "Tr2Renderer.h"


TriStepPopDepthStencil::TriStepPopDepthStencil( IRoot* lockobj )
{
}

TriStepPopDepthStencil::~TriStepPopDepthStencil(void)
{
}

TriStepResult TriStepPopDepthStencil::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	renderContext.m_esm.PopDepthStencilBuffer();
	return RS_OK;
}