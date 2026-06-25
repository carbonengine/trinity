// Copyright © 2022 CCP ehf.

#include "StdAfx.h"
#include "Tr2LightProfileRes.h"
#include "../Tr2HostBitmap.h"

BLUE_DEFINE( Tr2LightProfileRes );

namespace
{
IBlueResource* CreateTr2LightProfileRes( const wchar_t* name )
{
	Tr2LightProfileResPtr p;
	p.CreateInstance();
	return p.Detach();
}

BlueStdResult BakeLightProfile( const wchar_t* path, Tr2HostBitmapPtr& output )
{
	return Tr2LightProfileRes::BakeLightProfile( path, output );
}

}

BLUE_REGISTER_RESOURCE_EXTENSION( L"ddslp", CreateTr2LightProfileRes );
BLUE_REGISTER_RESOURCE_EXTENSION( L"ieslp", CreateTr2LightProfileRes );


const Be::ClassInfo* Tr2LightProfileRes::ExposeToBlue(){
	EXPOSURE_BEGIN( Tr2LightProfileRes, "" )

		MAP_INTERFACE( Tr2LightProfileRes )
			MAP_INTERFACE( IBlueResource )
				MAP_INTERFACE( ICacheable )
					MAP_ICACHEABLE_METHODS()

						MAP_METHOD_AND_WRAP(
							"GetThumbnail",
							GetThumbnail,
							"Generates a thumbnail for the profile. Should only be used for tools.\n"
							":param width: thumbnail width\n"
							":param height: thumbnail height" )

							EXPOSURE_CHAINTO( BlueAsyncRes )
}


MAP_FUNCTION_AND_WRAP(
	"BakeLightProfile",
	BakeLightProfile,
	"Bakes light profile from an IES file into a texture\n"
	":param path: path to .ies file" );