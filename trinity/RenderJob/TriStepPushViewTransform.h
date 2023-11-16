#pragma once
#ifndef TriStepPushViewTransform_h_
#define TriStepPushViewTransform_h_


#include "TriRenderStep.h"

/// Wrapper around Tr2Renderer::
BLUE_CLASS( TriStepPushViewTransform ) : public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();
	
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );
};

TYPEDEF_BLUECLASS( TriStepPushViewTransform );

#endif
