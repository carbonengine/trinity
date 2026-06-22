// Copyright © 2019 CCP ehf.

////////////////////////////////////////////////////////////
//
//    Created:   May 2019
//

#pragma once

BLUE_INTERFACE( ITr2DynamicBindingOwner ) : public IRoot
{
	virtual std::unordered_map<std::string, IRoot*> GetParameterMap() const = 0;
};
