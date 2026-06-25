// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TriStepPopViewport_h_
#define TriStepPopViewport_h_


#include "TriRenderStep.h"

/// Wrapper around Tr2Renderer::
BLUE_CLASS( TriStepPopViewport ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );
};

TYPEDEF_BLUECLASS( TriStepPopViewport );

#endif
