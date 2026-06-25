// Copyright © 2023 CCP ehf.

#pragma once
#ifndef _TRISTEPSETRENDERTARGET_H_
#define _TRISTEPSETRENDERTARGET_H_


#include "TriRenderStep.h"
#include "Tr2RenderTarget.h"

BLUE_CLASS( TriStepSetRenderTarget ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepSetRenderTarget( IRoot* lockobj = 0 );
	~TriStepSetRenderTarget( void );

	void py__init__( Tr2RenderTarget * renderTarget );

	//IRenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

	Tr2RenderTargetPtr m_renderTarget;
};

TYPEDEF_BLUECLASS( TriStepSetRenderTarget );

#endif