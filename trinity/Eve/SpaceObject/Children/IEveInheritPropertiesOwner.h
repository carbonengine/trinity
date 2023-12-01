////////////////////////////////////////////////////////////
//
//    Created:   Jan 2022
//    Copyright: CCP 2022
//

#pragma once

BLUE_INTERFACE( IEveInheritPropertiesOwner ) : public IRoot
{
	virtual void SetInheritProperties( const Color* colorSet ) = 0;
};
