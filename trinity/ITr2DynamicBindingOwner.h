// Copyright © 2019 CCP ehf.

#pragma once

BLUE_INTERFACE( ITr2DynamicBindingOwner ) :
	public IRoot
{
	virtual std::unordered_map<std::string, IRoot*> GetParameterMap() const = 0;
};
