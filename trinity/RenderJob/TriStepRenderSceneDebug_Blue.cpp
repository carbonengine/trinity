// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepRenderSceneDebug.h"

BLUE_DEFINE( TriStepRenderSceneDebug );

const Be::ClassInfo* TriStepRenderSceneDebug::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepRenderSceneDebug, "" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepRenderSceneDebug )

		MAP_ATTRIBUTE( "scene", m_scene, "na", Be::READWRITE )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			py__init__,
			1,
			"Create a render step to render a ITr2Scene with debug rendering\n"
			":param scene: an ITr2Scene (default None)" )

	EXPOSURE_CHAINTO( TriRenderStep )
}