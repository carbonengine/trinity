#include "StdAfx.h"
#include "TriStepPushViewport.h"
#include "Tr2Renderer.h"

TriStepResult TriStepPushViewport::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	renderContext.m_esm.PushViewport();
	return RS_OK;
}
