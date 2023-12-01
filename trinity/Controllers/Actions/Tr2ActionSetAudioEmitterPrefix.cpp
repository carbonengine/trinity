////////////////////////////////////////////////////////////
//
//    Created:   February 2022
//    Copyright: CCP 2022
//

#include "StdAfx.h"
#include "Tr2ActionSetAudioEmitterPrefix.h"
#include "Controllers/Tr2Controller.h"
#include "ITr2SoundEmitterOwner.h"
#include "Audio/ITr2AudEmitter.h"


Tr2ActionSetAudioEmitterPrefix::Tr2ActionSetAudioEmitterPrefix( IRoot* )
{
}

void Tr2ActionSetAudioEmitterPrefix::Start( Tr2Controller& controller )
{
	if( ITr2SoundEmitterOwnerPtr emitters = BlueCastPtr( controller.GetOwner() ) )
	{
		if( auto emitter = emitters->FindSoundEmitter( m_emitterName.c_str() ) )
		{
			emitter->SetPrefix( m_prefix );
		}
	}
}

void Tr2ActionSetAudioEmitterPrefix::StartWithController( PyObject* obj )
{
	Tr2Controller* controller = BluePythonCast<Tr2Controller*>( obj );
	if( !controller )
	{
		PyErr_SetString( PyExc_TypeError, "StartWithController expects a Tr2Controller as a parameter." );
		return;
	}
	Start( *controller );
}