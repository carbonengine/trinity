// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TriStepPushRenderTarget_h_
#define TriStepPushRenderTarget_h_


#include "TriRenderStep.h"
#include "Tr2RenderTarget.h"

BLUE_CLASS( TriStepPushRenderTarget ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepPushRenderTarget( IRoot* lockobj = 0 );

	//IRenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

	void py__init__( Tr2RenderTarget * rt, uint32_t slot = 0 );

private:
	Tr2RenderTargetPtr m_renderTarget;
	uint32_t m_slot;
};

TYPEDEF_BLUECLASS( TriStepPushRenderTarget );

#endif
