// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildAudio.h"


EveChildAudio::EveChildAudio( IRoot* lockobj ) :
	m_mute( false ),
	m_worldTransform( IdentityMatrix() )
{
	m_name = BlueSharedString( "EveChildAudio" );
}

bool EveChildAudio::Initialize()
{
	// Early exit if already initialized
	if( m_audioEmitter )
	{
		return true;
	}

	if( SUCCEEDED( BeClasses->CreateInstanceFromName( "AudEmitter", BlueInterfaceIID<ITr2AudEmitter>(), reinterpret_cast<void**>( &m_audioEmitter.p ) ) ) )
	{
		Vector3 position = m_worldTransform.GetTranslation();
		std::string emitterName;

		if( m_name.empty() )
		{
			emitterName = "audio_object";
		}
		else
		{
			emitterName = m_name.c_str();
		}
		m_audioEmitter->Initialize( emitterName, L"", position );

		return true;
	}
	return false;
}

void EveChildAudio::py__init__()
{
	Initialize();
}

bool EveChildAudio::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_mute ) )
	{
		if( m_audioEmitter )
		{
			if( m_mute )
			{
				m_audioEmitter->Mute();
			}
			else
			{
				m_audioEmitter->Unmute();
			}
		}
	}
	else if( IsMatch( val, m_name ) )
	{
		if( m_name.empty() )
		{
			SetEmitterName( "audio_object" );
		}
		else
		{
			SetEmitterName( m_name.c_str() );
		}
	}
	return true;
}

void EveChildAudio::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

void EveChildAudio::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	Matrix localToWorldTransform;
	if( params.childParent )
	{
		params.childParent->GetLocalToWorldTransform( localToWorldTransform );
	}
	else if( params.spaceObjectParent )
	{
		params.spaceObjectParent->GetLocalToWorldTransform( localToWorldTransform );
	}
	else
	{
		localToWorldTransform = IdentityMatrix();
	}
	UpdateTransform( localToWorldTransform );

	if( m_audioEmitter && !m_mute )
	{
		Vector3 position = m_worldTransform.GetTranslation();
		Quaternion rotation = RotationQuaternion( m_worldTransform );
		Matrix rotationMatrix = RotationMatrix( rotation );
		Vector3 front = TransformNormal( Vector3( 0, 1, 0 ), rotationMatrix );
		Vector3 top = TransformNormal( Vector3( 0, 0, 1 ), rotationMatrix );
		m_audioEmitter->SetPosition( front, top, position );
	}
}

void EveChildAudio::SetEmitterName( const std::string& name )
{
	if( m_audioEmitter )
	{
		m_audioEmitter->SetName( name.c_str() );
	}
}

void EveChildAudio::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	if( ITr2DebugRenderablePtr tmp = BlueCastPtr( m_audioEmitter ) )
	{
		tmp->GetDebugOptions( options );
	}
}

void EveChildAudio::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( ITr2DebugRenderablePtr tmp = BlueCastPtr( m_audioEmitter ) )
	{
		tmp->RenderDebugInfo( renderer );
	}
}