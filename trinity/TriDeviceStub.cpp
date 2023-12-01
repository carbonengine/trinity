#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "TriDevice.h"

#include "RenderJob/Tr2RenderJobs.h"

using namespace Tr2RenderContextEnum;

CCP_STATS_DECLARED_ELSEWHERE( presentTime );


void TriDevice::HandleRenderTick( Be::Time realTime, Be::Time simTime )
{
	if( m_renderJobs )
	{
		m_renderJobs->RunUpdate( realTime, simTime );
	}

	m_postUpdateCallbacks->Update();

	Render();
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
