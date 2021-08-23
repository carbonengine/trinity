#include "StdAfx.h"
#include "../include/Tr2GpuTimerAL.h"

#include TRINITY_AL_PLATFORM_INCLUDE( Tr2GpuTimerAL )

bool g_gpuTimersEnabled = true;

namespace
{
	auto nullGpuTimer = std::make_shared<TrinityALImpl::Tr2GpuTimerAL>();
}

Tr2GpuTimerAL::Tr2GpuTimerAL()
	:m_gpu_timer(nullGpuTimer)
{
}

ALResult Tr2GpuTimerAL::Create( Tr2PrimaryRenderContextAL& renderContext )
{
	if( !g_gpuTimersEnabled )
	{
		m_gpu_timer = nullGpuTimer;
		return E_FAIL;
	}
	
	m_gpu_timer = std::make_shared<TrinityALImpl::Tr2GpuTimerAL>();
	auto hr = m_gpu_timer->Create(renderContext);
	if( FAILED( hr))
	{
		m_gpu_timer = nullGpuTimer;
	}
	return hr;
}


bool Tr2GpuTimerAL::Begin( Tr2RenderContextAL& renderContext ) const
{
	return m_gpu_timer->Begin(renderContext);
}

void Tr2GpuTimerAL::End( Tr2RenderContextAL& renderContext ) const
{
	m_gpu_timer->End(renderContext);
}

float Tr2GpuTimerAL::GetTime( Tr2RenderContextAL& renderContext ) const
{
	return m_gpu_timer->GetTime(renderContext);
}

bool Tr2GpuTimerAL::IsValid() const
{
	return m_gpu_timer->IsValid();
}

bool Tr2GpuTimerAL::operator==( const Tr2GpuTimerAL& other ) const
{
	return m_gpu_timer == other.m_gpu_timer;
}

Tr2ALMemoryType Tr2GpuTimerAL::GetMemoryClass() const
{
	return m_gpu_timer->GetMemoryClass();
}
