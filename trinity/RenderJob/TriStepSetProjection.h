// Copyright © 2023 CCP ehf.

#pragma once
#ifndef _TRISTEPSETPROJECTION_H_
#define _TRISTEPSETPROJECTION_H_


#include "TriRenderStep.h"
#include "TriProjection.h"

BLUE_CLASS( TriStepSetProjection ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepSetProjection( IRoot* lockobj = 0 );
	~TriStepSetProjection( void );

	//IRenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

	// Python __init__ constructor
	void SetProjection( TriProjection * projection );

private:
	TriProjectionPtr m_projection;
};

TYPEDEF_BLUECLASS( TriStepSetProjection );

#endif