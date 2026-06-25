// Copyright © 2010 CCP ehf.

#pragma once
#ifndef TriStepSetVisualizationMode_h
#define TriStepSetVisualizationMode_h


#include "TriRenderStep.h"

BLUE_DECLARE_INTERFACE( ITr2VisualizationModeRenderer );

// -------------------------------------------------------------
// Description:
//   TriStepSetVisualizationMode is a render step that
//   sets scene's debug visualization mode.
// SeeAlso:
//   TriRenderStep
// -------------------------------------------------------------
BLUE_CLASS( TriStepSetVisualizationMode ) :
	public TriRenderStep
{
public:
	EXPOSE_TO_BLUE();

	TriStepSetVisualizationMode( IRoot* lockobj = 0 );
	~TriStepSetVisualizationMode( void );

	void py__init__( ITr2VisualizationModeRenderer * object, int mode );

	//RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext & renderContext );

	void SetObject( ITr2VisualizationModeRenderer * object );
	void SetVisualizationMode( int mode );

private:
	ITr2VisualizationModeRendererPtr m_object;
	int m_mode;
};

TYPEDEF_BLUECLASS( TriStepSetVisualizationMode );

#endif // TriStepSetVisualizationMode_h