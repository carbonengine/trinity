// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "TriStepEnableWireframeMode.h"
#include "ITr2VisualizationModeRenderer.h"

BLUE_DEFINE( TriStepEnableWireframeMode );

const Be::ClassInfo* TriStepEnableWireframeMode::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepEnableWireframeMode, "" )
		MAP_INTERFACE( TriStepEnableWireframeMode )

		MAP_ATTRIBUTE(
			"enableWireframe",
			m_enableWireframe,
			"Enable wireframe for the object",
			Be::READWRITE )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			py__init__,
			1,
			"Creates a render step that enables/disables wireframe rendering.\n"
			":param enable: True/False (default False)" )

	EXPOSURE_CHAINTO( TriRenderStep )
}