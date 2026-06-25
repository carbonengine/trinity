// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepRenderPass.h"

const Be::VarChooser TriMultiPassScenePassTypeChooser[] = {
	// Name							Value												Docstring
	{ "TRIPASS_BEGIN_RENDER", BeCast( ITr2MultiPassScene::RP_BEGIN_RENDER ), "Begin rendering" },
	{ "TRIPASS_PRE_PASS", BeCast( ITr2MultiPassScene::RP_PRE_PASS ), "Render prepass (depth, normals, specular)" },
	{ "TRIPASS_LIGHT_PASS", BeCast( ITr2MultiPassScene::RP_LIGHT_PASS ), "Render lights" },
	{ "TRIPASS_GATHER_PASS", BeCast( ITr2MultiPassScene::RP_GATHER_PASS ), "Render gather pass" },
	{ "TRIPASS_FLARE_PASS", BeCast( ITr2MultiPassScene::RP_FLARE_PASS ), "Render flare pass" },
	{ "TRIPASS_END_RENDER", BeCast( ITr2MultiPassScene::RP_END_RENDER ), "End rendering" },
	{ "TRIPASS_BACKGROUND_RENDER", BeCast( ITr2MultiPassScene::RP_BACKGROUND_RENDER ), "Background rendering" },
	{ "TRIPASS_MAIN_RENDER", BeCast( ITr2MultiPassScene::RP_MAIN_RENDER ), "Main rendering" },
	{ "TRIPASS_REFLECTION_RENDER", BeCast( ITr2MultiPassScene::RP_REFLECTION_RENDER ), "Reflection rendering" },
	{ "TRIPASS_DEPTH_PASS", BeCast( ITr2MultiPassScene::RP_DEPTH_PASS ), "Depth pass" },
	{ "TRIPASS_SET_PERFRAME_DATA", BeCast( ITr2MultiPassScene::RP_SET_PERFRAME_DATA ), "Set perframe data to shaders" },
	{ "TRIPASS_RENDER_UI", BeCast( ITr2MultiPassScene::RP_RENDER_UI ), "Set perframe data to shaders" },
	{ 0 }
};

BLUE_REGISTER_ENUM( "MULTI_PASS_SCENE_PASS", ITr2MultiPassScene::PassType, TriMultiPassScenePassTypeChooser );


BLUE_DEFINE( TriStepRenderPass );

const Be::ClassInfo* TriStepRenderPass::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepRenderPass, "Render step for rendering scene passes" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepRenderPass )

		MAP_ATTRIBUTE(
			"scene",
			m_scene,
			"The scene to be rendered",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"passType",
			m_pass,
			"Pass to be rendered",
			Be::READWRITE | Be::PERSIST | Be::ENUM,
			TriMultiPassScenePassTypeChooser )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			py__init__,
			2,
			"Create a render step to render a scene with a specific pass\n"
			":param scene: ITr2MultiPassScene (default None)\n"
			":param passType: TRIPREPASS pass type (default TRIPREPASS_BEGIN_RENDER)" )

	EXPOSURE_CHAINTO( TriRenderStep )
}