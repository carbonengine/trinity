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
#if TRINITY_PLATFORM == TRINITY_DIRECTX12
	m_frame = renderContext.GetCurrentFrameIndexDx12();
	completed = renderContext.GetCompletedFrameIndexDx12();
#elif TRINITY_PLATFORM == TRINITY_METAL
	m_frame = renderContext.GetMetalContext()->GetRecordingFrameNumber();
	completed = renderContext.GetMetalContext()->GetRenderedFrameNumber();
#elif TRINITY_PLATFORM == TRINITY_DIRECTX11 || TRINITY_PLATFORM == TRINITY_STUB
	// Something imaginary here: we don't do any frame accounting for dx11
	++m_frame;
	if( m_frame > 3 )
	{
		completed = m_frame - 3;
	}
#else
#error "Missing implementation"
#endif
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
