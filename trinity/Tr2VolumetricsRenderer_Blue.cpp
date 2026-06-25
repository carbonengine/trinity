// Copyright © 2023 CCP ehf.

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
			"thickness",
			m_froxelFogSettings.thickness.value,
			"Overall thickness of the fog. A higher thickness makes the fog more intense close up to the camera, making god ray shadows pop more.\n"
			":jessica-group: Froxel Fog",
			Be::READ )
		MAP_ATTRIBUTE(
			"lightDirectionality",
			m_froxelFogSettings.lightDirectionality.value,
			"Scattering directionality for the sun and dynamic local lights. A higher value causes the fog to light up only when looking directly towards light sources.\n"
			":jessica-group: Froxel Fog",
			Be::READ )
		MAP_ATTRIBUTE(
			"environmentIntensity",
			m_froxelFogSettings.environmentIntensity.value,
			"The visibility of the skybox behind the fog, blurred by the directionality setting above.\n"
			":jessica-group: Froxel Fog",
			Be::READ )
		MAP_ATTRIBUTE(
			"environmentDirectionality",
			m_froxelFogSettings.environmentDirectionality.value,
			"Scattering directionality for the environment map. A lower value causes a more uniform environment color, while a higher value causes the fog to light up only when looking directly at bright areas of the environment map.\n"
			":jessica-group: Froxel Fog",
			Be::READ )
		MAP_ATTRIBUTE(
			"fogColor",
			m_froxelFogSettings.fogColor.value,
			"The color of the fog itself. Changing this will change what light is scattered from the sun, dynamic lights and the fog.\n"
			":jessica-group: Froxel Fog",
			Be::READ )
		MAP_ATTRIBUTE(
			"backgroundVisibility",
			m_froxelFogSettings.backgroundVisibility.value,
			"How transparent the fog is. Increasing this causes the skybox and objects to become visible, no matter how high the fog thickness is set.\n"
			":jessica-group: Froxel Fog",
			Be::READ )

		MAP_ATTRIBUTE(
			"godRayNoiseIntensity",
			m_froxelFogSettings.godRayNoiseIntensity.value,
			"The intensity of the 2D noise added to the incoming sun light. This adds god rays even when there are no shadows being cast by objects. \n"
			":jessica-group: Froxel Fog",
			Be::READ )
		MAP_ATTRIBUTE(
			"godRayNoiseFrequency",
			m_froxelFogSettings.godRayNoiseFrequency.value,
			"The frequency of the 2D noise added to the incoming sun light. A higher value makes the god rays larger and less detailed. Around 15.0 is a good starting point. \n"
			":jessica-group: Froxel Fog",
			Be::READ )
		MAP_ATTRIBUTE(
			"godRayNoiseAnimationSpeed",
			m_froxelFogSettings.godRayNoiseAnimationSpeed.value,
			"The animation speed of the 2D noise added to the incoming sun light. Setting this above 0.0 makes the god rays shift and morph over time. \n"
			":jessica-group: Froxel Fog",
			Be::READ )

		MAP_ATTRIBUTE(
			"fogNoiseIntensity",
			m_froxelFogSettings.fogNoiseIntensity.value,
			"The intensity of the 3D noise used to modify the fog color. Setting this to 0 gives you a uniform fog, while values above 0.0 will add more variation to the fog color, similar to puffy clouds. \n "
			":jessica-group: Froxel Fog",
			Be::READ )
		MAP_ATTRIBUTE(
			"fogNoiseFrequency",
			m_froxelFogSettings.fogNoiseFrequency.value,
			"The frequency of the 3D noise used to modify the fog color. A higher value makes the cloud puffs larger and less detailed, while a smaller value makes the puffs smaller and more numerous. Around 15.0 is a good starting point. \n"
			":jessica-group: Froxel Fog",
			Be::READ )
		MAP_ATTRIBUTE(
			"fogNoiseMovementSpeed",
			m_froxelFogSettings.fogNoiseMovementSpeed.value,
			"The animation speed of the 3D noise used to modify the fog color. This is used to scroll the fog over time, simulating a simple 'wind' effect. \n"
			":jessica-group: Froxel Fog",
			Be::READ )


		MAP_ATTRIBUTE(
			"logBlending",
			m_logBlending,
			"Enables a logarithm-based blending for fog volumes, giving a smoother fade-in when a high-thickness fog is blended in. This is always enabled, but can be disabled for debugging purposes.\n"
			":jessica-group: Froxel Fog",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"logBlendingSmoothness",
			m_logBlendingSmoothness,
			"The smoothness of the logarithmic blending. A higher value here causes fog thickness to ramp up more slowly when a thick fog is faded in. This should be kept at the hard-coded default, but can be changed for debugging/testing purposes.\n"
			":jessica-group: Froxel Fog",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"gameBackClip",
			m_gameBackClip,
			"Hardcoded back clip value, must match the back clip used by the game. This value makes sure that the fog looks correct even if the back clip is changed in Graphite. It can be changed for debugging/testing purposes, but will not be saved.\n"
			":jessica-group: Froxel Fog",
			Be::READWRITE )




		MAP_ATTRIBUTE(
			"mieEnvironmentMap",
			m_mieEnvironmentMap,
			"",
			Be::READ )

	EXPOSURE_END()
}
