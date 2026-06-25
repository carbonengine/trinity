// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TriStepUpdate_h
#define TriStepUpdate_h


#include "TriRenderStep.h"
#include "include/ITr2Scene.h"


BLUE_CLASS( TriStepUpdate ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepUpdate( IRoot* lockobj = 0 );
	~TriStepUpdate( void );

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

	// Python __init__ constructor
	void SetUpdateable( ITr2Updateable * obj );

private:
	ITr2UpdateablePtr m_object;
};

TYPEDEF_BLUECLASS( TriStepUpdate );

#endif // TriStepUpdate_h