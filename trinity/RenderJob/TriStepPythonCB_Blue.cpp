#include "StdAfx.h"
#include "TriStepPythonCB.h"

BLUE_DEFINE( TriStepPythonCB );


const Be::ClassInfo* TriStepPythonCB::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepPythonCB, "" )

		MAP_INTERFACE( TriStepPythonCB )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"__init__",
			SetCallback,
			1,
			"Create a render step that issues a Python callback\n"
			":param cb: A Python callable (default None)"
		)
		
		MAP_METHOD_AND_WRAP
		( 
			"SetCallback",
			SetCallback, 
			"Set the callback called when this renderstep executes.\n"
			":param cb: a method which has no arguments."				
		)

	EXPOSURE_CHAINTO( TriRenderStep )
}

