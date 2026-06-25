// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#ifdef _WIN32

#include "TriStepRemoteUpdate.h"
#include "TriRenderStep.h"

BLUE_DEFINE( TriStepRemoteUpdate );

const Be::ClassInfo* TriStepRemoteUpdate::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepRemoteUpdate, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepRemoteUpdate )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			py__init__,
			1,
			"Creates a render step that performs a View/Projection/Viewport update from shared memory\n"
			"Useful when running trinity from a remote process\n"
			"\n:param view: TriView"
			"\n:param projection: TriProjection"
			"\n:param viewport: TriViewport"
			"\n:param id: unique identifier" )

	EXPOSURE_CHAINTO( TriRenderStep )
}

#endif
