////////////////////////////////////////////////////////////////////////////////
//
// Created:		June 2021
// Copyright:	CCP 2021
//
#include "StdAfx.h"
#include "EveComponentRegistry.h"

BLUE_DEFINE( EveComponentRegistry );

const Be::ClassInfo* EveComponentRegistry::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveComponentRegistry, "" )
		MAP_INTERFACE( EveComponentRegistry )
		MAP_METHOD_AND_WRAP( "GetComponentInfo", GetComponentInfo, "Returns information about the components registered" )
	EXPOSURE_END()
}
