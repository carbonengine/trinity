// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepPopDepthStencil.h"
#include "TriRenderStep.h"


BLUE_DEFINE( TriStepPopDepthStencil );

const Be::ClassInfo* TriStepPopDepthStencil::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepPopDepthStencil, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepPopDepthStencil )

	EXPOSURE_CHAINTO( TriRenderStep )
}