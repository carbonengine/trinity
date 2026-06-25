// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveSpaceSceneRenderDriver.h"
#include "EveSpaceScene.h"
#include "../Tr2TextureReference.h"


namespace
{
Be::VarChooser AntiAliasingQualityChooser[] = {
	{ "Disabled", BeCast( EveSpaceSceneRenderDriver::AntiAliasingQuality::Disabled ), "" },
	{ "Low", BeCast( EveSpaceSceneRenderDriver::AntiAliasingQuality::Low ), "" },
	{ "Medium", BeCast( EveSpaceSceneRenderDriver::AntiAliasingQuality::Medium ), "" },
	{ "High", BeCast( EveSpaceSceneRenderDriver::AntiAliasingQuality::High ), "" },
	{ 0 }
};
Be::VarChooser AmbientOcclusionQualityChooser[] = {
	{ "Disabled", BeCast( EveSpaceSceneRenderDriver::AmbientOcclusionQuality::Disabled ), "" },
	{ "Low", BeCast( EveSpaceSceneRenderDriver::AmbientOcclusionQuality::Low ), "" },
	{ "Medium", BeCast( EveSpaceSceneRenderDriver::AmbientOcclusionQuality::Medium ), "" },
	{ "High", BeCast( EveSpaceSceneRenderDriver::AmbientOcclusionQuality::High ), "" },
	{ 0 }
};
Be::VarChooser ShadowQualityChooser[] = {
	{ "Disabled", BeCast( ShadowQuality::SHADOW_DISABLED ), "" },
	{ "Low", BeCast( ShadowQuality::SHADOW_LOW ), "" },
	{ "High", BeCast( ShadowQuality::SHADOW_HIGH ), "" },
	{ "Raytraced", BeCast( ShadowQuality::SHADOW_RAYTRACED ), "" },
	{ 0 }
};

const Be::VarChooser TriRMChooser[] = {
	// Name		   Value		    Docstring
	{ "RM_OPAQUE", BeCast( Tr2EffectStateManager::RM_OPAQUE ), "Opaque rendering" },
	{ "RM_DECAL", BeCast( Tr2EffectStateManager::RM_DECAL ), "Decal rendering" },
	{ "RM_DECAL_NO_DEPTH", BeCast( Tr2EffectStateManager::RM_DECAL_NO_DEPTH ), "Decal rendering (Normals Only)" },
	{ "RM_ALPHA", BeCast( Tr2EffectStateManager::RM_ALPHA ), "Alpha-blended rendering" },
	{ "RM_ALPHA_ADDITIVE", BeCast( Tr2EffectStateManager::RM_ALPHA_ADDITIVE ), "Additive rendering" },
	{ "RM_DEPTH_ONLY", BeCast( Tr2EffectStateManager::RM_DEPTH_ONLY ), "Depth-only rendering" },
	{ "RM_PICKING", BeCast( Tr2EffectStateManager::RM_PICKING ), "Rendering for picking" },
	{ "RM_FULLSCREEN", BeCast( Tr2EffectStateManager::RM_FULLSCREEN ), "Full-screen effects (2D) rendering" },
	{ "RM_SPRITE2D", BeCast( Tr2EffectStateManager::RM_SPRITE2D ), "2D sprite rendering" },
	{ 0 }
};

}
extern Be::VarChooser EveVisualizerChooser[];
extern const Be::VarChooser Tr2VolumetricQuality_Chooser[];
extern Be::VarChooser PostProcessQualityChooser[];

