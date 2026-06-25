// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2SpotLight.h"
#include "Tr2Light.h"

BLUE_DEFINE( Tr2SpotLight );

const Be::ClassInfo* Tr2SpotLight::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2SpotLight, "" )
		MAP_INTERFACE( Tr2SpotLight )
		MAP_INTERFACE( Tr2Light )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "Name so artists dont get confused", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "flags", m_lightData.flags, "Various light options", Be::READWRITE | Be::PERSIST, Tr2LightFlagChooser )
		MAP_ATTRIBUTE( "position", m_lightData.position, "Light position", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_lightData.rotation, "Light rotation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_lightData.boneIndex, "The bone index that this light is connected to\n:jessica-widget: boneindex", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "radius", m_lightData.radius, "Outer radius of the spotlight (length)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "innerRadius", m_lightData.innerRadius, "Inner radius of the spotlight (determines fuzzyness)", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "innerAngle", m_lightData.innerAngle, "Inner angle of the spotlight (in degrees)", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "outerAngle", m_lightData.outerAngle, "Outer angle of the spotlight (in degrees)", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "color", m_lightData.color, "Light color (in linear space)\n:jessica-tuple-type: linearcolor", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "brightness", m_lightData.brightness, "Light brightness (modulates color) for easier animation", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "noiseAmplitude", m_lightData.noiseAmplitude, "Brightness noise amplitude\n:jessica-group: Noise", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseFrequency", m_lightData.noiseFrequency, "Brightness noise frequency\n:jessica-group: Noise", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseOctaves", m_lightData.noiseOctaves, "Brightness turbulence octaves\n:jessica-group: Noise", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE_WITH_CHOOSER( "castsShadows", m_lightData.castsShadows, "Casts shadows. If the light is also volumetric, it will create godrays around the affected objects.", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, PerLightShadowSettingChooser );
		MAP_ATTRIBUTE( "isVolumetric", m_lightData.isVolumetric, "Volumetric lights affect participating media such as fog.", Be::READWRITE | Be::NOTIFY | Be::PERSIST )

		MAP_ATTRIBUTE(
			"lightProfilePath",
			m_lightProfilePath,
			"Path to IES profile\n"
			":jessica-widget: filepath\n"
			":jessica-file-filter: ies",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "lightProfile", m_lightProfile, "", Be::READ )
	EXPOSURE_END()
}
