#pragma once
#ifndef TriStepPushViewport_h_
#define TriStepPushViewport_h_


#include "TriRenderStep.h"

/// Wrapper around Tr2Renderer::
BLUE_CLASS( TriStepPushViewport ) : public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();
	
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );
};

TYPEDEF_BLUECLASS( TriStepPushViewport );

#endif
