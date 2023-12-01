#include "StdAfx.h"
#include "TriStepSetRenderTarget.h"
#include "TriRenderStep.h"


BLUE_DEFINE( TriStepSetRenderTarget );

const Be::ClassInfo* TriStepSetRenderTarget::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepSetRenderTarget, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepSetRenderTarget )

		MAP_ATTRIBUTE("renderTarget", m_renderTarget, "na", Be::READWRITE | Be::PERSIST)

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"__init__", 
			py__init__, 
			1,
			"Creates a render job that sets a depth stencil to the device\n"
			":param renderTarget: a Tr2RenderTarget"
		)

	EXPOSURE_CHAINTO( TriRenderStep )
}