BLUE_REGISTER_ENUM_EX( "EveSpaceSceneRenderDriverAntiAliasingQuality", EveSpaceSceneRenderDriver::AntiAliasingQuality, AntiAliasingQualityChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
BLUE_REGISTER_ENUM_EX( "EveSpaceSceneRenderDriverAmbientOcclusionQuality", EveSpaceSceneRenderDriver::AmbientOcclusionQuality, AmbientOcclusionQualityChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );
BLUE_REGISTER_ENUM_EX( "ShadowQuality", ShadowQuality, ShadowQualityChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );



BLUE_DEFINE( EveSpaceSceneRenderDriver );

const Be::ClassInfo* EveSpaceSceneRenderDriver::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSpaceSceneRenderDriver, "" )
		MAP_INTERFACE( EveSpaceSceneRenderDriver )
		MAP_INTERFACE( ITr2RenderNode )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"enableRendering",
			m_enableRendering,
			"Turns off rendering the scene (including post-processing). The scene is still updated",
			Be::READWRITE )

		MAP_PROPERTY(
			"scene",
			GetScene,
			SetScene,
			"The scene to render" )

		MAP_ATTRIBUTE( "scene", m_scene, "", Be::PERSISTONLY ) // Purely for blue.Find methods to work

		MAP_ATTRIBUTE(
			"SSAO",
			m_ssao,
			"SSAO effect",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"projection",
			m_projection,
			"Camera projection transform. Only needed if `camera` attribute is not set",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"view",
			m_view,
			"Camera view transform. Only needed if `camera` attribute is not set",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"camera",
			m_camera,
			"Camera to be used for rendering",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"distortionEffect",
			m_distortionEffect,
			"Effect used for distortion",
			Be::READ )

		MAP_ATTRIBUTE(
			"toolsScenes",
			m_toolsScenes,
			"List of primitive scenes to render along with the space scene",
			Be::READ )

		MAP_ATTRIBUTE(
			"postProcess",
			m_postProcess,
			"Post-process renderer",
			Be::READ )

		MAP_ATTRIBUTE(
			"mainPassRenderingEnabled",
			m_mainPassRenderingEnabled,
			"If true, main (color) pass is rendered along with shadows and SSAO.",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"fpsRenderer",
			m_fpsRenderer,
			"FPS counter renderer",
			Be::READ )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"internalPixelFormat",
			m_internalPixelFormat,
			"Pixel format used for color render targets internally, before tonemapping",
			Be::READWRITE | Be::ENUM,
			Tr2RenderContextEnum_PixelFormat_Chooser )


		MAP_ATTRIBUTE_WITH_CHOOSER(
			"shadowQuality",
			m_settings.shadowQuality,
			"Shadow quality setting. One of trinity.ShadowQuality enum",
			Be::READWRITE | Be::ENUM,
			ShadowQualityChooser )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"antiAliasingQuality",
			m_settings.antiAliasingQuality,
			"Anti-aliasing quality setting. One of trinity.EveSpaceSceneRenderDriverAntiAliasingQuality enum",
			Be::READWRITE | Be::ENUM,
			AntiAliasingQualityChooser )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"aoQuality",
			m_settings.aoQuality,
			"Ambient occlusion quality setting. One of trinity.EveSpaceSceneRenderDriverAmbientOcclusionQuality enum",
			Be::READWRITE | Be::ENUM,
			AmbientOcclusionQualityChooser )

		MAP_ATTRIBUTE(
			"clearColor",
			m_settings.clearColor,
			"Color to use to clear the background of the scene",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"forceOpaqueBuffer",
			m_settings.forceOpaqueBuffer,
			"Force generating opaque color texture (for debugging and tools)",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"forceNormalMap",
			m_settings.forceNormalMap,
			"Force generating scene normal texture (for debugging and tools)",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"forceVelocityMap",
			m_settings.forceVelocityMap,
			"Force generating scene velocity texture (for debugging and tools)",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"enableUpscaling",
			m_settings.enableUpscaling,
			"Allows disabling upscaling even if it is enabled globally",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"enableDistortion",
			m_settings.enableDistortion,
			"Enable/disable distortion effects",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"showFPS",
			m_settings.showFPS,
			"Render FPS counter",
			Be::READWRITE )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"visualizeMethod",
			m_settings.visualizeMethod,
			"Debug visualization method to use",
			Be::READWRITE | Be::ENUM,
			EveVisualizerChooser )


		MAP_ATTRIBUTE_WITH_CHOOSER(
			"volumetricQuality",
			m_settings.volumetricQuality,
			"Quality setting for volumetric effects (clouds and fog). One of trinity.Tr2VolumerticQuality enum",
			Be::READWRITE | Be::ENUM,
			Tr2VolumetricQuality_Chooser )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"postProcessingQuality",
			m_settings.postProcessingQuality,
			"Quality setting for post-processing. One of trinity.PostProcessQuality enum",
			Be::READWRITE | Be::ENUM,
			PostProcess::PostProcessQualityChooser )

		MAP_ATTRIBUTE(
			"background",
			m_background,
			"Render node to render in the background of the scene",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"sceneOverlay",
			m_sceneOverlay,
			"Render node to render after (or instead of) the main color pass of the scene",
			Be::READWRITE )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"customStencilFormat",
			m_customStencilFormat,
			"Pixel format for custom stencil buffer generated in depth pass. Set to Unknown to disable.",
			Be::READWRITE | Be::ENUM,
			Tr2RenderContextEnum_PixelFormat_Chooser )

		MAP_ATTRIBUTE(
			"depthPassTechnique",
			m_depthPassTechnique,
			"Name of the shader technique used during the depth pass",
			Be::READWRITE )

		MAP_METHOD_AND_WRAP(
			"GetAllTempTextures",
			GetAllTempTextures,
			"Returns all temp textures used by the gpu resource pool. For debugging purposes." )

		MAP_PROPERTY(
			"debugMode",
			GetDebugMode,
			SetDebugMode,
			"Toggles debug mode on and off. In debug mode sharing is disabled for temporary GPU resources" )

		MAP_ATTRIBUTE(
			"reflectionCorrectionEnabled",
			m_reflectionCorrectionEnabled,
			"Enables ambient light reflection angle correction, improving the ambient light specular reflections on rough surfaces",
			Be::READWRITE )

	EXPOSURE_END()
}
