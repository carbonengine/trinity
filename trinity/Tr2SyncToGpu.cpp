// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2SyncToGpu.h"


Tr2SyncToGpu& Tr2SyncToGpu::GetInstance()
{
	static Tr2SyncToGpu instance;
	return instance;
}

void Tr2SyncToGpu::Tick()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	uint64_t completed;
	m_frame = renderContext.GetRecordingFrameNumber();
	completed = renderContext.GetRenderedFrameNumber();
	for( auto it = begin( m_tasks ); it != end( m_tasks ); ++it )
	{
		if( it->frame > completed )
		{
			m_tasks.erase( begin( m_tasks ), it );
			return;
		}
		it->task();
	}
	m_tasks.clear();
}

void Tr2SyncToGpu::Flush()
{
	for( auto& task : m_tasks )
	{
		task.task();
	}
	m_tasks.clear();
}
