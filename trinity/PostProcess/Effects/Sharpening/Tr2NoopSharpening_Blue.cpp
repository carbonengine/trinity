////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//

#include "StdAfx.h"
#include "Tr2NoopSharpening.h"

BLUE_DEFINE( Tr2NoopSharpening );

const Be::ClassInfo* Tr2NoopSharpening::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2NoopSharpening, "" )
		MAP_INTERFACE( Tr2NoopSharpening )
		MAP_INTERFACE( ITr2Sharpening )

	EXPOSURE_END( )
}
