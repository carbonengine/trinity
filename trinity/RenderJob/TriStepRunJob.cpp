#include "StdAfx.h"
#include "TriStepRunJob.h"
#include "TriRenderJob.h"

TriStepRunJob::TriStepRunJob( IRoot* lockobj )
{

}

TriStepRunJob::~TriStepRunJob(void)
{
}

TriStepResult TriStepRunJob::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( m_name.c_str() );

	if( !m_job )
	{
		return RS_OK;
	}

	TriRenderJobStatus status = m_job->Run( realTime, simTime );

	switch( status )
	{
		case RJ_DONE:
			return RS_OK;

		case RJ_IN_PROGRESS:
			return RS_IN_PROGRESS;

		case RJ_FAILED:
			return RS_FAILED;

		default:
			CCP_LOGERR( "TriStepRunJob::Execute: Unexpected return value from running render job (%d)", status );
			return RS_FAILED;
	}
}

void TriStepRunJob::SetRenderJob( TriRenderJob* job )
{
	m_job = job;
}