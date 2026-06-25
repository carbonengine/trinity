// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "TriStepSetDebugRenderer.h"
#include "include/ITr2DebugRenderer.h"

BLUE_DEFINE( TriStepSetDebugRenderer );

const Be::ClassInfo* TriStepSetDebugRenderer::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepSetDebugRenderer, "" )
		MAP_INTERFACE( TriStepSetDebugRenderer )

		MAP_ATTRIBUTE( "renderer", m_debugRenderer, "Debug renderer to set as global debug renderer", Be::READWRITE )
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			SetDebugRenderer,
			1,
			":param renderer: debug renderer" )

	EXPOSURE_CHAINTO( TriRenderStep )
}