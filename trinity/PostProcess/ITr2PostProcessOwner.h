// Copyright © 2024 CCP ehf.

#pragma once

#include "Eve/EveComponentRegistry.h"
#include "PostProcess/Tr2PostProcessAttributes.h"

BLUE_INTERFACE( ITr2PostProcessOwner ) :
	public IRoot
{
public:
	virtual Tr2PostProcessAttributes* GetPostProcessAttributes() = 0;
};

REGISTER_COMPONENT_TYPE( "PostProcessOwner", ITr2PostProcessOwner );
