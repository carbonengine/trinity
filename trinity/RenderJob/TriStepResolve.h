// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TriStepResolve_h
#define TriStepResolve_h


#include "TriRenderStep.h"

BLUE_DECLARE( Tr2RenderTarget );

BLUE_CLASS( TriStepResolve ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepResolve( IRoot* lockobj = 0 );

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

	void py__init__( Tr2RenderTarget * destination, Tr2RenderTarget * source );

private:
	Tr2RenderTargetPtr m_source;
	Tr2RenderTargetPtr m_destination;
	bool m_generateMipmap;
};

TYPEDEF_BLUECLASS( TriStepResolve );

#endif