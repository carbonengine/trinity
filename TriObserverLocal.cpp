#include "StdAfx.h"
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

void TriObserverLocal::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	if ( auto tmp = dynamic_cast< ITr2DebugRenderable* > ( m_observer.p ) )
	{
		tmp->GetDebugOptions( options );
	}
}

void TriObserverLocal::RenderDebugInfo( ITr2DebugRenderer2& renderer, Matrix& parentWorldLocation )
{
	auto tmp = dynamic_cast< ITr2DebugRenderable* > ( m_observer.p );
	if( tmp )
	{
		tmp->RenderDebugInfo( renderer );
	}
}