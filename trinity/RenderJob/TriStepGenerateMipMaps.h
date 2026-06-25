// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TriStepGenerateMipMaps_h_
#define TriStepGenerateMipMaps_h_


#include "TriRenderStep.h"
#include "Tr2RenderTarget.h"

BLUE_CLASS( TriStepGenerateMipMaps ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepGenerateMipMaps( IRoot* lockobj = 0 );
	~TriStepGenerateMipMaps( void );

	//IRenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

	void py__init__( Tr2RenderTarget * rt );

protected:
	Tr2RenderTargetPtr m_renderTarget;
};

TYPEDEF_BLUECLASS( TriStepGenerateMipMaps );

#endif