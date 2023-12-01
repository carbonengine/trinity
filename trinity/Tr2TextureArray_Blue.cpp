////////////////////////////////////////////////////////////////////////////////
//
// Created:		June 2022
// Copyright:	CCP 2022
//

#include "StdAfx.h"
#include "Tr2TextureArray.h"

BLUE_DEFINE( Tr2TextureArray );


const Be::ClassInfo* Tr2TextureArray::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TextureArray, "" )
		MAP_INTERFACE( Tr2TextureArray )
		MAP_INTERFACE( ITr2TextureProvider )

		MAP_PROPERTY_READONLY( "width", GetWidth, "Array texture width" )
		MAP_PROPERTY_READONLY( "height", GetHeight, "Array texture height" )
		MAP_PROPERTY_READONLY( "arraySize", GetArraySize, "Number of textures in the array" )
		MAP_PROPERTY_READONLY( "format", GetFormat, "Pixel format\n:jessica-widget: enum\n:jessica-choices: trinity.PixelFormat" )
		MAP_PROPERTY_READONLY( "mipCount", GetMipCount, "Number of mip levels" )

	EXPOSURE_END()
}