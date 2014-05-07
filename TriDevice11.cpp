#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "TriDevice.h"
#include "UI/App.h"
#include "TriError.h"
#include "RenderJob/Tr2RenderJobs.h"


#include "TriSettingsRegistrar.h"
bool g_emulateDriverReset = false;
TRI_REGISTER_SETTING( "emulateDriverReset",		g_emulateDriverReset );

CCP_STATS_DECLARED_ELSEWHERE( presentTime );


void TriDevice::SetDefaultRenderStates() {}

void TriDevice::UpdateCursor() {}

void TriDevice::HandleRenderTick( Be::Time timestamp )
{
	if( !mDisplay )
	{
		return;
	}
	
	if( mDeviceLost )
	{
		ChangeDevice( mAdapter, mHwnd, nullptr );
		Tr2Renderer::SetIsDeviceResetting( mDeviceLost );
		return;
	}

	if( !Tr2RenderContext_GetMainThreadRenderContext().m_d3dDevice11 )
	{
		return;
	}

	static unsigned s_tickCounter = 0;
	if( (g_app && g_app->IsHidden()) )
	{
		//Update the game very occasionally, as things like missiles need to be handled
		// even if we're not actually rendering anything.
		++s_tickCounter	%= 100;
		if( s_tickCounter != 0 )
		{
			return;
		}
	}

	HRESULT hr = Tr2RenderContext_GetMainThreadRenderContext().m_d3dDevice11->GetDeviceRemovedReason();
	if( FAILED( hr ) || g_emulateDriverReset )
	{
		g_emulateDriverReset = false;
		if( !mDeviceLost )
		{
			CCP_LOGWARN( "DX11 device removed, reason: %x", hr );
			Tr2Renderer::SetIsDeviceResetting( true );
			DeviceLost();
			ReleaseDeviceResources( TRISTORAGE_ALL );

			mDeviceLost = true;
			Tr2RenderContext::DestroyMainThreadRenderContext();

			Tr2TrackedALObjectBase::LogAllLiveResources();
			return;
		}
	}
    
	if( mDeviceLost )
	{
		return;
	}

	if( m_renderJobs )
	{
		m_renderJobs->RunUpdate( timestamp );
	}

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
		REPORTERROR( "Failed to render a frame" );
	}
}

// -- Smaller helpers to enable big methods like TriDevice::Render to be mostly API neutral.

// Do we have a valid device pointer? Lower level question than "do we have a valid renderContext",
// so first question may be true while second one is still false.
bool TriDevice::DeviceExists()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return renderContext.m_d3dDevice11 != nullptr;
}

// Show or hide the cursor
void TriDevice::DoShowCursor( bool show ) {}

bool TriDevice::DoLowLevelDeviceReset( const Tr2PresentParametersAL& pp )
{
	return true;
}

bool TriDevice::DeviceSupportsVertexTexture()
{
	return true;
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
			if( renderContext.m_swapChain )
			{
				CR( renderContext.m_swapChain->SetFullscreenState( FALSE, nullptr ) );
			}
			ShowWindow( mHwnd, SW_MINIMIZE );
		}
	}
}

#endif
