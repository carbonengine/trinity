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
:	m_enabled( true )
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
	if( m_name.empty() )
	{
		if( ClassType() && ClassType()->mClassId )
		{
			renderContext.AddGpuMarker( ClassType()->mClassId->GetName() );
		}
	}
	else
	{
		renderContext.AddGpuMarker( m_name.c_str() );
	}
	m_timer.Begin( renderContext );
}

void TriRenderStep::EndExecute( Tr2RenderContext& renderContext )
{
	m_timer.End( renderContext );
}

bool TriRenderStep::GetCaptureGpuTime() const
{
	return m_timer.GetCaptureGpuTime();
}

void TriRenderStep::SetCaptureGpuTime( bool capture )
{
	m_timer.SetCaptureGpuTime( capture );
}

bool TriRenderStep::GetCaptureCpuTime() const
{
	return m_timer.GetCaptureCpuTime();
}

void TriRenderStep::SetCaptureCpuTime( bool capture )
{
	m_timer.SetCaptureCpuTime( capture );
}

float TriRenderStep::GpuTime() const
{
	return m_timer.GpuTime();
}

float TriRenderStep::CpuTime() const
{
	return m_timer.CpuTime();
}

const std::string& TriRenderStep::GetStatName() const
{
	return m_timer.GetStatName();
}

void TriRenderStep::SetStatName( const char* name )
{
	m_timer.SetStatName( name );
}
