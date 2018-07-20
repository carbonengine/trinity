////////////////////////////////////////////////////////////
//
//    Created:   July 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2CairoScriptSourceRes.h"


BLUE_DEFINE( Tr2CairoScriptSourceRes );


IBlueResource* CreateTr2CairoScriptSourceRes( const wchar_t* name )
{
	Tr2CairoScriptSourceResPtr p;
	p.CreateInstance();
	return p.Detach();
}

BLUE_REGISTER_RESOURCE_EXTENSION( L"ecsraw", CreateTr2CairoScriptSourceRes );


const Be::ClassInfo* Tr2CairoScriptSourceRes::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2CairoScriptSourceRes, "" )

		MAP_INTERFACE( Tr2CairoScriptSourceRes )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )

		MAP_PROPERTY_READONLY( "width", GetWidth, "source image width" )
		MAP_PROPERTY_READONLY( "height", GetHeight, "source image height" )

		MAP_ICACHEABLE_METHODS()
	EXPOSURE_CHAINTO( BlueAsyncRes )
}
