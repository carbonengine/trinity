// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TriStepPythonCB_H
#define TriStepPythonCB_H


#include "TriRenderStep.h"

BLUE_CLASS( TriStepPythonCB ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepPythonCB( IRoot* lockobj = 0 );
	~TriStepPythonCB();

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

private:
	BlueScriptCallback m_callback;

	void SetCallback( const BlueScriptCallback& callback );

	TriStepPythonCB( const TriStepPythonCB& );
	TriStepPythonCB& operator=( const TriStepPythonCB& );
};

TYPEDEF_BLUECLASS( TriStepPythonCB );

#endif // TriStepPythonCB_H
