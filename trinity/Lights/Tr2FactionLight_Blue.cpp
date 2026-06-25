// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2FactionLight.h"

BLUE_DEFINE( Tr2FactionLight );

const Be::ClassInfo* Tr2FactionLight::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2FactionLight, "" )
		MAP_INTERFACE( Tr2FactionLight )
		MAP_INTERFACE( IEveInheritPropertiesOwner )
		MAP_INTERFACE( Tr2Light )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "Name so artists dont get confused", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "flags", m_lightData.flags, "Various light options", Be::READWRITE | Be::PERSIST, Tr2LightFlagChooser )
		MAP_ATTRIBUTE( "position", m_lightData.position, "Light position", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "rotation", m_lightData.rotation, "Light rotation", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "boneIndex", m_lightData.boneIndex, "The bone index that this light is connected to\n:jessica-widget: boneindex", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "radius", m_lightData.radius, "Light radius", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "innerRadius", m_lightData.innerRadius, "Inner light radius (to mimick a glowing sphere)", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "brightness", m_lightData.brightness, "Light brightness (modulates color) for easier animation\n:jessica-group: color", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "saturation", m_saturation, "[0:inf] 0=grayscale 1=normal (output capped so feel free to over-saturate)\n:jessica-group: color", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE_WITH_CHOOSER( "factionColor", m_selectedColor, "Light color\n:jessica-group: color", Be::READWRITE | Be::PERSIST | Be::NOTIFY | Be::ENUM, SOFDataFactionColorChooser::EveSOFDataFactionColorSetTypeChooser )
		MAP_PROPERTY_READONLY( "selectedColor", GetSelectedColor, "Light color helper\n:jessica-group: color" )

		MAP_ATTRIBUTE( "isSpotlight", m_isSpotlight, "if the light behaves as a spotLight or a pointLight\n:jessica-group: SpotlightOptions", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "rotation", m_lightData.rotation, "Light rotation\n:jessica-group: SpotlightOptions", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "innerAngle", m_lightData.innerAngle, "Inner angle of the spotlight (in degrees)\n:jessica-group: SpotlightOptions", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "outerAngle", m_lightData.outerAngle, "Outer angle of the spotlight (in degrees)\n:jessica-group: SpotlightOptions", Be::READWRITE | Be::PERSIST )

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