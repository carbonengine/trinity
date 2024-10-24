////////////////////////////////////////////////////////////
//
//    Created:   February 2023
//    Copyright: CCP 2023
//

#include "StdAfx.h"
#include "Tr2VolumetricsRenderer.h"
#include "TriRenderBatch.h"


const Be::VarChooser Tr2VolumetricQuality_Chooser[] = {
	{ "Low", BeCast( Tr2VolumerticQuality::Low ), "" },
	{ "Medium", BeCast( Tr2VolumerticQuality::Medium ), "" },
	{ "High", BeCast( Tr2VolumerticQuality::High ), "" },
	{ "Ultra", BeCast( Tr2VolumerticQuality::Ultra ), "" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX(
	"Tr2VolumerticQuality",
	Tr2VolumerticQuality,
	Tr2VolumetricQuality_Chooser,
	ENUM_REG_ENUM_OBJECT_ON_MODULE );

BLUE_DEFINE_INTERFACE( ITr2FroxelFogSettings );

BLUE_DEFINE( Tr2VolumetricsRenderer );

const Be::ClassInfo* Tr2VolumetricsRenderer::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2VolumetricsRenderer, "" )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"quality",
			m_quality,
			"Volumetrics quality setting. Different volumetric renderables may take it into account\n"
			":jessica-group: Clouds",
			Be::READWRITE | Be::ENUM,
			Tr2VolumetricQuality_Chooser )
		MAP_ATTRIBUTE(
			"scaleFactor",
			m_scaleFactor,
			"Volumetrics resulution factor (1 - full size, 0.5 - 1/2 resolution, etc.)\n"
			":jessica-group: Clouds",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"blur",
			m_blur,
			"Blur volumetrics to remove noise\n"
			":jessica-group: Clouds",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"castShadows",
			m_castShadows,
			"Volumetrics cast shadows onto opaque geometry\n"
			":jessica-group: Clouds",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"receiveShadows",
			m_receiveShadows,
			"Volumetrics receive shadows from opaque geometry\n"
			":jessica-group: Clouds",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"volumeSlices",
			m_volumeSlices,
			"Frustum-align volume texture (represented as texture array) containing volumetic colors/density",
			Be::READ )
		MAP_ATTRIBUTE(
			"downsampledDepth",
			m_downsampledDepth,
			"",
			Be::READ )
		MAP_ATTRIBUTE(
			"blurScratch",
			m_blurScratch,
			"",
			Be::READ )



		MAP_ATTRIBUTE(
			"thickness",
			m_froxelFogSettings.thickness,
			"Overall thickness of the fog. A higher thickness makes the fog more intense close up to the camera, making god ray shadows pop more.\n"
			":jessica-group: Froxel Fog",
			Be::READ )
		MAP_ATTRIBUTE(
			"directionality",
			m_froxelFogSettings.directionality,
			"Scattering directionality of the fog. A higher value causes the fog to light up only when looking directly towards light sources.\n"
			":jessica-group: Froxel Fog",
			Be::READ )
		MAP_ATTRIBUTE(
			"environmentIntensity",
			m_froxelFogSettings.environmentIntensity,
			"The visibility of the skybox behind the fog, blurred by the directionality setting above.\n"
			":jessica-group: Froxel Fog",
			Be::READ )
		MAP_ATTRIBUTE(
			"fogColor",
			m_froxelFogSettings.fogColor,
			"The color of the fog itself. Changing this will change what light is scattered from the sun, dynamic lights and the fog.\n"
			":jessica-group: Froxel Fog",
			Be::READ )
		MAP_ATTRIBUTE(
			"backgroundColor",
			m_froxelFogSettings.backgroundColor,
			"How transparent the fog is to the background, controllable for RGB separately. When set to (0, 0, 0), the background is completely blocked by the fog. \n"
			":jessica-group: Froxel Fog",
			Be::READ )

		MAP_ATTRIBUTE(
			"gameBackClip",
			m_gameBackClip,
			"Hardcoded back clip value, must match the back clip used by the game. This value makes sure that the fog looks correct even if the back clip is changed in Graphite. \n"
			":jessica-group: Froxel Fog",
			Be::READ )

		MAP_ATTRIBUTE(
			"mieEnvironmentMap",
			m_mieEnvironmentMap,
			"",
			Be::READ )
		MAP_ATTRIBUTE(
			"fogFroxels",
			m_fogResources.fogFroxels,
			"",
			Be::READ )

	EXPOSURE_END()
}
