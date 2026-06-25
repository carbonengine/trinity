// Copyright © 2023 CCP ehf.

#pragma once
#ifndef _TRISTEPSETVIEW_H_
#define _TRISTEPSETVIEW_H_

#include "TriRenderStep.h"
#include "TriView.h"

// forwards
BLUE_DECLARE( EveCamera );

BLUE_CLASS( TriStepSetView ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepSetView( IRoot* lockobj = 0 );
	~TriStepSetView( void );

	//IRenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

	// Python __init__ constructor
	void SetViewCameraParent( TriView * view, EveCamera * camera );

private:
	TriViewPtr m_view;
	EveCameraPtr m_camera;
};

TYPEDEF_BLUECLASS( TriStepSetView );

#endif