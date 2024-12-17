#include "StdAfx.h"

#if TRINITY_PLATFORM==TRINITY_DIRECTX12

#include "TriDevice.h"
#include "RenderJob/Tr2RenderJobs.h"
#include "TriSettingsRegistrar.h"
#include <sstream>
#include <iomanip>


bool g_emulateDriverReset = false;
TRI_REGISTER_SETTING( "emulateDriverReset", g_emulateDriverReset );

CCP_STATS_DECLARED_ELSEWHERE( presentTime );
extern unsigned long long g_currentFrameCounter;

namespace
{

std::string GetDeviceRemovedReasonString( HRESULT hr )
{
	std::stringstream reason;
	switch( hr )
	{
	case DXGI_ERROR_DEVICE_HUNG:
		reason << "DXGI_ERROR_DEVICE_HUNG: ";
		break;
	case DXGI_ERROR_DEVICE_REMOVED:
		reason << "DXGI_ERROR_DEVICE_REMOVED: ";
		break;
	case DXGI_ERROR_DEVICE_RESET:
		reason << "DXGI_ERROR_DEVICE_RESET: ";
		break;
	case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
		reason << "DXGI_ERROR_DRIVER_INTERNAL_ERROR: ";
		break;
	case DXGI_ERROR_INVALID_CALL:
		reason << "DXGI_ERROR_INVALID_CALL: ";
		break;
	default:
		reason << "HRESULT: ";
		break;
	}
	reason << "0x" << std::setfill( '0' ) << std::setw( 8 ) << std::hex << (uint32_t)hr;
	return reason.str();
}

void NotifyDeviceRemoved( HRESULT hr, BlueScriptCallback& onDeviceRemoved )
{
	static uint32_t s_deviceLostCount = 0;
	++s_deviceLostCount;

	Tr2RenderContextEnum::RenderContextStatus status = Tr2RenderContextEnum::CONTEXT_STATUS_INVALID;
	std::string marker, markerMessage, offendingShader;
	if( SUCCEEDED( Tr2RenderContext_GetMainThreadRenderContext().GetGpuStateMarker( status, marker, offendingShader ) ) )
	{
		if( !marker.empty() )
		{
			markerMessage = ", GPU marker: " + marker;
		}
		if( !offendingShader.empty() )
		{
			markerMessage = ", shader: " + offendingShader;
		}
	}
	auto reason = GetDeviceRemovedReasonString( hr );

	if( onDeviceRemoved.IsValid() )
	{
		onDeviceRemoved.CallVoid( (uint64_t)(unsigned long)hr, reason, s_deviceLostCount, marker, "", offendingShader );
	}

	CCP_LOGERR( "DX12 device removed, reason: %s%s", reason.c_str(), markerMessage.c_str() );

	if( BeCrashes )
	{
		std::string str;
		BeCrashes->SetCrashKeyValue( "gpuRemovedCount", ( str = std::to_string( s_deviceLostCount ) ).c_str() );
		BeCrashes->SetCrashKeyValue( "gpuRemovedFrame", ( str = std::to_string( g_currentFrameCounter ) ).c_str() );
		BeCrashes->SetCrashKeyValue( "gpuRemovedReason", reason.c_str() );
		if( !marker.empty() )
		{
			BeCrashes->SetCrashKeyValue( "gpuRemovedMarker", marker.c_str() );
		}
	}
};

}


