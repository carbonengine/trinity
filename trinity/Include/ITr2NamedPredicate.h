////////////////////////////////////////////////////////////
//
//    Created:   July 2019
//    Copyright: CCP 2019
//

#pragma once

BLUE_INTERFACE( ITr2NamedPredicate ): public IRoot
{
	virtual bool GetPredicate( const char* name ) const = 0;
};