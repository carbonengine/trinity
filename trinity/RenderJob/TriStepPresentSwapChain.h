#pragma once
#ifndef TriStepPresentSwapChain_h
#define TriStepPresentSwapChain_h

#include "TriRenderStep.h"
#include "Tr2SwapChain.h"

BLUE_DECLARE( TriStepPresentSwapChain );

class TriStepPresentSwapChain:
     public TriRenderStep
{
public:
    EXPOSE_TO_BLUE();
    TriStepPresentSwapChain( IRoot* lockobj = NULL );

	//IRenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );

	void py__init__( Tr2SwapChain* sc );

private:
	Tr2SwapChainPtr m_swapChain;
};

TYPEDEF_BLUECLASS( TriStepPresentSwapChain );
#endif //TriStepPresentSwapChain_h
