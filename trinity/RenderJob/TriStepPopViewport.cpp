#include "StdAfx.h"
#include "TriStepPopViewport.h"
#include "Tr2Renderer.h"

TriStepResult TriStepPopViewport::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	renderContext.m_esm.PopViewport();
	return RS_OK;
}
