#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

const char ERRNODEVICE[] = "There is no D3D device available";

#include "TriDevice.h"
#include "TriError.h"

#include "UI/App.h"

#include "RenderJob/Tr2RenderJobs.h"

using namespace Tr2RenderContextEnum;

#include "dxerr.h"

bool TriDevice::DoLowLevelDeviceReset( const Tr2PresentParametersAL& presentationParameters )
{
	D3DPRESENT_PARAMETERS pp;
	pp.BackBufferWidth = presentationParameters.mode.width;
	pp.BackBufferHeight = presentationParameters.mode.height;
	pp.BackBufferFormat = ConvertToD3D9Format( presentationParameters.mode.format );
	pp.BackBufferCount = presentationParameters.backBufferCount;
	pp.MultiSampleType = D3DMULTISAMPLE_TYPE( presentationParameters.msaaType );
    pp.MultiSampleQuality = presentationParameters.msaaQuality;
	pp.SwapEffect = presentationParameters.swapEffect == SWAP_EFFECT_DISCARD ? D3DSWAPEFFECT_DISCARD : D3DSWAPEFFECT_COPY;
	pp.hDeviceWindow = HWND( presentationParameters.outputWindow );
    pp.Windowed = presentationParameters.windowed ? TRUE : FALSE;
	pp.EnableAutoDepthStencil = presentationParameters.depthStencilFormat != DSFMT_UNKNOWN;
	pp.AutoDepthStencilFormat = ConvertToD3D9DepthStencilFormat( presentationParameters.depthStencilFormat );
    pp.Flags = 0;
	pp.FullScreen_RefreshRateInHz = presentationParameters.windowed ? 0 : presentationParameters.mode.refreshRateDenominator;
	switch( presentationParameters.presentInterval )
	{
	case PRESENT_INTERVAL_IMMEDIATE:
		pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		break;
	case PRESENT_INTERVAL_TWO:
		pp.PresentationInterval = D3DPRESENT_INTERVAL_TWO;
		break;
	case PRESENT_INTERVAL_THREE:
		pp.PresentationInterval = D3DPRESENT_INTERVAL_THREE;
		break;
	case PRESENT_INTERVAL_FOUR:
		pp.PresentationInterval = D3DPRESENT_INTERVAL_FOUR;
		break;
	default:
		pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();

	HRESULT hr = renderContext.m_d3dDevice9->Reset( &pp );
	if( FAILED( hr ) )
	{
		CCP_LOGERR( "Device reset attempt failed - %X: %s", hr, DXGetErrorString( hr ) );
		hr = renderContext.m_d3dDevice9->Reset( &pp );
	}
	if( FAILED( hr ) )
	{
		CCP_LOGERR( "Second device reset attempt failed - %X: %s", hr, DXGetErrorString( hr ) );

		// This does nothing except for in dev9
		Tr2TrackedALObjectBase::LogAllLiveResources( TRISTORAGE_VIDEOMEMORY );
		TriError::ReportError( hr, Clsid(), "Device Reset failed" );
		return false;
	}

	return true;
}

void TriDevice::SetDefaultRenderStates()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	renderContext.m_d3dDevice9->SetRenderState( D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE );
	renderContext.m_d3dDevice9->SetRenderState( D3DRS_LOCALVIEWER, TRUE );
	renderContext.m_d3dDevice9->SetRenderState( D3DRS_NORMALIZENORMALS, TRUE );
	renderContext.m_d3dDevice9->SetRenderState( D3DRS_DITHERENABLE, FALSE );	
	renderContext.m_d3dDevice9->SetRenderState( D3DRS_SPECULARENABLE, FALSE );
	renderContext.m_d3dDevice9->SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );	
}

void TriDevice::UpdateCursor()
{	
#if BLUE_WITH_PYTHON
	if (!mPresentParam.windowed)
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();

		static POINT opt = {-1,-1};
		POINT pt;		
		GetCursorPos(&pt);
		if((pt.x != opt.x) || (pt.y != opt.y))
		{
			opt = pt;
			if( renderContext.m_d3dDevice9 )
			{
				renderContext.m_d3dDevice9->SetCursorPosition(pt.x, pt.y, D3DCURSOR_IMMEDIATE_UPDATE);
			}
		}
	}
#endif
}

void TriDevice::HandleRenderTick( Be::Time timestamp )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !renderContext.m_d3dDevice9 || !mDisplay )
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

	HRESULT hr = renderContext.m_d3dDevice9->TestCooperativeLevel();
	switch( hr )
	{
	case E_DEVICELOST:
		if( !mDeviceLost )
		{
			DeviceLost();
			mDeviceLost = true;
			return; // There's no point in continuing along this function now...
		}
		break;

	case D3DERR_DEVICENOTRESET:
		// Device coming back from lost state.
		if( !ResetDevice( nullptr ) )
		{
			return; //big problem.  we are probably lost here.  
		}
		mDeviceLost = false;
		break;

	case D3DERR_DRIVERINTERNALERROR:
		TriError::ReportError(hr, Clsid(), "TestCooperativeLevel failed");
		ResetDevice( nullptr );
		return;

	default:
		if( FAILED( hr ) )
		{
			TriError::ReportError(hr, Clsid(), "TestCooperativeLevel failed");
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
		CCP_STATS_ZONE( "Present" );
		CR_RETURN( renderContext.Present() );
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
	return renderContext.m_d3dDevice9 != nullptr;
}

// Show or hide the cursor
void TriDevice::DoShowCursor( bool show )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( renderContext.m_d3dDevice9 )
	{
		renderContext.m_d3dDevice9->ShowCursor( show );
	}
}

bool TriDevice::DeviceSupportsVertexTexture()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return renderContext.GetCaps().SupportsVertexShaderTextures();
}

// --------------------------------------------------------------------------------------
// Description:
//   A chance for device to respond to application window being activated/deactivated. 
//   Not implemented for DX9 - we are happy with how windows behaves in DX9.
// Arguments:
//   activated - true if application was activated; false otherwise
// --------------------------------------------------------------------------------------
void TriDevice::ApplicationActivated( ApplicationActivation )
{
}

#endif
