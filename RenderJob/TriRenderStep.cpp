#include "StdAfx.h"
#include "TriRenderStep.h"
#include "ITriRenderStep.h"

BLUE_DEFINE_INTERFACE( ITriRenderStep );

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

TriRenderStep::TriRenderStep( IRoot* lockobj )
:	m_enabled( true ),
	m_captureGpuTime( false ),
	m_captureCpuTime( false ),
	m_beginTime( 0 ),
	m_cpuTime( -1.f ),
	m_statEntryCpu( nullptr ),
	m_statEntryGpu( nullptr )
{
}

TriRenderStep::~TriRenderStep()
{
}

bool TriRenderStep::IsEnabled() const
{
	return m_enabled;
}

void TriRenderStep::BeginExecute( Tr2RenderContext& renderContext )
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

void TriRenderStep::EndExecute( Tr2RenderContext& renderContext )
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

bool TriRenderStep::GetCaptureGpuTime() const
{
	return m_captureGpuTime;
}

void TriRenderStep::SetCaptureGpuTime( bool capture )
{
	if( m_captureGpuTime == capture )
	{
		return;
	}
	m_captureGpuTime = capture;
	if( capture )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		m_gpuTimer.Create( renderContext );
	}
	else
	{
		m_gpuTimer.Destroy();
	}
}

float TriRenderStep::GpuTime() const
{
	if( m_gpuTimer.IsValid() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		return m_gpuTimer.GetTime( renderContext ) * 1000.f;
	}
	return -1.f;
}

float TriRenderStep::CpuTime() const
{
	if( m_captureCpuTime )
	{
		return m_cpuTime * 1000.f;
	}
	return -1.f;
}