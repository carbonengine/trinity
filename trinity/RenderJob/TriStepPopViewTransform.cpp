// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepPopViewTransform.h"
#include "Tr2Renderer.h"

TriStepResult TriStepPopViewTransform::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	Tr2Renderer::PopViewTransform();
	return RS_OK;
}
