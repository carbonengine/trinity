#include "StdAfx.h"
#include "TriStepPushRenderTarget.h"
#include "TriRenderStep.h"

BLUE_DEFINE( TriStepPushRenderTarget );


const Be::ClassInfo* TriStepPushRenderTarget::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepPushRenderTarget, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepPushRenderTarget )

		MAP_ATTRIBUTE("renderTarget", m_renderTarget, "na", Be::READWRITE )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"__init__", 
			py__init__, 
			1, 
			"Creates a render job that pushes a rendertarget to the device\n"
			":param renderTarget: a Tr2RenderTarget"
		)

	EXPOSURE_CHAINTO( TriRenderStep )
}
