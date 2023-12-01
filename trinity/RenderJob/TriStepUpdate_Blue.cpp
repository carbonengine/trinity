#include "StdAfx.h"
#include "TriStepUpdate.h"

BLUE_DEFINE( TriStepUpdate );

const Be::ClassInfo* TriStepUpdate::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepUpdate, "" )
		MAP_INTERFACE( TriStepUpdate )

		MAP_ATTRIBUTE( "object", m_object, "The object to be updated", Be::READWRITE)
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS( 
			"__init__", 
			SetUpdateable, 
			1, 
			":param obj: The object to be updated" )

	EXPOSURE_CHAINTO( TriRenderStep )
}