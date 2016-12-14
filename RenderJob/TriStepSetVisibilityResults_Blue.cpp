////////////////////////////////////////////////////////////
//
//    Created:   August 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"
#include "TriStepSetVisibilityResults.h"

BLUE_DEFINE( TriStepSetVisibilityResults );

const Be::ClassInfo* TriStepSetVisibilityResults::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepSetVisibilityResults, "Render step for executing assigning visibility results to a queriable scene" )
		MAP_INTERFACE( TriStepSetVisibilityResults )

		MAP_ATTRIBUTE
		(
			"queryable", 
			m_queryable,
			"Queryable object to assign visibility results to",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"results", 
			m_results,
			"The results of the visibility query to assign to queryable object",
			Be::READWRITE
		)

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		( 
			"__init__", 
			py__init__, 
			2,
			"Create a render step that assigns already generated visibility results to a queryable scene\n"
			":param queryable: an object that supports ITr2VisibilityQueryable (default None)\n"
			":param results: a Tr2VisibilityResults object that stores visibility results"
		)

		EXPOSURE_CHAINTO( TriRenderStep )
}