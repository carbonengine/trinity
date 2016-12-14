#include "StdAfx.h"
#include "TriStepPushDepthStencil.h"
#include "TriRenderStep.h"
#include "Tr2DepthStencil.h"

BLUE_DEFINE( TriStepPushDepthStencil );

const Be::ClassInfo* TriStepPushDepthStencil::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepPushDepthStencil, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepPushDepthStencil )

		MAP_ATTRIBUTE("depthStencil", m_depthStencil, "na", Be::READWRITE )
		MAP_ATTRIBUTE("pushCurrent", m_pushCurrent, "If depthStencil is None, then push current but leave it bound (True), or actually disable depth testing (False).", Be::READWRITE )		

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"__init__", 
			py__init__, 
			1,
			"Creates a render job that pushes a depth stencil to the device\n"
			":param depthStencil: a Tr2DepthStencil"
		)

	EXPOSURE_CHAINTO( TriRenderStep )
}