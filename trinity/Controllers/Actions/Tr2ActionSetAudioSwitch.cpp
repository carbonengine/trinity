// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "Tr2ActionSetAudioSwitch.h"
#include "Controllers/Tr2Controller.h"
#include "ITr2SoundEmitterOwner.h"
#include <ITr2AudEmitter.h>


Tr2ActionSetAudioSwitch::Tr2ActionSetAudioSwitch( IRoot* )
{
}

void Tr2ActionSetAudioSwitch::Start( ITr2ActionController& controller )
{
	if( ITr2SoundEmitterOwnerPtr emitters = BlueCastPtr( controller.GetOwner() ) )
	{
		if( auto emitter = emitters->FindSoundEmitter( m_emitterName.c_str() ) )
		{
			emitter->SetSwitch( m_switchGroup.c_str(), m_switchState.c_str() );
		}
	}
}

void Tr2ActionSetAudioSwitch::StartWithController( ITr2ActionController* controller )
{
	if( !controller )
	{
		PyErr_SetString( PyExc_TypeError, "StartWithController expects a Tr2Controller as a parameter." );
		return;
	}
	Start( *controller );
}
