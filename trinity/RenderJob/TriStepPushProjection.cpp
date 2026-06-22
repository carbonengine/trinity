// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepPushProjection.h"
#include "Tr2Renderer.h"

TriStepResult TriStepPushProjection::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	Tr2Renderer::PushProjection();
	return RS_OK;
}
