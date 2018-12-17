#include "StdAfx.h"
#include "Tr2TexturedPointLight.h"


BLUE_DEFINE( Tr2TexturedPointLight );

const Be::ClassInfo* Tr2TexturedPointLight::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TexturedPointLight, "" )
		MAP_INTERFACE( Tr2TexturedPointLight )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "texturePath", m_texturePath, ":jessica-group: Texture", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "texture", m_texture, ":jessica-group: Texture", Be::READ | Be::PERSIST )
	EXPOSURE_CHAINTO( Tr2PointLight )
}
