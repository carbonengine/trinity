// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepPushViewTransform.h"
#include "TriRenderStep.h"

BLUE_DEFINE( TriStepPushViewTransform );

const Be::ClassInfo* TriStepPushViewTransform::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepPushViewTransform, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepPushViewTransform )

	EXPOSURE_CHAINTO( TriRenderStep )
}