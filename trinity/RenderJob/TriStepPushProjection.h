#pragma once
#ifndef TriStepPushProjection_h_
#define TriStepPushProjection_h_


#include "TriRenderStep.h"

/// Wrapper around Tr2Renderer::
BLUE_CLASS( TriStepPushProjection ) : public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();
	
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );
};

TYPEDEF_BLUECLASS( TriStepPushProjection );

#endif
