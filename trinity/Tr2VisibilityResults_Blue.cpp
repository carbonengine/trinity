#include "StdAfx.h"
#include "Tr2VisibilityResults.h"

BLUE_DEFINE( Tr2VisibilityResults );

const Be::ClassInfo* Tr2VisibilityResults::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2VisibilityResults, "" )

		MAP_INTERFACE( Tr2VisibilityResults )

		MAP_METHOD_AND_WRAP( "Clear", Clear, "Clears the result set" )

		MAP_METHOD_AND_WRAP( 
			"GetNumVisibilityEvents",
			GetNumVisibilityEvents,
			"Gets the number of visibility events in the result set"
		)

	EXPOSURE_END()
}