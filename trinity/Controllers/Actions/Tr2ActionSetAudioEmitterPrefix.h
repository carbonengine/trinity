// Copyright © 2022 CCP ehf.

#pragma once

#include "ITr2ControllerAction.h"


BLUE_CLASS( Tr2ActionSetAudioEmitterPrefix ) :
	public ITr2ControllerAction
{
public:
	Tr2ActionSetAudioEmitterPrefix( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	void Start( ITr2ActionController & controller ) override;
	void StartWithController( ITr2ActionController * controller );

private:
	std::string m_emitterName;
	std::wstring m_prefix;
};

TYPEDEF_BLUECLASS( Tr2ActionSetAudioEmitterPrefix );
