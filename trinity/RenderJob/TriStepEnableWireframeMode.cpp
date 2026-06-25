// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "TriStepEnableWireframeMode.h"
#include "ITr2VisualizationModeRenderer.h"

TriStepEnableWireframeMode::TriStepEnableWireframeMode( IRoot* lockobj ) :
	m_enableWireframe( false )
{
}

TriStepEnableWireframeMode::~TriStepEnableWireframeMode( void )
{
}

// -------------------------------------------------------------
// Description:
//   Implements TriRenderStep method for executing a render step.
//	 Sets scene's wireframe mode
// Arguments:
//   time - Current system time (not used)
// Return value:
//   RS_OK always.
// -------------------------------------------------------------
TriStepResult TriStepEnableWireframeMode::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	renderContext.m_esm.SetWireframeRendering( m_enableWireframe );
	return RS_OK;
}

void TriStepEnableWireframeMode::py__init__( bool value )
{
	m_enableWireframe = value;
}
