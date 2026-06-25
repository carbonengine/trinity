// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepPopProjection.h"
#include "TriRenderStep.h"

BLUE_DEFINE( TriStepPopProjection );

const Be::ClassInfo* TriStepPopProjection::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepPopProjection, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepPopProjection )

	EXPOSURE_CHAINTO( TriRenderStep )
}