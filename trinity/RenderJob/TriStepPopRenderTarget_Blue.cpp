#include "StdAfx.h"
#include "TriStepPopRenderTarget.h"
#include "TriRenderStep.h"


BLUE_DEFINE( TriStepPopRenderTarget );

const Be::ClassInfo* TriStepPopRenderTarget::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepPopRenderTarget, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepPopRenderTarget )

		MAP_ATTRIBUTE( "slot", m_slot, "", Be::READWRITE )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			py__init__,
			1,
			"Creates a render step that pops a rendertarget from the device\n"
			":param slot: render target slot" )

	EXPOSURE_CHAINTO( TriRenderStep )
}