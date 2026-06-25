// Copyright © 2011 CCP ehf.

#include "StdAfx.h"

#include "TriStepToggleCubemap.h"
#include "ITr2VisualizationModeRenderer.h"
#include "Interior/Tr2InteriorScene.h"

BLUE_DEFINE( TriStepToggleCubemap );

const Be::ClassInfo* TriStepToggleCubemap::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepToggleCubemap, "" )
		MAP_INTERFACE( TriStepToggleCubemap )

		MAP_ATTRIBUTE(
			"m_showCubemap",
			m_showCubemap,
			"Toggle the cubemap for the scene",
			Be::READWRITE )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			py__init__,
			2,
			":param show: show/hide cubemap\n"
			":param scene: cubemap scene" )

	EXPOSURE_CHAINTO( TriRenderStep )
}
