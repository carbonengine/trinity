// Copyright © 2023 CCP ehf.

#pragma once
#ifndef _TRISTEPRENDERSCENE_H_
#define _TRISTEPRENDERSCENE_H_


#include "TriRenderStep.h"
#include "include/ITr2Scene.h"


BLUE_CLASS( TriStepRenderScene ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepRenderScene( IRoot* lockobj = 0 );
	~TriStepRenderScene( void );

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

	void py__init__( ITr2Scene * scene );

	ITr2ScenePtr m_scene;
};

TYPEDEF_BLUECLASS( TriStepRenderScene );

#endif