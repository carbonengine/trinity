#include "StdAfx.h"
#include "Tr2SpotLight.h"
#include "Tr2Light.h"

BLUE_DEFINE( Tr2SpotLight );

const Be::ClassInfo* Tr2SpotLight::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2SpotLight, "" )
		MAP_INTERFACE( Tr2SpotLight )
		MAP_INTERFACE( Tr2Light )

		MAP_ATTRIBUTE( "name", m_name, "Name so artists dont get confused", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "position", m_lightData.position, "Light position", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_lightData.rotation, "Light rotation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "radius", m_lightData.radius, "Outer radius of the spotlight (length)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "innerRadius", m_lightData.innerRadius, "Inner radius of the spotlight", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "innerAngle", m_lightData.innerAngle, "Inner angle of the spotlight (in degrees)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "outerAngle", m_lightData.outerAngle, "Outer angle of the spotlight (in degrees)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "color", m_lightData.color, "Light color (in linear space)\n:jessica-tuple-type: linearcolor", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "brightness", m_lightData.brightness, "Light brightness (modulates color) for easier animation", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "noiseAmplitude", m_lightData.noiseAmplitude, "Brightness noise amplitude\n:jessica-group: Noise", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseFrequency", m_lightData.noiseFrequency, "Brightness noise frequency\n:jessica-group: Noise", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseOctaves", m_lightData.noiseOctaves, "Brightness turbulence octaves\n:jessica-group: Noise", Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
