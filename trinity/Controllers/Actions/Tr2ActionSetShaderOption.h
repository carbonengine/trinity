// Copyright © 2018 CCP ehf.

#pragma once

#include "ITr2ControllerAction.h"

BLUE_CLASS( Tr2ActionSetShaderOption ) :
	public ITr2ControllerAction
{
public:
	EXPOSE_TO_BLUE();

	void Start( ITr2ActionController & controller ) override;

private:
	BlueSharedString m_optionKey;
	BlueSharedString m_optionValue;
};

TYPEDEF_BLUECLASS( Tr2ActionSetShaderOption );
