////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2021
// Copyright:	CCP 2021
//
#pragma once

#include "ITr2Renderable.h"

BLUE_INTERFACE( IEveLightOwner ) :
	public IRoot
{
	virtual void GetLightsFromOwner( Tr2LightManager & lightManager ) const = 0;
};
