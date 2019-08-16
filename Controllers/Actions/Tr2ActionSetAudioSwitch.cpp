////////////////////////////////////////////////////////////
//
//    Created:   June 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "Tr2ActionSetAudioSwitch.h"
#include "Controllers/Tr2Controller.h"
#include "ITr2SoundEmitterOwner.h"
#include "Include/ITr2SoundEmitter.h"


Tr2ActionSetAudioSwitch::Tr2ActionSetAudioSwitch( IRoot* )
{
}

void Tr2ActionSetAudioSwitch::Start( Tr2Controller& controller )
{
	if( ITr2SoundEmitterOwnerPtr emitters = BlueCastPtr( controller.GetOwner() ) )
	{
		if( auto emitter = emitters->FindSoundEmitter( m_emitterName.c_str() ) )
		{
			emitter->SetSwitch( m_switchGroup.c_str(), m_switchState.c_str() );
		}
	}
}
