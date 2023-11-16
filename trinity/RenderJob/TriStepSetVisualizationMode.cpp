////////////////////////////////////////////////////////////
//
//    Created:   August 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"
#include "TriStepSetVisualizationMode.h"
#include "ITr2VisualizationModeRenderer.h"

TriStepSetVisualizationMode::TriStepSetVisualizationMode( IRoot* lockobj )
:	m_mode( 0 )
{
}

TriStepSetVisualizationMode::~TriStepSetVisualizationMode(void)
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer. 
// --------------------------------------------------------------------------------------
void TriStepSetVisualizationMode::py__init__( ITr2VisualizationModeRenderer* object, int mode )
{
	SetObject( object );
	SetVisualizationMode( mode );
}

// -------------------------------------------------------------
// Description:
//   Implements TriRenderStep method for executing a render step.
//	 Sets scene's debug visualization mode.
// Arguments:
//   time - Current system time (not used)
// Return value:
//   RS_OK always.
// -------------------------------------------------------------
TriStepResult TriStepSetVisualizationMode::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( m_object )
	{
		m_object->SetVisualizationMode( m_mode );
	}
	return RS_OK;
}

// -------------------------------------------------------------
// Description:
//   Sets the object that needs its debug visualization mode changed.
// Arguments:
//   scene - Interior scene
// -------------------------------------------------------------
void TriStepSetVisualizationMode::SetObject( ITr2VisualizationModeRenderer* object )
{
	m_object = object;
}

// -------------------------------------------------------------
// Description:
//   Sets the new visuzaliation mode for the object.
// Arguments:
//   mode - Visualization mode
// -------------------------------------------------------------
void TriStepSetVisualizationMode::SetVisualizationMode( int mode )
{
	m_mode = mode;
}
