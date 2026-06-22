// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2ActionSetAudioSwitch.h"
#include "../ITr2Controller.h"


BLUE_DEFINE( Tr2ActionSetAudioSwitch );


const Be::ClassInfo* Tr2ActionSetAudioSwitch::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ActionSetAudioSwitch, "" )
		MAP_INTERFACE( Tr2ActionSetAudioSwitch )
		MAP_INTERFACE( ITr2ControllerAction )
		MAP_ATTRIBUTE( "emitter", m_emitterName, "Emitter name", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "switchGroup", m_switchGroup, "The switch group to apply the switch state to", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "switchState", m_switchState, "The switch state to apply to the emitter", Be::READWRITE | Be::PERSIST );
		MAP_METHOD_AND_WRAP(
			"StartWithController",
			StartWithController,
			"Starts the action manually without changing state machine state.\n"
			":type controller: Tr2Controller\n"
			":param controller: The controller sent to the start method of the action.\n" )
	EXPOSURE_END()
}
