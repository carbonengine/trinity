// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildAudio.h"

BLUE_DEFINE( EveChildAudio );

const Be::ClassInfo* EveChildAudio::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildAudio, "Audio emitter space object child" )
		MAP_INTERFACE( EveChildAudio )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "mute", m_mute, "Mute state", Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE( "audioEmitter", m_audioEmitter, "The audio emitter", Be::READ | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "SetEmitterName", SetEmitterName, "Sets emitter name" )
		MAP_METHOD_AND_WRAP( "__init__", py__init__, "Initialize the audio emitter after contruction/deserialization" )

	EXPOSURE_END()
}