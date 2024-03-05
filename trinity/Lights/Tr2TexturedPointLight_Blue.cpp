#include "StdAfx.h"
#include "Tr2TexturedPointLight.h"

BLUE_DEFINE( Tr2TexturedPointLight );


const Be::ClassInfo* Tr2TexturedPointLight::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TexturedPointLight, "" )
		MAP_INTERFACE( Tr2TexturedPointLight )
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
		
		MAP_ATTRIBUTE( "color", m_lightData.color, "Light color (in linear space)\n:jessica-tuple-type: linearcolor", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "brightness", m_lightData.brightness, "Light brightness (modulates color) for easier animation", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "noiseAmplitude", m_lightData.noiseAmplitude, "Brightness noise amplitude\n:jessica-group: Noise", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseFrequency", m_lightData.noiseFrequency, "Brightness noise frequency\n:jessica-group: Noise", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "noiseOctaves", m_lightData.noiseOctaves, "Brightness turbulence octaves\n:jessica-group: Noise", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "texturePath", m_lightData.texturePath, ":jessica-group: Texture", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "texture", m_texture, ":jessica-group: Texture", Be::READ )

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
