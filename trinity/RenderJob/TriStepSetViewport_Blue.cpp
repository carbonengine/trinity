// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepSetViewport.h"
#include "TriRenderStep.h"
#include "TriViewport.h"

BLUE_DEFINE( TriStepSetViewport );

const Be::ClassInfo* TriStepSetViewport::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepSetViewport, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepSetViewport )

		MAP_ATTRIBUTE( "viewport", m_viewport, "na", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			SetViewport,
			1,
			"Creates a render step that sets a viewport to the device\n"
			":param viewport: the TriViewport to set (default None)" )

	EXPOSURE_CHAINTO( TriRenderStep )
}