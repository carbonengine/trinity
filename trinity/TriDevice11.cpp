#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "TriDevice.h"
#include "RenderJob/Tr2RenderJobs.h"


#include "TriSettingsRegistrar.h"
bool g_emulateDriverReset = false;
bool g_fixFullscreenBehaviorForOldWindows = false;
TRI_REGISTER_SETTING( "emulateDriverReset",		g_emulateDriverReset );
TRI_REGISTER_SETTING( "fixFullscreenBehaviorForOldWindows", g_fixFullscreenBehaviorForOldWindows );

CCP_STATS_DECLARED_ELSEWHERE( presentTime );


void TriDevice::HandleRenderTick(  Be::Time realTime, Be::Time simTime )
{
#if BLUE_WITH_PYTHON
	AutoTasklet _at( PyOS->GetTaskletTimer(), "TriDevice::HandleRenderTick" );
#endif

	if( mDeviceLost )
	{
		ChangeDevice( mAdapter, mHwnd, nullptr );
		return;
	}

	if( !Tr2RenderContext_GetMainThreadRenderContext().m_d3dDevice11 )
	{
		return;
	}

	if( ShouldSkipFrame() )
	{
		Throttle();
		return;
	}

	HRESULT hr = Tr2RenderContext_GetMainThreadRenderContext().m_d3dDevice11->GetDeviceRemovedReason();
	if( FAILED( hr ) || g_emulateDriverReset )
	{
		g_emulateDriverReset = false;
		if( !mDeviceLost )
		{
			static uint32_t s_deviceLostCount = 0;
			++s_deviceLostCount;

			Tr2RenderContextEnum::RenderContextStatus status = Tr2RenderContextEnum::CONTEXT_STATUS_INVALID;
			std::string marker, markerMessage;
			if( SUCCEEDED( Tr2RenderContext_GetMainThreadRenderContext().GetGpuStateMarker( status, marker ) ) )
			{
				markerMessage = ", GPU marker: " + marker;
			}

			Tr2RenderContextEnum::PixelFormat format;
			uint64_t size;
			uint32_t width;
			uint32_t height;
			uint32_t depth;
			uint32_t mips;
			bool hasPageFault = false;
			char resourceDesc[256] = { 0 };
			if( SUCCEEDED( Tr2RenderContext_GetMainThreadRenderContext().GetGpuPageFaultResource( format, size, width, height, depth, mips ) ) )
			{
				sprintf_s( resourceDesc, "fmt %i, size: %u, %ux%ux%u, mips: %u", int( format ), unsigned( size ), width, height, depth, mips );
				markerMessage += ", Page Fault: ";
				markerMessage += resourceDesc;
				hasPageFault = true;
			}

			if( m_onDeviceRemoved.IsValid() )
			{
				m_onDeviceRemoved.CallVoid( (uint64_t)(unsigned long)hr, BeGetErrorMessage( ALResult( hr ) ), s_deviceLostCount, marker, resourceDesc, "" );
			}

			CCP_LOGERR( "DX11 device removed, reason: 0x%x%s", hr, markerMessage.c_str() );

			if( BeCrashes )
			{
				extern unsigned long long g_currentFrameCounter;

				std::string str;
				BeCrashes->SetCrashKeyValue( "gpuRemovedCount", ( str = std::to_string( s_deviceLostCount ) ).c_str() );
				BeCrashes->SetCrashKeyValue( "gpuRemovedFrame", ( str = std::to_string( g_currentFrameCounter ) ).c_str() );
				BeCrashes->SetCrashKeyValue( "gpuRemovedReason", ( str = std::to_string( hr ) ).c_str() );
				if( !marker.empty() )
				{
					BeCrashes->SetCrashKeyValue( "gpuRemovedMarker", marker.c_str() );
				}
				if( hasPageFault )
				{
					BeCrashes->SetCrashKeyValue( "gpuPageFaultResource", resourceDesc );
				}
			}
			ReleaseDeviceResources( TRISTORAGE_ALL );

			mDeviceLost = true;
			Tr2RenderContext::DestroyMainThreadRenderContext();

			LogAllLiveResources();
			return;
		}
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

	// Present the backbuffer from the last renderering to the front buffer.
	// it is more efficient to do it like this (revers order), because there is some
	// acyncrounicy between EndScene() and Present(). So if we pump Python
	// and do all the other stuff between we get a degree of paralization
	{
		CCP_STATS_SCOPED_TIME( presentTime );
		CR_RETURN( Tr2RenderContext_GetMainThreadRenderContext().Present() );
	}
		
	if( !Render() )
	{
		CCP_LOGERR( "Failed to render a frame" );
	}
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
			if( renderContext.m_swapChain )
			{
				if( SUCCEEDED( renderContext.m_swapChain->SetFullscreenState( TRUE, nullptr ) ) )
				{
					return;
				}
			}
			CCP_LOGWARN( "TriDevice: SetFullscreenState failed when activating app. Trying to reset device." );
			ChangeDevice( mAdapter, mHwnd, nullptr );
		}
		else
		{
#if defined( WIN32 )
			if( g_fixFullscreenBehaviorForOldWindows && renderContext.m_swapChain )
			{
				CR( renderContext.m_swapChain->SetFullscreenState( FALSE, nullptr ) );
			}
#endif
			ShowWindow( mHwnd, SW_MINIMIZE );
		}
	}
}

#endif
