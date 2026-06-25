// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2RenderJobs.h"
#include "TriRenderJob.h"
#include "Tr2PushPopDS.h"
#include "Tr2PushPopRT.h"

// Leave the 'device' in the name for compatibility
CCP_STATS_DECLARE( deviceRenderJobs, "Trinity/device/RenderJobs", true, CST_TIME, "Time taken for render job processing" );
CCP_STATS_DECLARE( deviceChainedRenderJobsCount, "Trinity/device/ChainedRenderJobsCount", true, CST_COUNTER_LOW, "Count of scheduled chained render jobs" );
CCP_STATS_DECLARE( deviceOnceRenderJobsCount, "Trinity/device/OnceRenderJobsCount", true, CST_COUNTER_LOW, "Count of scheduled once render jobs" );

Tr2RenderJobs::Tr2RenderJobs( IRoot* lockobj ) :
	PARENTLOCK( m_scheduledOnce ), PARENTLOCK( m_scheduledRecurring ), PARENTLOCK( m_scheduledChained ), PARENTLOCK( m_updateRecurring )
{
}

Tr2RenderJobs::~Tr2RenderJobs()
{
}

void Tr2RenderJobs::Run( Be::Time realTime, Be::Time simTime )
{
	CCP_STATS_SCOPED_TIME( deviceRenderJobs );
	CCP_STATS_SET( deviceChainedRenderJobsCount, m_scheduledChained.size() );
	CCP_STATS_SET( deviceOnceRenderJobsCount, m_scheduledOnce.size() );

	D3DPERF_EVENT( L"RenderJobs" );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	std::vector<TriRenderJobPtr> copyOfJobs;

	Tr2PushPopRT pushPopRT( renderContext );
	Tr2PushPopDS pushPopDS( renderContext );

	copyOfJobs.insert( copyOfJobs.end(), m_scheduledRecurring.begin(), m_scheduledRecurring.end() );

	for( auto it = copyOfJobs.cbegin(); it != copyOfJobs.cend(); ++it )
	{
		TriRenderJob* rj = *it;
		TriRenderJobStatus status = rj->Run( realTime, simTime );
		CCP_ASSERT( status != RJ_FAILED );
	}

	// Process jobs scheduled for one-off execution. Every job on this list is run,
	// jobs that are still in progress are continued next frame.
	copyOfJobs.clear();
	copyOfJobs.insert( copyOfJobs.end(), m_scheduledOnce.begin(), m_scheduledOnce.end() );

	CTriRenderJobVector continuedJobs;
	for( auto it = copyOfJobs.cbegin(); it != copyOfJobs.cend(); ++it )
	{
		TriRenderJob* rj = *it;
		TriRenderJobStatus status = rj->Run( realTime, simTime );
		if( status == RJ_IN_PROGRESS )
		{
			continuedJobs.Insert( -1, rj );
		}
	}
	m_scheduledOnce.Remove( -1 );

	for( auto it = continuedJobs.cbegin(); it != continuedJobs.cend(); ++it )
	{
		TriRenderJob* rj = *it;
		m_scheduledOnce.Insert( -1, rj );
	}
	continuedJobs.Remove( -1 );

	// Process jobs scheduled for chained one-off execution. Jobs on this list are run
	// until a job is found still in progress. That job and the remaining jobs are then
	// continued on the next frame.
	copyOfJobs.clear();
	copyOfJobs.insert( copyOfJobs.end(), m_scheduledChained.begin(), m_scheduledChained.end() );
	for( auto it = copyOfJobs.cbegin(); it != copyOfJobs.cend(); ++it )
	{
		TriRenderJob* rj = *it;
		TriRenderJobStatus status = rj->Run( realTime, simTime );
		if( status == RJ_IN_PROGRESS )
		{
			for( ; it != copyOfJobs.cend(); ++it )
			{
				rj = *it;
				continuedJobs.Insert( -1, rj );
			}
			break;
		}
	}
	m_scheduledChained.Remove( -1 );

	for( auto it = continuedJobs.cbegin(); it != continuedJobs.cend(); ++it )
	{
		TriRenderJob* rj = *it;
		m_scheduledChained.Insert( -1, rj );
	}
	continuedJobs.Remove( -1 );
}

void Tr2RenderJobs::RunUpdate( Be::Time realTime, Be::Time simTime )
{
	std::vector<TriRenderJobPtr> copyOfJobs;
	copyOfJobs.insert( copyOfJobs.end(), m_updateRecurring.begin(), m_updateRecurring.end() );

	for( auto it = copyOfJobs.cbegin(); it != copyOfJobs.cend(); ++it )
	{
		TriRenderJob* rj = *it;
		TriRenderJobStatus status = rj->Run( realTime, simTime );
		CCP_ASSERT( status != RJ_FAILED );
	}
}