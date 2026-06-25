// Copyright © 2023 CCP ehf.

#pragma once
#ifndef _TRISTEPRENDERSCENEDEBUG_H_
#define _TRISTEPRENDERSCENEDEBUG_H_


#include "TriRenderStep.h"
#include "include/ITr2Scene.h"


BLUE_CLASS( TriStepRenderSceneDebug ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepRenderSceneDebug( IRoot* lockobj = 0 );
	~TriStepRenderSceneDebug( void );

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

	// Python __init__ constructor
	void py__init__( ITr2Scene * scene );

private:
	ITr2ScenePtr m_scene;
};

TYPEDEF_BLUECLASS( TriStepRenderSceneDebug );

#endif