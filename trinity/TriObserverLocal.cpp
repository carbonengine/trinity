// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include <ITr2AudEmitter.h>
#include "TriObserverLocal.h"


TriObserverLocal::TriObserverLocal( IRoot* lockobj ) :
	m_front( 0.0f, 0.0f, 1.0f ),
	m_position( 0.0f, 0.0f, 0.0f ),
	m_mute( false )
{
}

TriObserverLocal::~TriObserverLocal()
{
}

void TriObserverLocal::Update( const Matrix& worldTransform )
{
	if( m_observer )
	{
		Vector3 front, up, position;
		position = TransformCoord( m_position, worldTransform );
		front = TransformNormal( m_front, worldTransform );

		const float epsilon = 1.0e-10f;

		// Sometimes with get world transforms with zero scale, which can cause the front vector to be zero.
		// Audio2 doesn't like that at all, so we send an arbitrary front vector in that case.
		if( LengthSq( front ) < epsilon )
		{
			front = Vector3( 0.0f, 0.0f, 1.0f );
			up = Vector3( 0.0f, 1.0f, 0.0f );
		}
		else
		{
			up = TransformNormal( Vector3( 0.0f, 1.0f, 0.0f ), worldTransform );
		}
		m_observer.p->UpdatePlacement( front, up, position );
	}
}

IBluePlacementObserver* TriObserverLocal::GetObserver()
{
	return m_observer;
}

void TriObserverLocal::SetObserver( IBluePlacementObserver* obs )
{
	m_observer = obs;
}

void TriObserverLocal::SetPosition( Vector3 pos )
{
	m_position = pos;
}

void TriObserverLocal::SetFront( const Vector3& front )
{
	m_front = front;
}

bool TriObserverLocal::GetMute()
{
	return m_mute;
}

void TriObserverLocal::SetMute( bool isMute )
{
	if( m_mute == isMute )
	{
		return;
	}
	m_mute = isMute;
	ITr2AudEmitterPtr emitter;
	if( m_observer != nullptr )
	{
		if( ITr2AudEmitterPtr audEmitter = dynamic_cast<ITr2AudEmitter*>( m_observer.p ) )
		{
			emitter = audEmitter;
		}
	}

	if( emitter != nullptr )
	{
		m_mute ? emitter->Mute() : emitter->Unmute();
	}
}

bool TriObserverLocal::OnModified( Be::Var* val )
{
	return true;
}

void TriObserverLocal::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	if( auto tmp = dynamic_cast<ITr2DebugRenderable*>( m_observer.p ) )
	{
		tmp->GetDebugOptions( options );
	}
}

void TriObserverLocal::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	auto tmp = dynamic_cast<ITr2DebugRenderable*>( m_observer.p );
	if( tmp )
	{
		tmp->RenderDebugInfo( renderer );
	}
}


//-----------------------------------------------------
// Description:
//   Send an audio event to an observer if it is not a nullptr and can be cast to ITr2AudEmitter.
// Arguments:
//   observer - The observer that you want to send an audio event to.
//   audioEvent - The audio event you want to be sent to the sound engine.
//-----------------------------------------------------
void SendEventToAudEmitter( TriObserverLocal* observer, const std::wstring& audioEvent )
{
	if( observer != nullptr )
	{
		if( ITr2AudEmitter* emitter = dynamic_cast<ITr2AudEmitter*>( observer->GetObserver() ) )
		{
			emitter->SendEvent( audioEvent );
		}
	}
}
