// Copyright © 2024 CCP ehf.

#include "StdAfx.h"
#include "TriStepSetUpscalingContextID.h"
#include "TriRenderStep.h"


BLUE_DEFINE( TriStepSetUpscalingContextID );

const Be::ClassInfo* TriStepSetUpscalingContextID::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepSetUpscalingContextID, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepSetUpscalingContextID )

		MAP_ATTRIBUTE( "upscalingContextID", m_upscalingContextID, "na", Be::READ )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			py__init__,
			1,
			"Sets the upscalingContextID for easy retrieval on the current render job\n"
			":param upscalingContextID: the id of the upscaling context" )

	EXPOSURE_CHAINTO( TriRenderStep )
}