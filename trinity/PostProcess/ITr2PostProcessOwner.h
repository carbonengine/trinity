#pragma once

#include "Eve/EveComponentRegistry.h"
#include "PostProcess/Tr2PostProcessUtils.h"

BLUE_INTERFACE( ITr2PostProcessOwner ) 
{
public:

	virtual PostProcess::Attributes& GetPostProcessAttributes() = 0;
};

REGISTER_COMPONENT_TYPE( "PostProcessOwner", ITr2PostProcessOwner );
