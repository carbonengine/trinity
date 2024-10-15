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
			"directionality",
			m_settings.value.directionality,
			"Scattering directionality of the fog. A higher value causes the fog to light up only when looking directly towards light sources.\n"
			":jessica-group: Froxel Fog",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"environmentIntensity",
			m_settings.value.environmentIntensity,
			"The visibility of the skybox behind the fog, blurred by the directionality setting above.\n"
			":jessica-group: Froxel Fog",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"fogColor",
			m_settings.value.fogColor,
			"The color of the fog itself. Changing this will change what light is scattered from the sun, dynamic lights and the fog.\n"
			":jessica-group: Froxel Fog",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"backgroundColor",
			m_settings.value.backgroundColor,
			"How transparent the fog is to the background, controllable for RGB separately. When set to (0, 0, 0), the background is completely blocked by the fog. \n"
			":jessica-group: Froxel Fog",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "boundingSphereCenter", m_boundingSphere.center, "", Be::READ )
		MAP_ATTRIBUTE( "boundingSphereRadius", m_boundingSphere.radius, "", Be::READ )
		MAP_ATTRIBUTE( "volumes", m_volumes, "", Be::READ | Be::PERSIST )

	EXPOSURE_END()
}
