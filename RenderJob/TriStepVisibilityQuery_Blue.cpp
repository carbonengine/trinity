#include "StdAfx.h"
#include "TriStepVisibilityQuery.h"

BLUE_DEFINE( TriStepVisibilityQuery );

const Be::ClassInfo* TriStepVisibilityQuery::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepVisibilityQuery, "Render step for executing a visibility query.")

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepVisibilityQuery )

		MAP_ATTRIBUTE
		(
			"queryable", 
			m_queryable,
			"The object to be queried",
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		(
			"results", 
			m_results,
			"The results of the visibility query",
			Be::READWRITE
		)

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		( 
			"__init__", 
			py__init__, 
			2,
			"Create a render step that queries a queryable object for visibility results."
			"\nNote that this step should occur after setting the render target and viewport,"
			"\notherwise the adjusted projection matrix used in the visibility query might be incorrect."
			"\n:param queryable: an object that supports ITr2VisibilityQueryable (default None)"
			"\n:param results: a Tr2VisibilityResults object to store the results in"
		)

		EXPOSURE_CHAINTO( TriRenderStep )
}