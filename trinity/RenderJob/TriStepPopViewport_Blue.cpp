#include "StdAfx.h"
#include "TriStepPopViewport.h"
#include "TriRenderStep.h"

BLUE_DEFINE( TriStepPopViewport );

const Be::ClassInfo* TriStepPopViewport::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepPopViewport, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepPopViewport )

	EXPOSURE_CHAINTO( TriRenderStep )
}