// Copyright © 2011 CCP ehf.

#include "StdAfx.h"

#include "TriStepToggleCubemap.h"
#include "ITr2VisualizationModeRenderer.h"
#include "Interior/Tr2InteriorScene.h"

TriStepToggleCubemap::TriStepToggleCubemap( IRoot* lockobj ) :
	m_showCubemap( true ), m_scene( NULL )
{
}

TriStepToggleCubemap::~TriStepToggleCubemap( void )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer.
// --------------------------------------------------------------------------------------
void TriStepToggleCubemap::py__init__( Be::OptionalWithDefaultValue<bool, true> showCubemap, Tr2InteriorScene* scene )
{
	m_showCubemap = showCubemap;
	m_scene = scene;
}

// -------------------------------------------------------------
// Description:
//   Implements TriRenderStep method for executing a render step.
//	 Enables or disables the background cubemap
// Arguments:
//   time - Current system time (not used)
// Return value:
//   RS_OK always.
// -------------------------------------------------------------
TriStepResult TriStepToggleCubemap::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( this->m_scene )
	{
		this->m_scene->SetRenderBackgroundCubeMap( m_showCubemap );
	}

	return RS_OK;
}
