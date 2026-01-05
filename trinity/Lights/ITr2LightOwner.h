////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2022
// Copyright:	CCP 2022
//
#pragma once

#include "Eve/EveComponentRegistry.h"

BLUE_DECLARE( Tr2LightManager );
BLUE_DECLARE( Tr2Light );

BLUE_INTERFACE( ITr2LightOwner ) :
	public IRoot
{
	virtual void GetLights( Tr2LightManager& lightManager ) const = 0;
	virtual void AddLight( Tr2Light* light ) {};
	virtual void ClearLights() {};
};

REGISTER_COMPONENT_TYPE( "LightOwner", ITr2LightOwner );