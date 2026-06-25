// Copyright © 2022 CCP ehf.

#pragma once

BLUE_INTERFACE( IEveInheritPropertiesOwner ) :
	public IRoot
{
	virtual void SetInheritProperties( const Color* colorSet ) = 0;
};
