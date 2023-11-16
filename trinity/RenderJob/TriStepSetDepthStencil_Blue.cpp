#include "StdAfx.h"
#include "TriStepSetDepthStencil.h"
#include "TriRenderStep.h"
#include "Tr2DepthStencil.h"


BLUE_DEFINE( TriStepSetDepthStencil );

const Be::ClassInfo* TriStepSetDepthStencil::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepSetDepthStencil, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepSetDepthStencil )

		MAP_ATTRIBUTE( "depthStencil", m_depthStencil, "Tr2DepthStencil set by this step", Be::READWRITE )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"__init__", 
			SetDepthStencil,
			1,
			"Creates a render job that sets a depth stencil surface to the device\n"
			":param depthStencil: a Tr2DepthStencil"
		)

	EXPOSURE_CHAINTO( TriRenderStep )
}