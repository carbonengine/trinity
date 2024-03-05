#include "StdAfx.h"
#include "TriStepPushViewport.h"
#include "TriRenderStep.h"

BLUE_DEFINE( TriStepPushViewport );

const Be::ClassInfo* TriStepPushViewport::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepPushViewport, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepPushViewport )

	EXPOSURE_CHAINTO( TriRenderStep )
}