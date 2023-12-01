#pragma once
#ifndef TriStepPopViewTransform_h_
#define TriStepPopViewTransform_h_


#include "TriRenderStep.h"

/// Wrapper around Tr2Renderer::
BLUE_CLASS( TriStepPopViewTransform ) : public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();
	
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );
};

TYPEDEF_BLUECLASS( TriStepPopViewTransform );

#endif
