// Copyright © 2023 CCP ehf.

#pragma once
#ifndef _TRISTEPSETVIEWPORT_H_
#define _TRISTEPSETVIEWPORT_H_


#include "TriRenderStep.h"

BLUE_DECLARE( TriViewport );

BLUE_CLASS( TriStepSetViewport ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepSetViewport( IRoot* lockobj = 0 );
	~TriStepSetViewport( void );

	//IRenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

	void SetViewport( TriViewport * viewport );

private:
	TriViewportPtr m_viewport;
};

TYPEDEF_BLUECLASS( TriStepSetViewport );

#endif