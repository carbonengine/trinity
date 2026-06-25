// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if GSTATE_ENABLED

#include "Tr2GrannyStateRes.h"

BLUE_DEFINE( Tr2GrannyStateRes );

IBlueResource* CreateTr2GrannyStateRes( const wchar_t* name )
{
	Tr2GrannyStateResPtr p;
	p.CreateInstance();
	return p.Detach();
}

BLUE_REGISTER_RESOURCE_EXTENSION( L"gsfraw", CreateTr2GrannyStateRes );

const Be::ClassInfo* Tr2GrannyStateRes::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2GrannyStateRes, "" )

		MAP_INTERFACE( Tr2GrannyStateRes )
		MAP_INTERFACE( IBlueResource )
		MAP_INTERFACE( ICacheable )
		MAP_ICACHEABLE_METHODS()


	EXPOSURE_CHAINTO( BlueAsyncRes )
}

#endif