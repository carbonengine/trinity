// Copyright © 2010 CCP ehf.

#pragma once
#ifndef TriStepEnableWireframeMode_h
#define TriStepEnableWireframeMode_h


#include "TriRenderStep.h"
#include "Shader/Tr2EffectStateManager.h"

BLUE_DECLARE_INTERFACE( ITr2VisualizationModeRenderer );

// -------------------------------------------------------------
// Description:
//   TriStepEnableWireframeMode is a render step that
//   sets the wireframe for a renderjob.
// SeeAlso:
//   TriRenderStep
// -------------------------------------------------------------
BLUE_CLASS( TriStepEnableWireframeMode ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepEnableWireframeMode( IRoot* lockobj = 0 );
	~TriStepEnableWireframeMode( void );

	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

protected:
	void py__init__( bool value );

private:
	bool m_enableWireframe;
};

TYPEDEF_BLUECLASS( TriStepEnableWireframeMode );

#endif // TriStepEnableWireframeMode_h