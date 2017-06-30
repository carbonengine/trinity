#pragma once
#ifndef _TRISTEPRENDERSHADER_H_
#define _TRISTEPRENDERSHADER_H_

#include "StdAfx.h"

#include "TriRenderStep.h"

BLUE_DECLARE( Tr2Material );

BLUE_CLASS( TriStepRenderFullScreenShader ) : public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();
	
	TriStepRenderFullScreenShader( IRoot* lockobj = 0 );
	~TriStepRenderFullScreenShader(void);

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );

	// Python __init__ constructor
	void py__init__( Tr2Material* shader );

private:
	Tr2MaterialPtr m_shader;
};

TYPEDEF_BLUECLASS( TriStepRenderFullScreenShader );

#endif