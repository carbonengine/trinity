// Copyright © 2023 CCP ehf.

#pragma once

#include "ITr2StateMachineStateFinalizer.h"


BLUE_CLASS( Tr2SyncToAnimation ) :
	public ITr2StateMachineStateFinalizer
{
public:
	EXPOSE_TO_BLUE();

	bool CanTransition( Tr2Controller & controller ) const;

private:
	std::string m_mask;
};

TYPEDEF_BLUECLASS( Tr2SyncToAnimation );