// Copyright © 2019 CCP ehf.

#pragma once

#include "ITr2ControllerAction.h"



BLUE_CLASS( Tr2ActionCallback ) :
	public ITr2ControllerAction
{
public:
	EXPOSE_TO_BLUE();

	void Start( ITr2ActionController & controller ) override;

private:
	BlueSharedString m_callbackName;
};

TYPEDEF_BLUECLASS( Tr2ActionCallback );
