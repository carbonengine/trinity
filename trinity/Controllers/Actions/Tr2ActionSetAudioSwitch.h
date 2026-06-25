// Copyright © 2019 CCP ehf.

#pragma once

#include "ITr2ControllerAction.h"


// A mechanism through which you can set switches in Wwise from a controller.
BLUE_CLASS( Tr2ActionSetAudioSwitch ) :
	public ITr2ControllerAction
{
public:
	Tr2ActionSetAudioSwitch( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	void Start( ITr2ActionController & controller ) override;
	void StartWithController( ITr2ActionController * controller );

private:
	std::string m_emitterName;
	std::wstring m_switchGroup;
	std::wstring m_switchState;
};

TYPEDEF_BLUECLASS( Tr2ActionSetAudioSwitch );
