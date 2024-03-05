#pragma once
#ifndef TriStepSetRenderState_h
#define TriStepSetRenderState_h


#include "TriRenderStep.h"

BLUE_DECLARE( TriStepSetRenderState );

class TriStepSetRenderState:
     public TriRenderStep
{
public:
    EXPOSE_TO_BLUE();
    TriStepSetRenderState( IRoot* lockobj = NULL );

	//IRenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );

	void SetStateAndValue( unsigned int state, unsigned int value );

private:
	Tr2RenderContextEnum::RenderState m_state;
	uint32_t m_value;
};

TYPEDEF_BLUECLASS( TriStepSetRenderState );
#endif //TriStepSetRenderState_h
