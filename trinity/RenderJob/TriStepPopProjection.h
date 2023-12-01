#pragma once
#ifndef TriStepPopProjection_h_
#define TriStepPopProjection_h_


#include "TriRenderStep.h"

/// Wrapper around Tr2Renderer::
BLUE_CLASS( TriStepPopProjection ) : public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();
	
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );
};

TYPEDEF_BLUECLASS( TriStepPopProjection );

#endif
