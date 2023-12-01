#include "StdAfx.h"
#include "Tr2TextureReference.h"

BLUE_DEFINE( Tr2TextureReference );


const Be::ClassInfo* Tr2TextureReference::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2TextureReference, "" )
        MAP_INTERFACE( Tr2TextureReference )
        MAP_INTERFACE( ITr2TextureProvider )

		MAP_PROPERTY_READONLY( "width", GetWidth, "Texture width" )
		MAP_PROPERTY_READONLY( "height", GetHeight, "Texture height" )
		MAP_PROPERTY_READONLY( "depth", GetDepth, "Texture depth" )
		MAP_PROPERTY_READONLY( "type", GetType, "Texture type" )
		MAP_PROPERTY_READONLY( "mipCount", GetMipCount, "Number of mip levels" )
		MAP_PROPERTY_READONLY( "format", GetFormat, "Texture pixel format" )
		MAP_PROPERTY_READONLY( "arraySize", GetArraySize, "Number of textures in the array" )

		MAP_METHOD_AND_WRAP( 
			"Save", 
			Save, 
			"Saves referenced texture to disk.\n"
			":param path: res path to the file"
		)
    EXPOSURE_END()
}



BLUE_DEFINE( Tr2TransientTextureReference );


const Be::ClassInfo* Tr2TransientTextureReference::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TransientTextureReference, "" )
		MAP_INTERFACE( Tr2TransientTextureReference )
		MAP_INTERFACE( ITr2TextureProvider )
	EXPOSURE_END()
}
