#include "StdAfx.h"

#ifdef _WIN32

#include "TriStepRemoteSync.h"

BLUE_DEFINE( TriStepRemoteSync );

const Be::ClassInfo* TriStepRemoteSync::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepRemoteSync, "" )
		MAP_INTERFACE( TriStepRemoteSync )
		MAP_INTERFACE( TriRenderStep )
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
		"__init__", 
		py__init__,
		1,
		"Creates a render step that syncs the rendering to a windows event primitive.\n"
		"Usefull when needed to sync to an external process like Maya.\n"
		":param id: this id will be appended to the event names if it is >= 0")
		EXPOSURE_CHAINTO( TriRenderStep )
}

#endif
