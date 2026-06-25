// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "TriStepSetDebugRenderer.h"
#include "include/ITr2DebugRenderer.h"

TriStepSetDebugRenderer::TriStepSetDebugRenderer( IRoot* lockobj )
{
}

TriStepSetDebugRenderer::~TriStepSetDebugRenderer( void )
{
}

// -------------------------------------------------------------
// Description:
//   Implements TriRenderStep method for executing a render step.
//	 Assigns stored debug renerer to the global debug renderer
//   variable.
// Arguments:
//   time - Current system time (not used)
// Return value:
//   RS_OK always.
// -------------------------------------------------------------
TriStepResult TriStepSetDebugRenderer::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	extern ITr2DebugRendererPtr g_debugRenderer;
	g_debugRenderer = m_debugRenderer;
	return RS_OK;
}

// -------------------------------------------------------------
// Description:
//   Sets the debug renderer to assign to the global debug renderer
//   variable.
// Arguments:
//   renderer - Debug renderer to set
// -------------------------------------------------------------
void TriStepSetDebugRenderer::SetDebugRenderer( ITr2DebugRenderer* renderer )
{
	m_debugRenderer = renderer;
}
