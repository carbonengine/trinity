// Copyright © 2017 CCP ehf.

#include "StdAfx.h"
#include "Tr2ProfileTimer.h"

namespace
{
CcpStaticStatisticsEntry* GetOrCreateStatisticsEntry( const std::string& name )
{
	auto& entries = CcpStatistics::GetEntryArray();
	for( auto it = entries.begin(); it != entries.end(); ++it )
	{
		if( ( *it )->GetName() == name )
		{
			return *it;
		}
	}
	return BlueStatistics::CreateDynamicEntry( name.c_str(), false, CST_TIME, "" );
}
}

// -------------------------------------------------------------
Tr2ProfileTimer::Tr2ProfileTimer() :
	m_captureGpuTime( false ),
	m_captureCpuTime( false ),
	m_beginTime( 0 ),
	m_cpuTime( -1.f ),
	m_statEntryCpu( nullptr ),
	m_statEntryGpu( nullptr )
{
}

// -------------------------------------------------------------
void Tr2ProfileTimer::ReleaseResources( TriStorage s )
{
	if( m_gpuTimer.GetMemoryClass() & s )
	{
		m_gpuTimer = Tr2GpuTimerAL();
	}
	m_gpuTimer = Tr2GpuTimerAL();
}

// -------------------------------------------------------------
bool Tr2ProfileTimer::OnPrepareResources()
{
	if( m_captureGpuTime && !m_gpuTimer.IsValid() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		m_gpuTimer.Create( renderContext ).GetResult();
	}
	return true;
}

// -------------------------------------------------------------
void Tr2ProfileTimer::Begin( Tr2RenderContext& renderContext )
{
	if( m_gpuTimer.IsValid() )
	{
		m_gpuTimer.Begin( renderContext );
	}
	if( m_captureCpuTime )
	{
		m_beginTime = CcpGetTimestamp();
	}
}

// -------------------------------------------------------------
void Tr2ProfileTimer::End( Tr2RenderContext& renderContext )
{
	if( m_captureCpuTime )
	{
		auto endTime = CcpGetTimestamp();
		m_cpuTime = float( double( endTime - m_beginTime ) / CcpGetTimestampFrequency() );
		if( !m_statEntryCpu )
		{
			if( !m_statName.empty() )
			{
				m_statEntryCpu = GetOrCreateStatisticsEntry( m_statName + "/cpuTime" );
			}
		}
		else if( m_statName.empty() )
		{
			m_statEntryCpu = nullptr;
		}
		else
		{
			m_statEntryCpu->Set( double( m_cpuTime ) );
		}
	}
	else if( m_statEntryCpu )
	{
		m_statEntryCpu = nullptr;
	}

	if( m_gpuTimer.IsValid() )
	{
		m_gpuTimer.End( renderContext );
		if( !m_statEntryGpu )
		{
			if( !m_statName.empty() )
			{
				m_statEntryGpu = GetOrCreateStatisticsEntry( m_statName + "/gpuTime" );
			}
		}
		else if( m_statName.empty() )
		{
			m_statEntryGpu = nullptr;
		}
		else
		{
			m_statEntryGpu->Set( double( std::max( 0.f, m_gpuTimer.GetTime( renderContext ) ) ) );
		}
	}
	else if( m_statEntryGpu )
	{
		m_statEntryGpu = nullptr;
	}
}

// -------------------------------------------------------------
bool Tr2ProfileTimer::GetCaptureGpuTime() const
{
	return m_captureGpuTime;
}

// -------------------------------------------------------------
void Tr2ProfileTimer::SetCaptureGpuTime( bool capture )
{
	if( m_captureGpuTime == capture )
	{
		return;
	}
	m_captureGpuTime = capture;
	if( capture )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		m_gpuTimer.Create( renderContext ).GetResult();
	}
	else
	{
		m_gpuTimer = Tr2GpuTimerAL();
	}
}

// -------------------------------------------------------------
bool Tr2ProfileTimer::GetCaptureCpuTime() const
{
	return m_captureCpuTime;
}

// -------------------------------------------------------------
void Tr2ProfileTimer::SetCaptureCpuTime( bool capture )
{
	m_captureCpuTime = capture;
}

// -------------------------------------------------------------
float Tr2ProfileTimer::GpuTime() const
{
	if( m_gpuTimer.IsValid() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		return m_gpuTimer.GetTime( renderContext ) * 1000.f;
	}
	return -1.f;
}

// -------------------------------------------------------------
float Tr2ProfileTimer::CpuTime() const
{
	if( m_captureCpuTime )
	{
		return m_cpuTime * 1000.f;
	}
	return -1.f;
}

// -------------------------------------------------------------
const std::string& Tr2ProfileTimer::GetStatName() const
{
	return m_statName;
}

// -------------------------------------------------------------
void Tr2ProfileTimer::SetStatName( const char* name )
{
	m_statName = name;
}
