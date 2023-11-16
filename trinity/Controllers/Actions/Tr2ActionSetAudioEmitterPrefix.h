////////////////////////////////////////////////////////////
//
//    Created:   February 2022
//    Copyright: CCP 2022
//

#pragma once

#include "ITr2ControllerAction.h"


BLUE_CLASS( Tr2ActionSetAudioEmitterPrefix ) : public ITr2ControllerAction
{
public:
	Tr2ActionSetAudioEmitterPrefix( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	void Start( Tr2Controller& controller ) override;
	void StartWithController( PyObject * controller );
private:
	std::string m_emitterName;
	std::wstring m_prefix;
};

TYPEDEF_BLUECLASS( Tr2ActionSetAudioEmitterPrefix );
