////////////////////////////////////////////////////////////
//
//    Created:   October 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2ActionPlaySound.h"


BLUE_DEFINE( Tr2ActionPlaySound );


const Be::ClassInfo* Tr2ActionPlaySound::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ActionPlaySound, "" )
		MAP_INTERFACE( Tr2ActionPlaySound )
		MAP_INTERFACE( ITr2ControllerAction )
		MAP_ATTRIBUTE( "emitter", m_emitterName, "Emitter name", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "event", m_soundEvent, "Sound event name", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "target", m_target, "The target object whose audio emitters you will be sending a sound to, defaults to the controller's owner.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "bypassPrefix", m_bypassPrefix, "Determines whether the audio emitter prefix is bypassed when sending this event.", Be::READWRITE | Be::PERSIST );
		MAP_METHOD_AND_WRAP(
			"StartWithController",
			StartWithController,
			"Starts the action manually without changing state machine state.\n"
			":type controller: Tr2Controller\n"
			":param controller: The controller sent to the start method of the action.\n" )
	EXPOSURE_END()
}
