// Copyright © 2023 CCP ehf.

#pragma once
#ifndef ITr2VisualizationModeRenderer_h
#define ITr2VisualizationModeRenderer_h

// -------------------------------------------------------------
// Description:
//   Objects implementing ITr2VisualizationModeRenderer are
//	 supposed to have an ability to render using different
//   debug visualization modes.
// SeeAlso:
//   TriStepSetVisualizationMode
// -------------------------------------------------------------
BLUE_INTERFACE( ITr2VisualizationModeRenderer ) :
	public IRoot
{
	// -------------------------------------------------------------
	// Description:
	//   Sets the new visualization mode for the object.
	// Arguments:
	//   visualizationMode - new visualization mode (semantics of
	//                       modes is implementation-specific).
	// -------------------------------------------------------------
	virtual void SetVisualizationMode( int visualizationMode ) = 0;
};

TYPEDEF_BLUECLASS( ITr2VisualizationModeRenderer );

#endif // ITr2VisualizationModeRenderer_h
