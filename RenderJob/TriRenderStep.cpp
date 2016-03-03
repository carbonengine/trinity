#include "StdAfx.h"
#include "TriRenderStep.h"
#include "ITriRenderStep.h"

BLUE_DEFINE_INTERFACE( ITriRenderStep );

TriRenderStep::TriRenderStep( IRoot* lockobj )
:	m_enabled( true ),
	m_captureGpuTime( false ),
	m_captureCpuTime( false ),
	m_beginTime( 0 ),
	m_cpuTime( -1.f )
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
	}
	if( m_gpuTimer.IsValid() )
	{
		m_gpuTimer.End( renderContext );
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