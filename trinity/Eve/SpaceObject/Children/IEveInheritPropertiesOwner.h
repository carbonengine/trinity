// Copyright © 2022 CCP ehf.

////////////////////////////////////////////////////////////
//
//    Created:   Jan 2022
//

#pragma once

BLUE_INTERFACE( IEveInheritPropertiesOwner ) : public IRoot
{
	virtual void SetInheritProperties( const Color* colorSet ) = 0;
};
