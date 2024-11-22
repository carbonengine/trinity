////////////////////////////////////////////////////////////
//
//    Created:   December 2024
//    Copyright: CCP 2024
//

#include "StdAfx.h"
#include "EveChildFogVolume.h"


BLUE_DEFINE( EveChildFogVolume );

const Be::ClassInfo* EveChildFogVolume::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildFogVolume, "" )
		MAP_INTERFACE( ITr2FroxelFogSettings )
		MAP_INTERFACE( EveEntity )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( IInitialize )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"priority",
			m_settings.priority,
			"Priority of this override. Affects blending between different override objects",
			Be::READWRITE | Be::PERSIST | Be::ENUM,
			PostProcessEnums::Tr2PostProcessPriorityChooser )
		MAP_ATTRIBUTE( 
			"intensity", 
			m_intensity, 
			"Manually adjustable intensity for fog settings\n"
			":jessica-numeric-range: (0.0, 1.0)", 
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"thickness",
			m_settings.value.thickness,
			"Overall thickness of the fog. A higher thickness makes the fog more intense close up to the camera, making god ray shadows pop more.\n"
			":jessica-group: Froxel Fog",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"lightDirectionality",
			m_settings.value.lightDirectionality,
			"Scattering directionality for the sun and dynamic local lights. A higher value causes the fog to light up only when looking directly towards light sources.\n"
			":jessica-group: Froxel Fog",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"environmentIntensity",
			m_settings.value.environmentIntensity,
			"The visibility of the skybox behind the fog, blurred by the directionality setting above.\n"
			":jessica-group: Froxel Fog",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"environmentDirectionality",
			m_settings.value.environmentDirectionality,
			"Scattering directionality for the environment map. A lower value causes a more uniform environment color, while a higher value causes the fog to light up only when looking directly at bright areas of the environment map.\n"
			":jessica-group: Froxel Fog",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"fogColor",
			m_settings.value.fogColor,
			"The color of the fog itself. Changing this will change what light is scattered from the sun, dynamic lights and the fog.\n"
			":jessica-group: Froxel Fog",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"backgroundVisibility",
			m_settings.value.backgroundVisibility,
			"How transparent the fog is. Increasing this causes the skybox and objects to become visible, no matter how high the fog thickness is set. \n"
			":jessica-group: Froxel Fog",
			Be::READWRITE | Be::PERSIST )

		

		MAP_ATTRIBUTE(
			"godRayNoiseIntensity",
			m_settings.value.godRayNoiseIntensity,
			"The intensity of the 2D noise added to the incoming sun light. This adds god rays even when there are no shadows being cast by objects. \n"
			":jessica-group: Froxel Fog",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"godRayNoiseFrequency",
			m_settings.value.godRayNoiseFrequency,
			"The frequency of the 2D noise added to the incoming sun light. A higher value makes the god rays larger and less detailed. Around 15.0 is a good starting point. \n"
			":jessica-group: Froxel Fog",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"godRayNoiseAnimationSpeed",
			m_settings.value.godRayNoiseAnimationSpeed,
			"The animation speed of the 2D noise added to the incoming sun light. Setting this above 0.0 makes the god rays shift and morph over time. \n"
			":jessica-group: Froxel Fog",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"fogNoiseIntensity",
			m_settings.value.fogNoiseIntensity,
			"The intensity of the 3D noise used to modify the fog color. Setting this to 0 gives you a uniform fog, while values above 0.0 will add more variation to the fog color, similar to puffy clouds. \n "
			":jessica-group: Froxel Fog",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"fogNoiseFrequency",
			m_settings.value.fogNoiseFrequency,
			"The frequency of the 3D noise used to modify the fog color. A higher value makes the cloud puffs larger and less detailed, while a smaller value makes the puffs smaller and more numerous. Around 15.0 is a good starting point. \n"
			":jessica-group: Froxel Fog",
			Be::READWRITE | Be::PERSIST )
		/* MAP_ATTRIBUTE(
			"fogNoiseMovementSpeed",
			m_settings.value.fogNoiseMovementSpeed,
			"The animation speed of the 3D noise used to modify the fog color. This is used to scroll the fog over time, simulating a simple 'wind' effect. \n"
			":jessica-group: Froxel Fog",
			Be::READWRITE | Be::PERSIST )*/


		MAP_ATTRIBUTE( "boundingSphereCenter", m_boundingSphere.center, "", Be::READ )
		MAP_ATTRIBUTE( "boundingSphereRadius", m_boundingSphere.radius, "", Be::READ )
		MAP_ATTRIBUTE( "volumes", m_volumes, "", Be::READ | Be::PERSIST )

	EXPOSURE_END()
}
