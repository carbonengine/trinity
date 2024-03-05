#include "StdAfx.h"
#include "TriStepRenderSceneDebug.h"



TriStepRenderSceneDebug::TriStepRenderSceneDebug( IRoot* lockobj )
{
}

TriStepRenderSceneDebug::~TriStepRenderSceneDebug(void)
{
}

TriStepResult TriStepRenderSceneDebug::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( m_scene )
	{
		m_scene->RenderDebugInfo( renderContext );
	}
	return RS_OK;
}

void TriStepRenderSceneDebug::py__init__( ITr2Scene* scene )
{
	m_scene = scene;
}
