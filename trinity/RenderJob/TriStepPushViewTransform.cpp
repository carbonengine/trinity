// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepPushViewTransform.h"
#include "Tr2Renderer.h"

TriStepResult TriStepPushViewTransform::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	Tr2Renderer::PushViewTransform();
	return RS_OK;
}
