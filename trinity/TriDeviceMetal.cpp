// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if ( TRINITY_PLATFORM == TRINITY_METAL )

#include "TriDevice.h"

#include "RenderJob/Tr2RenderJobs.h"

using namespace Tr2RenderContextEnum;

CCP_STATS_DECLARED_ELSEWHERE( presentTime );


void TriDevice::HandleRenderTick( Be::Time realTime, Be::Time simTime )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !renderContext.IsValid() )
	{
		return;
	}

	if( ShouldSkipFrame() )
	{
		Throttle();
		return;
	}

	if( m_upscalingChanged )
	{
		CCP_LOGNOTICE( "Resetting device - Upscaler changed" );
		CreateUpscalingTechnique( mAdapter );
		ResetDevice();
		return;
	}

	if( mDeviceLost )
	{
		return;
	}
	Tr2RenderContext_GetMainThreadRenderContext().MarkFrameEvent( Tr2RenderContextEnum::FRAME_EVENT_UPDATE_STARTED );

	if( m_renderJobs )
	{
		m_renderJobs->RunUpdate( realTime, simTime );
	}
	Tr2RenderContext_GetMainThreadRenderContext().MarkFrameEvent( Tr2RenderContextEnum::FRAME_EVENT_UPDATE_FINISHED );

	m_postUpdateCallbacks->Update();

	{
		Tr2RenderContext_GetMainThreadRenderContext().MarkFrameEvent( Tr2RenderContextEnum::FRAME_EVENT_PRESENT_STARTED );
		CCP_STATS_SCOPED_TIME( presentTime );
		CR_RETURN( renderContext.Present() );
		Tr2RenderContext_GetMainThreadRenderContext().MarkFrameEvent( Tr2RenderContextEnum::FRAME_EVENT_PRESENT_FINISHED );
	}

	renderContext.MarkFrameEvent( Tr2RenderContextEnum::FRAME_EVENT_RENDERING_STARTED );
	if( !Render() )
	{
		CCP_LOGERR( "Failed to render a frame" );
	}
	renderContext.MarkFrameEvent( Tr2RenderContextEnum::FRAME_EVENT_RENDERING_FINISHED );
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
//   Not implemented for DX9 - we are happy with how windows behaves in DX9.
// Arguments:
//   activated - true if application was activated; false otherwise
// --------------------------------------------------------------------------------------
void TriDevice::ApplicationActivated( ApplicationActivation )
{
}

#endif
