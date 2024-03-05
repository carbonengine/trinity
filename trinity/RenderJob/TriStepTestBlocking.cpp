#include "StdAfx.h"
#include "TriStepTestBlocking.h"

TriStepTestBlocking::TriStepTestBlocking( IRoot* lockobj ) :
	m_inProgress( true )
{

}

TriStepTestBlocking::~TriStepTestBlocking(void)
{
}

TriStepResult TriStepTestBlocking::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( m_inProgress )
	{
		CCP_LOG( "%s in progress", m_name.c_str() );
		return RS_IN_PROGRESS;
	}
	else
	{
		CCP_LOG( "%s done!", m_name.c_str() );
		return RS_OK;
	}
}
