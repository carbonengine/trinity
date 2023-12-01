#pragma once
#ifndef TriStepPopRenderTarget_h_
#define TriStepPopRenderTarget_h_


#include "TriRenderStep.h"

BLUE_CLASS( TriStepPopRenderTarget ) : public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();
	
	TriStepPopRenderTarget( IRoot* lockobj = 0);

	//IRenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );

	void py__init__( uint32_t slot = 0 );

private:
	uint32_t m_slot;
};

TYPEDEF_BLUECLASS( TriStepPopRenderTarget );

#endif
