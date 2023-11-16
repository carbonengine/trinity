#include "StdAfx.h"
#include "TriStepSetProjection.h"
#include "TriRenderStep.h"

BLUE_DEFINE( TriStepSetProjection );

const Be::ClassInfo* TriStepSetProjection::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepSetProjection, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepSetProjection )

		MAP_ATTRIBUTE("projection", m_projection, "na", Be::READWRITE | Be::PERSIST)

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"__init__",
			SetProjection, 
			1,
			"Create a render step to set the projection to the device\n"
			":param projection: a TriProjection to set (default None)"
		)

	EXPOSURE_CHAINTO( TriRenderStep )
}