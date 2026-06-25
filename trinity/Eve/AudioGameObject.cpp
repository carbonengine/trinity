// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "AudioGameObject.h"
#include "Eve/EveUpdateContext.h"

AudioGameObject::AudioGameObject( IRoot* lockobj ) :
	PARENTLOCK( m_externalParameters ),
	m_rotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_translation( 0.0f, 0.0f, 0.0f ),
	m_mute( false ),
	m_display( true )
{
}

bool AudioGameObject::Initialize()
{
	// Early exit if already initialized
	if( m_audioEmitter )
	{
		return true;
	}

	if( SUCCEEDED( BeClasses->CreateInstanceFromName( "AudEmitter", BlueInterfaceIID<ITr2AudEmitter>(), reinterpret_cast<void**>( &m_audioEmitter.p ) ) ) )
	{
		UpdateWorldTransform( Be::Time( 0.0 ) );

		Vector3 position = GetWorldPosition();
		std::string emitterName;
		if( m_name.empty() )
		{
			emitterName = "audio_object";
		}
		else
		{
			emitterName = m_name;
		}
		Quaternion rotation = GetWorldRotation();
		Matrix rotationMatrix = RotationMatrix( rotation );
		Vector3 front = TransformNormal( Vector3( 0, 1, 0 ), rotationMatrix );
		Vector3 top = TransformNormal( Vector3( 0, 0, 1 ), rotationMatrix );

		m_audioEmitter->Initialize( emitterName, L"", position );
		m_audioEmitter->SetPosition( front, top, position );

		return true;
	}

	return false;
}

void AudioGameObject::py__init__()
{
	Initialize();
}

void AudioGameObject::UpdateSyncronous( const EveUpdateContext& updateContext )
{
	UpdateWorldTransform( updateContext.GetTime() );

	if( m_audioEmitter && !m_mute )
	{
		Vector3 position = GetWorldPosition();
		Quaternion rotation = GetWorldRotation();
		Matrix rotationMatrix = RotationMatrix( rotation );
		Vector3 front = TransformNormal( Vector3( 0, 1, 0 ), rotationMatrix );
		Vector3 top = TransformNormal( Vector3( 0, 0, 1 ), rotationMatrix );
		m_audioEmitter->SetPosition( front, top, position );
	}
}

void AudioGameObject::UpdateAsyncronous( const EveUpdateContext& updateContext )
{
}

void AudioGameObject::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform )
{
}

void AudioGameObject::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors )
{
}

bool AudioGameObject::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	Vector3 pos = m_worldTransform.GetTranslation();
	sphere = Vector4( pos.x, pos.y, pos.z, 1.0f );
	return true;
}

void AudioGameObject::GetPerObjectStructs( EveSpaceObjectVSData& vsData, EveSpaceObjectPSData& psData ) const
{
}

void AudioGameObject::UpdateModelCenterWorldPosition( Vector3& position, Be::Time t )
{
	UpdateWorldTransform( t );
	Matrix currentTransform = TransformationMatrix( Vector3( 1.0f, 1.0f, 1.0f ), m_rotation, m_translation ) * m_worldTransform;
	position = currentTransform.GetTranslation();
}

void AudioGameObject::GetModelCenterWorldPosition( Vector3& position ) const
{
	Matrix currentTransform = TransformationMatrix( Vector3( 1.0f, 1.0f, 1.0f ), m_rotation, m_translation ) * m_worldTransform;
	position = currentTransform.GetTranslation();
}

bool AudioGameObject::GetLocalBoundingBox( Vector3& min, Vector3& max )
{
	min = Vector3( -1.0f, -1.0f, -1.0f );
	max = Vector3( 1.0f, 1.0f, 1.0f );
	return true;
}

void AudioGameObject::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = TransformationMatrix( Vector3( 1.0f, 1.0f, 1.0f ), m_rotation, m_translation ) * m_worldTransform;
}

Vector3 AudioGameObject::GetWorldPosition()
{
	return m_worldTransform.GetTranslation();
}

Quaternion AudioGameObject::GetWorldRotation()
{
	return Normalize( m_rotation * RotationQuaternion( m_worldTransform ) );
}

void AudioGameObject::SetEmitterName( const std::string& name )
{
	if( m_audioEmitter )
	{
		m_audioEmitter->SetName( name.c_str() );
	}
}

unsigned int AudioGameObject::PlayAudioEvent( const std::wstring& eventName )
{
	if( m_audioEmitter && !eventName.empty() )
	{
		return m_audioEmitter->SendEvent( eventName );
	}
	return 0;
}

void AudioGameObject::UpdateWorldTransform( Be::Time time )
{
	Quaternion rotation;
	Vector3 translation;

	if( m_ballPosition )
	{
		m_ballPosition->Update( &translation, time );
	}
	else
	{
		translation = m_translation;
	}

	if( m_ballRotation )
	{
		m_ballRotation->Update( &rotation, time );
	}
	else
	{
		rotation = m_rotation;
	}

	m_worldTransform = RotationMatrix( rotation ) * TranslationMatrix( translation );
}

void AudioGameObject::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Audio Speaker" );

	if( ITr2DebugRenderablePtr tmp = BlueCastPtr( m_audioEmitter ) )
	{
		tmp->GetDebugOptions( options );
	}
}

void AudioGameObject::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( renderer.HasOption( GetRawRoot(), "Audio Speaker" ) )
	{
		Matrix rotationMatrix = RotationMatrix( m_rotation );
		Vector3 orientation = rotationMatrix.GetZ(); // Z-axis is typically the forward direction
		renderer.DrawAudioSpeaker( this, m_worldTransform, 30, 5, Tr2DebugRenderer::Wireframe, 0xffff00ff );
	}

	if( ITr2DebugRenderablePtr tmp = BlueCastPtr( m_audioEmitter ) )
	{
		tmp->RenderDebugInfo( renderer );
	}
}

bool AudioGameObject::OnModified( Be::Var* val )
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