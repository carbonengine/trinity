// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2ImageRes.h"

BLUE_DEFINE( Tr2ImageRes );

IBlueResource* CreateTr2ImageRes( const wchar_t* name )
{
	Tr2ImageResPtr p;
	p.CreateInstance();
	return p.Detach();
}

BLUE_REGISTER_RESOURCE_EXTENSION( L"pngraw", CreateTr2ImageRes );
BLUE_REGISTER_RESOURCE_EXTENSION( L"ddsraw", CreateTr2ImageRes );
BLUE_REGISTER_RESOURCE_EXTENSION( L"sddraw", CreateTr2ImageRes );
BLUE_REGISTER_RESOURCE_EXTENSION( L"tgaraw", CreateTr2ImageRes );

const Be::ClassInfo* Tr2ImageRes::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ImageRes, "" )

		MAP_INTERFACE( Tr2ImageRes )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_ICACHEABLE_METHODS()

		MAP_PROPERTY_READONLY(
			"width",
			GetWidth,
			"Width of the image (in pixels)" )

		MAP_PROPERTY_READONLY(
			"height",
			GetHeight,
			"Height of the image (in pixels)" )

		MAP_METHOD_AND_WRAP(
			"IsPixelOpaque",
			IsPixelOpaque,
			"Returns true if the alpha value for the given pixel is > 0.5."
			"\n:param x: x-coordinate of pixel"
			"\n:param y: y-coordinate of pixel" )

		MAP_METHOD_AND_WRAP(
			"GetPixelColor",
			GetPixelColor,
			"Returns the color value of the given pixel."
			"\n:param x: x-coordinate of pixel"
			"\n:param y: y-coordinate of pixel" )
	EXPOSURE_CHAINTO( BlueAsyncRes )
}
