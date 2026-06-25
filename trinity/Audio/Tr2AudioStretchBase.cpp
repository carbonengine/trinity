// Copyright © 2020 CCP ehf.

#include "StdAfx.h"
#include "Tr2AudioStretchBase.h"
#include "Tr2Renderer.h"

Tr2AudioStretchBase::Tr2AudioStretchBase( IRoot* lockobj )
{
	Initialize();
}

Tr2AudioStretchBase::~Tr2AudioStretchBase()
{
}

bool Tr2AudioStretchBase::Initialize()
{
	// If emitters don't already exist, create and set default names
	if( nullptr == m_sourceEmitter )
	{
		BeClasses->CreateInstanceFromName( "AudEmitter", BlueInterfaceIID<ITr2AudEmitter>(), reinterpret_cast<void**>( &m_sourceEmitter.p ) );
		m_sourceEmitter.p->SetName( "stretch_source_sfx" );
	}
	if( nullptr == m_destEmitter )
	{
		BeClasses->CreateInstanceFromName( "AudEmitter", BlueInterfaceIID<ITr2AudEmitter>(), reinterpret_cast<void**>( &m_destEmitter.p ) );
		m_destEmitter.p->SetName( "stretch_dest_sfx" );
	}
	if( nullptr == m_stretchEmitter )
	{
		BeClasses->CreateInstanceFromName( "AudEmitter", BlueInterfaceIID<ITr2AudEmitter>(), reinterpret_cast<void**>( &m_stretchEmitter.p ) );
		m_stretchEmitter.p->SetName( "stretch_mid_sfx" );
	}

	return true;
}

void Tr2AudioStretchBase::Update( Vector3& sourcePosition, Vector3& destPosition )
{
	Vector3 front( 0, 1, 0 ), top( 0, 0, 1 );
	if( nullptr != m_sourceEmitter )
	{
		m_sourceEmitter->SetPosition( front, top, sourcePosition );
	}
	if( nullptr != m_destEmitter )
	{
		m_destEmitter->SetPosition( front, top, destPosition );
	}
	if( nullptr != m_stretchEmitter )
	{
		// Place the stretch audio emitter between the source and destination points and
		// follow the camera as long as it is between source and dest.
		Vector3 cameraPos = Tr2Renderer::GetViewPosition();
		Vector3 sp = cameraPos - sourcePosition;
		Vector3 dp = destPosition - sourcePosition;
		Vector3 offset = Dot( sp, dp ) / Dot( dp, dp ) * dp;
		offset = ClampLength( offset, Length( dp ) );
		Vector3 dest = sourcePosition;
		dest = Dot( dp, dest + offset ) > 0 ? dest + offset : dest;
		m_stretchEmitter->SetPosition( front, top, dest );
	}
}

ITr2AudEmitterPtr Tr2AudioStretchBase::FindEmitterByName( const char* name )
{
	if( m_sourceEmitter->GetName() == name )
	{
		return m_sourceEmitter;
	}
	else if( m_destEmitter->GetName() == name )
	{
		return m_destEmitter;
	}
	else if( m_stretchEmitter->GetName() == name )
	{
		return m_stretchEmitter;
	}
	return nullptr;
}

void Tr2AudioStretchBase::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	if( auto tmp = dynamic_cast<ITr2DebugRenderable*>( m_sourceEmitter.p ) )
	{
		tmp->GetDebugOptions( options );
	}
	if( auto tmp = dynamic_cast<ITr2DebugRenderable*>( m_destEmitter.p ) )
	{
		tmp->GetDebugOptions( options );
	}
	if( auto tmp = dynamic_cast<ITr2DebugRenderable*>( m_stretchEmitter.p ) )
	{
		tmp->GetDebugOptions( options );
	}
}

void Tr2AudioStretchBase::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( auto tmp = dynamic_cast<ITr2DebugRenderable*>( m_sourceEmitter.p ) )
	{
		tmp->RenderDebugInfo( renderer );
	}
	if( auto tmp = dynamic_cast<ITr2DebugRenderable*>( m_destEmitter.p ) )
	{
		tmp->RenderDebugInfo( renderer );
	}
	if( auto tmp = dynamic_cast<ITr2DebugRenderable*>( m_stretchEmitter.p ) )
	{
		tmp->RenderDebugInfo( renderer );
	}
}
