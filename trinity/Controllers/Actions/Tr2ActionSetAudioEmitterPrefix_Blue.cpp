////////////////////////////////////////////////////////////
//
//    Created:   February 2022 
//    Copyright: CCP 2022
//

#include "StdAfx.h"
#include "Tr2ActionSetAudioEmitterPrefix.h"


BLUE_DEFINE( Tr2ActionSetAudioEmitterPrefix );


const Be::ClassInfo* Tr2ActionSetAudioEmitterPrefix::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2ActionSetAudioEmitterPrefix, "" )
		MAP_INTERFACE( Tr2ActionSetAudioEmitterPrefix )
		MAP_INTERFACE( ITr2ControllerAction )
		MAP_ATTRIBUTE( "emitter", m_emitterName, "Emitter name", Be::READWRITE | Be::PERSIST );
		MAP_ATTRIBUTE( "prefix", m_prefix, "The prefix to set on this audio emitter, will be prepended to all events sent from the emitter.", Be::READWRITE | Be::PERSIST );
		MAP_METHOD_AND_WRAP(
			"StartWithController",
			StartWithController,
			"Starts the action manually without changing state machine state.\n"
			":param controller: The controller sent to the start method of the action.\n" 
        )
	EXPOSURE_END()
}
