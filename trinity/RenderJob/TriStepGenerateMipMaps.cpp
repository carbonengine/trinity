#include "StdAfx.h"
#include "TriStepGenerateMipMaps.h"


TriStepGenerateMipMaps::TriStepGenerateMipMaps( IRoot* lockobj )
{
}

TriStepGenerateMipMaps::~TriStepGenerateMipMaps(void)
{
}

TriStepResult TriStepGenerateMipMaps::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( m_renderTarget )
	{
		m_renderTarget->GetRenderTarget().GenerateMipMaps( renderContext );
	}
	return RS_OK;
}

void TriStepGenerateMipMaps::py__init__( Tr2RenderTarget* rt )
{
	m_renderTarget = rt;
}
