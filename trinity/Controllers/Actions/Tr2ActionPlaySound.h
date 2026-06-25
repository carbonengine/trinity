// Copyright © 2018 CCP ehf.

#pragma once

#include "ITr2ControllerAction.h"


BLUE_CLASS( Tr2ActionPlaySound ) :
	public ITr2ControllerAction
{
public:
	Tr2ActionPlaySound( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	void Start( ITr2ActionController & controller ) override;
	void StartWithController( PyObject * controller );

private:
	BlueSharedString m_emitterName;
	BlueSharedString m_soundEvent;
	BlueSharedString m_target;
	bool m_bypassPrefix;
};

TYPEDEF_BLUECLASS( Tr2ActionPlaySound );
