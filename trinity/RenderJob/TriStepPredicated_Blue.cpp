////////////////////////////////////////////////////////////
//
//    Created:   July 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "TriStepPredicated.h"
#include "Include/ITr2NamedPredicate.h"


BLUE_DEFINE_INTERFACE( ITr2NamedPredicate );

BLUE_DEFINE( TriStepPredicated );

const Be::ClassInfo* TriStepPredicated::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepPredicated, "" )
		MAP_INTERFACE( TriStepPredicated )
		MAP_INTERFACE( TriRenderStep )

		MAP_ATTRIBUTE
		(
			"step",
			m_step,
			"Render step to execute\n"
			":jessica-group: Predicate",
			Be::READWRITE
		)
		MAP_ATTRIBUTE
		(
			"predicate",
			m_predicate,
			"Predicate object to use for conditional execution\n"
			":jessica-group: Predicate",
			Be::READWRITE
		)
		MAP_ATTRIBUTE
		(
			"predicateName",
			m_predicateName,
			"Name of the predicate\n"
			":jessica-group: Predicate",
			Be::READWRITE
		)
		MAP_PROPERTY_READONLY( 
			"predicateValue", 
			GetPredicateValue,
			"Current predicate value\n"
			":jessica-group: Predicate"
			)

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"__init__",
			Create,
			3,
			"Create a step that will execute its child step when provided named predicate returns true.\n"
			":param name: predicate name\n"
			":param predicate: predicate object\n"
			":param step: child step"
		)

	EXPOSURE_CHAINTO( TriRenderStep )
}
