////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2022
// Copyright:	CCP 2022
//
#pragma once

BLUE_DECLARE( Tr2LightManager );
BLUE_DECLARE( Tr2Light );

BLUE_INTERFACE( ITr2LightOwner ) :
	public IRoot
{
	virtual void GetLights( Tr2LightManager & lightManager ) const = 0;
	virtual void AddLight( Tr2Light * light ) = 0;
	virtual void ClearLights() = 0;
};
