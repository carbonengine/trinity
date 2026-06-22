// Copyright © 2024 CCP ehf.

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


#define VOLUME_ATTRIBUTE_DEFINE( NAME, DESC )                                                                                                                   \
	MAP_ATTRIBUTE( #NAME "Enabled", m_settings.NAME.enabled, "Enables " #NAME " for the froxel fog \n:jessica-group: Froxel Fog", Be::READWRITE | Be::PERSIST ) \
	MAP_ATTRIBUTE( #NAME, m_settings.NAME.value, DESC "\n:jessica-group: Froxel Fog", Be::READWRITE | Be::PERSIST )


		VOLUME_ATTRIBUTE_DEFINE( thickness, "Overall thickness of the fog. A higher thickness makes the fog more intense close up to the camera, making god ray shadows pop more." )

		VOLUME_ATTRIBUTE_DEFINE( lightDirectionality, "Scattering directionality for the sun and dynamic local lights. A higher value causes the fog to light up only when looking directly towards light sources." )

		VOLUME_ATTRIBUTE_DEFINE( environmentIntensity, "The visibility of the skybox behind the fog, blurred by the directionality setting above." )
		VOLUME_ATTRIBUTE_DEFINE( environmentDirectionality, "Scattering directionality for the environment map. A lower value causes a more uniform environment color, while a higher value causes the fog to light up only when looking directly at bright areas of the environment map." )

		VOLUME_ATTRIBUTE_DEFINE( fogColor, "The color of the fog itself. Changing this will change what light is scattered from the sun, dynamic lights and the fog." )
		VOLUME_ATTRIBUTE_DEFINE( backgroundVisibility, "How transparent the fog is. Increasing this causes the skybox and objects to become visible, no matter how high the fog thickness is set." )

		VOLUME_ATTRIBUTE_DEFINE( godRayNoiseIntensity, "The intensity of the 2D noise added to the incoming sun light. This adds god rays even when there are no shadows being cast by objects." )
		VOLUME_ATTRIBUTE_DEFINE( godRayNoiseFrequency, "The frequency of the 2D noise added to the incoming sun light. A higher value makes the god rays larger and less detailed. Around 15.0 is a good starting point." )
		VOLUME_ATTRIBUTE_DEFINE( godRayNoiseAnimationSpeed, "The animation speed of the 2D noise added to the incoming sun light. Setting this above 0.0 makes the god rays shift and morph over time." )

		VOLUME_ATTRIBUTE_DEFINE( fogNoiseIntensity, "The intensity of the 3D noise used to modify the fog color. Setting this to 0 gives you a uniform fog, while values above 0.0 will add more variation to the fog color, similar to puffy clouds." )
		VOLUME_ATTRIBUTE_DEFINE( fogNoiseFrequency, "The frequency of the 3D noise used to modify the fog color. A higher value makes the cloud puffs larger and less detailed, while a smaller value makes the puffs smaller and more numerous. Around 15.0 is a good starting point." )

		MAP_ATTRIBUTE( "boundingSphereCenter", m_boundingSphere.center, "", Be::READ )
		MAP_ATTRIBUTE( "boundingSphereRadius", m_boundingSphere.radius, "", Be::READ )
		MAP_ATTRIBUTE( "volumes", m_volumes, "", Be::READ | Be::PERSIST )

	EXPOSURE_END()
}
