#include "StdAfx.h"
#include "TriStepRenderScene.h"

BLUE_DEFINE( TriStepRenderScene );

const Be::ClassInfo* TriStepRenderScene::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriStepRenderScene, "Render step for rendering a scene" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepRenderScene )

		MAP_ATTRIBUTE
		(
			"scene", 
			m_scene,
			"The scene to be rendered",
			Be::READWRITE | Be::PERSIST
		)

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		( 
			"__init__", 
			py__init__,
			1,
			"Creates a render step that renders an ITr2Scene\n"
			":param scene: an ITr2Scene object"
		)

	EXPOSURE_CHAINTO( TriRenderStep )
}