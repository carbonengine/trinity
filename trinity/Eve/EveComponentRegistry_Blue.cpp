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
		MAP_PROPERTY_READONLY("lightOwners", GetLightOwnerCount, "How many objects contain light")
		MAP_PROPERTY_READONLY("reflectionRenderable", GetReflectionRenderableCount, "How many objects are registered as reflection renderables")
	EXPOSURE_END()
}
