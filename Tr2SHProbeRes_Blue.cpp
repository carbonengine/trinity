#include "StdAfx.h"

#include "Tr2SHProbeRes.h"

BLUE_DEFINE( Tr2SHProbeRes );


IBlueResource* CreateTr2SHProbeRes( const wchar_t* name )
{
	Tr2SHProbeResPtr p;
	p.CreateInstance();
	return p.Detach();
}

BLUE_REGISTER_RESOURCE_EXTENSION( L"shp", CreateTr2SHProbeRes );

const Be::ClassInfo* Tr2SHProbeRes::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2SHProbeRes, "" )

		MAP_INTERFACE( Tr2SHProbeRes )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_ICACHEABLE_METHODS()

		EXPOSURE_CHAINTO( BlueAsyncRes )
}
