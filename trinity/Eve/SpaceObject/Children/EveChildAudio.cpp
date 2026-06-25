// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildAudio.h"


EveChildAudio::EveChildAudio( IRoot* lockobj ) :
	m_name( "EveChildAudio" ),
	m_mute( false ),
	m_worldTransform( IdentityMatrix() )
{
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
			emitterName = m_name;
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
			SetEmitterName( m_name );
		}
	}
	return true;
}

const char* EveChildAudio::GetName() const
{
	return m_name.c_str();
}

void EveChildAudio::SetName( const char* name )
{
	m_name = name;
}

void EveChildAudio::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

void EveChildAudio::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
}

void EveChildAudio::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
}

bool EveChildAudio::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	return false;
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

void EveChildAudio::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
}

void EveChildAudio::SetEmitterName( const std::string& name )
{
	if( m_audioEmitter )
	{
		m_audioEmitter->SetName( name.c_str() );
	}
}

void EveChildAudio::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{
}

void EveChildAudio::ChangeLOD( Tr2Lod lod )
{
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