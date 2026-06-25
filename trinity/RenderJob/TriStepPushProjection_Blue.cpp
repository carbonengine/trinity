// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepPushProjection.h"
#include "TriRenderStep.h"

BLUE_DEFINE( TriStepPushProjection );

const Be::ClassInfo* TriStepPushProjection::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepPushProjection, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepPushProjection )

	EXPOSURE_CHAINTO( TriRenderStep )
}