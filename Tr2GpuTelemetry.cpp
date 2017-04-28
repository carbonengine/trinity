#include "StdAfx.h"
#include "Tr2GpuTelemetry.h"

bool g_gpuTelemetryEnabled = false;

#if 0 && CCP_TELEMETRY_ENABLED && ( TRINITY_PLATFORM == TRINITY_DIRECTX9 || TRINITY_PLATFORM == TRINITY_DIRECTX11 )

#include "tmxgpu.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX9
#include "gpu\gpu_d3d9.cpp"
#elif TRINITY_PLATFORM == TRINITY_DIRECTX11
#include "gpu\gpu_d3d11.cpp"
#endif

namespace
{

TmxGpuContext* s_telemetryContext = nullptr;

void OnTelemetryEvent( CcpTelemetryEvent event, void* userData )
{
	if( s_telemetryContext )
	{
		tmxGpuReset( s_telemetryContext );
		CCP_DELETE s_telemetryContext;
		s_telemetryContext = nullptr;
	}
	if( !g_gpuTelemetryEnabled )
	{
		return;
	}
	auto context = Tr2RenderContextAL::GetPrimaryRenderContextPointer();
	if( !context )
	{
		return;
	}
	if( event == CCP_TELEMETRY_STARTED )
	{
#if TRINITY_PLATFORM == TRINITY_DIRECTX9
		auto platformContext = context->m_d3dDevice9;
		auto init = &tmxGpuInitD3D9;
#elif TRINITY_PLATFORM == TRINITY_DIRECTX11
		auto platformContext = context->m_context;
		auto init = &tmxGpuInitD3D11;
#endif

		if( context && platformContext )
		{
			tmZone( g_telemetryContext, TMZF_NONE, "Tr2GpuTelemetry::OnTelemetryEvent" );
			s_telemetryContext = CCP_NEW( "Tr2GpuTelemetry/s_telemetryContext" ) TmxGpuContext;

			if( ( *init )( s_telemetryContext, g_telemetryContext, platformContext, TMXGPU_INIT_QUERIES, 32, nullptr ) != TMX_OK )
			{
				CCP_DELETE s_telemetryContext;
				s_telemetryContext = nullptr;
			}
		}
	}
}
}

void Tr2GpuTelemetryDeviceCreated()
{
	static bool s_hasRegistered = false;
	if( !s_hasRegistered )
	{
		CcpRegisterTelemetryEventHandler( &OnTelemetryEvent, nullptr );
		s_hasRegistered = true;
	}
}

void Tr2GpuTelemetryDeviceReset()
{
	if( s_telemetryContext )
	{
		tmxGpuReset( s_telemetryContext );
		CCP_DELETE s_telemetryContext;
		s_telemetryContext = nullptr;
	}
}

void Tr2GpuTelemetryTick()
{
	if( s_telemetryContext )
	{
		tmxGpuTick( s_telemetryContext );
	}
}

void Tr2GpuTelemetryEnterZone( const char* name )
{
	if( s_telemetryContext )
	{
		tmxGpuDrawCallBegin( s_telemetryContext, "%s (frame %u)", name, CcpTelemetryGetTickCount() );
	}
}

void Tr2GpuTelemetryLeaveZone()
{
	if( s_telemetryContext )
	{
		tmxGpuDrawCallEnd( s_telemetryContext );
	}
}

#else

void Tr2GpuTelemetryDeviceCreated()
{
}

void Tr2GpuTelemetryDeviceReset()
{
}

void Tr2GpuTelemetryTick()
{
}

void Tr2GpuTelemetryEnterZone( const char* )
{
}

void Tr2GpuTelemetryLeaveZone()
{
}

#endif
