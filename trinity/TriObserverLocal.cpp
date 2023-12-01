#include "StdAfx.h"
#include "Audio/ITr2AudEmitter.h"
#include "TriObserverLocal.h"


TriObserverLocal::TriObserverLocal( IRoot* lockobj ) :
	m_front( 0.0f, 0.0f, 1.0f ),
	m_position( 0.0f, 0.0f, 0.0f )
{}

TriObserverLocal::~TriObserverLocal()
{}

void TriObserverLocal::Update( const Matrix& worldTransform )
{
	if( m_observer )
	{
		Vector3 front, up, position;
		position = TransformCoord( m_position, worldTransform );
		front = TransformNormal( m_front, worldTransform );
		up = TransformNormal( Vector3( 0.0f, 1.0f, 0.0f ), worldTransform );
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

void TriObserverLocal::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	if ( auto tmp = dynamic_cast< ITr2DebugRenderable* > ( m_observer.p ) )
	{
		tmp->GetDebugOptions( options );
	}
}

void TriObserverLocal::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	auto tmp = dynamic_cast< ITr2DebugRenderable* > ( m_observer.p );
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
