#include "StdAfx.h"
#include "TriStepPopViewTransform.h"
#include "TriRenderStep.h"

BLUE_DEFINE( TriStepPopViewTransform );

const Be::ClassInfo* TriStepPopViewTransform::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepPopViewTransform, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepPopViewTransform )

	EXPOSURE_CHAINTO( TriRenderStep )
}