void TriDevice::HandleRenderTick( Be::Time realTime, Be::Time simTime )
{
#if BLUE_WITH_PYTHON
	AutoTasklet _at( PyOS->GetTaskletTimer(), "TriDevice::HandleRenderTick" );
#endif

	auto TearDownDevice = [&]() {
		ReleaseDeviceResources( TRISTORAGE_ALL );

		mDeviceLost = true;
		Tr2RenderContext::DestroyMainThreadRenderContext();

		LogAllLiveResources();
	};

	auto HandleLostDevice = [&]() {
		HRESULT hr = Tr2RenderContext_GetMainThreadRenderContext().m_device->GetDeviceRemovedReason();
		if( FAILED( hr ) || g_emulateDriverReset )
		{
			g_emulateDriverReset = false;
			if( !mDeviceLost )
			{
				NotifyDeviceRemoved( hr, m_onDeviceRemoved );
				TearDownDevice();
				return true;
			}
		}
		return false;
	};

	if( mDeviceLost )
	{
		if( !ChangeDevice( mAdapter, mHwnd, nullptr ) )
		{
			CCP_LOGERR( "Failed to create D3D device" );
		}

		return;
	}

	if( !Tr2RenderContext_GetMainThreadRenderContext().m_device )
	{
		return;
	}

	if( ShouldSkipFrame() )
	{
		Throttle();
		return;
	}
	Tr2RenderContext_GetMainThreadRenderContext().MarkFrameEvent( Tr2RenderContextEnum::FRAME_EVENT_UPDATE_STARTED );
	if( HandleLostDevice() )
	{
		return;
	}

	if( m_upscalingChanged )
	{
		CCP_LOGNOTICE( "DX12 device removed, reason: upscaler changed" );
		TearDownDevice();
		return;
	}

	if( mDeviceLost )
	{
		return;
	}

	if( m_renderJobs )
	{
		m_renderJobs->RunUpdate( realTime, simTime );
	}

	m_postUpdateCallbacks->Update();
	Tr2RenderContext_GetMainThreadRenderContext().MarkFrameEvent( Tr2RenderContextEnum::FRAME_EVENT_UPDATE_FINISHED );

	// Present the backbuffer from the last renderering to the front buffer.
	// it is more efficient to do it like this (revers order), because there is some
	// acyncrounicy between EndScene() and Present(). So if we pump Python
	// and do all the other stuff between we get a degree of paralization
	Tr2RenderContext_GetMainThreadRenderContext().MarkFrameEvent( Tr2RenderContextEnum::FRAME_EVENT_PRESENT_STARTED );
	{
		CCP_STATS_SCOPED_TIME( presentTime );
		CR_RETURN( Tr2RenderContext_GetMainThreadRenderContext().Present() );
	}
	if( HandleLostDevice() )
	{
		return;
	}
	Tr2RenderContext_GetMainThreadRenderContext().MarkFrameEvent( Tr2RenderContextEnum::FRAME_EVENT_PRESENT_FINISHED );

	Tr2RenderContext_GetMainThreadRenderContext().MarkFrameEvent( Tr2RenderContextEnum::FRAME_EVENT_RENDERING_STARTED );
	if( !Render() )
	{
		CCP_LOGERR( "Failed to render a frame" );
	}
	Tr2RenderContext_GetMainThreadRenderContext().MarkFrameEvent( Tr2RenderContextEnum::FRAME_EVENT_RENDERING_FINISHED );
}

// -- Smaller helpers to enable big methods like TriDevice::Render to be mostly API neutral.

// Do we have a valid device pointer? Lower level question than "do we have a valid renderContext",
// so first question may be true while second one is still false.
bool TriDevice::DeviceExists()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return renderContext.IsValid();
}

// --------------------------------------------------------------------------------------
// Description:
//   A chance for device to respond to application window being activated/deactivated. 
//   For DX11 platform we minimize fullscreen window when user alt-tabs from application
//   and restore it back when he returns. This makes DX11 window behavior being 
//   consistent with DX9.
// Arguments:
//   activated - true if application was activated; false otherwise
// --------------------------------------------------------------------------------------
void TriDevice::ApplicationActivated( ApplicationActivation activated )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !mPresentParam.windowed && mHwnd )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		if( activated == APP_ACTIVATED )
		{
#if !USE_BORDERLESS_WINDOW
			ChangeDevice( mAdapter, mHwnd, nullptr );
#endif
		}
		else
		{
			ShowWindow( mHwnd, SW_MINIMIZE );
#if !USE_BORDERLESS_WINDOW
			ChangeDevice( mAdapter, mHwnd, nullptr );
#endif
		}
	}
}

#endif
