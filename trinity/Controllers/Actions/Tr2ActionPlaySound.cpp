// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2ActionPlaySound.h"
#include "Controllers/Tr2Controller.h"
#include "ITr2SoundEmitterOwner.h"
#include <ITr2AudEmitter.h>
#include "Eve/EveMultiEffect.h"
#include "Eve/EveMultiEffectParameter.h"
#include "Eve/SpaceObject/Children/IEveEffectChildrenOwner.h"


Tr2ActionPlaySound::Tr2ActionPlaySound( IRoot* ) :
	m_bypassPrefix( false ),
	m_target( "" )
{
}

void Tr2ActionPlaySound::Start( ITr2ActionController& controller )
{
	ITr2SoundEmitterOwnerPtr owner = BlueCastPtr( controller.GetOwner() );

	if( !m_target.empty() )
	{
		if( EveMultiEffectPtr multiEffect = BlueCastPtr( controller.GetOwner() ) )
		{
			if( EveMultiEffectParameterPtr mep = multiEffect->GetParameterByName( m_target ) )
			{
				owner = BlueCastPtr( mep->GetParameterObject() );
			}
		}
		else if( IEveEffectChildrenOwnerPtr childEffectOwner = BlueCastPtr( controller.GetOwner() ) )
		{
			owner = BlueCastPtr( childEffectOwner->GetEffectChildByName( m_target.c_str() ) );
		}
	}

	if( owner != nullptr )
	{
		if( auto emitter = owner->FindSoundEmitter( m_emitterName.c_str() ) )
		{
			emitter->SendEvent( static_cast<const wchar_t*>( CA2W( m_soundEvent.c_str() ) ), m_bypassPrefix );
		}
	}
	else
	{
		CCP_LOGERR( "Tr2ActionPlaySound failed to find an owner! The target must be derived from ITr2SoundEmitterOwner." );
	}
}

void Tr2ActionPlaySound::StartWithController( PyObject* obj )
{
	Tr2Controller* controller = BluePythonCast<Tr2Controller*>( obj );
	if( !controller )
	{
		PyErr_SetString( PyExc_TypeError, "StartWithController expects a Tr2Controller as a parameter." );
		return;
	}
	Start( *controller );
}
