// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveVolumeObject.h"
#include "Eve/Volume/EveBoxVolume.h"

EveVolumeObject::EveVolumeObject( IRoot* lockobj ) :
	EveEntity( lockobj ),
	m_worldTransform( IdentityMatrix() ),
	m_enabled( true ),
	m_lastSentTransform( IdentityMatrix() ),
	m_hasSentTransform( false ),
	m_lastSentEnabled( true ),
	m_resendOnRegister( false )
{
}

EveVolumeObject::~EveVolumeObject()
{
	// scenes don't unregister their objects when they are destroyed
	RemoveFromConsumer();
}

ITr2BoxVolumePtr EveVolumeObject::GetConsumer() const
{
	return m_consumer;
}

void EveVolumeObject::SetConsumer( ITr2BoxVolume* consumer )
{
	if( m_consumer.p == consumer )
	{
		return;
	}

	RemoveFromConsumer();
	m_consumer = consumer;
}

void EveVolumeObject::RebuildBoundingSphere()
{
	m_boundingSphere = m_boxVolume ? m_boxVolume->GetBoundingSphere() : CcpMath::Sphere();
}

void EveVolumeObject::UpdateWorldTransform( Be::Time time )
{
	Quaternion rotation;
	Vector3 translation;

	if( m_ballPosition )
	{
		m_ballPosition->Update( &translation, time );
	}
	else
	{
		translation = Vector3( 0.0f, 0.0f, 0.0f );
	}

	if( m_ballRotation )
	{
		m_ballRotation->Update( &rotation, time );
	}
	else
	{
		rotation = Quaternion( 0.0f, 0.0f, 0.0f, 1.0f );
	}

	m_worldTransform = RotationMatrix( rotation ) * TranslationMatrix( translation );
}

void EveVolumeObject::SendTransform( const Matrix& unitBoxToWorld )
{
	m_consumer->SetTransform( unitBoxToWorld );
	m_lastSentTransform = unitBoxToWorld;
	m_hasSentTransform = true;
}

void EveVolumeObject::RemoveFromConsumer()
{
	if( m_consumer && m_hasSentTransform )
	{
		m_consumer->Remove();
	}
	m_hasSentTransform = false;
}

bool EveVolumeObject::TransformsNearlyEqual( const Matrix& a, const Matrix& b )
{
	const XMVECTOR axisEpsilon = XMVectorReplicate( 1e-4f );
	const XMVECTOR translationEpsilon = XMVectorReplicate( 1e-2f ); // 1 cm

	return XMVector3NearEqual( a.GetX(), b.GetX(), axisEpsilon ) && XMVector3NearEqual( a.GetY(), b.GetY(), axisEpsilon ) && XMVector3NearEqual( a.GetZ(), b.GetZ(), axisEpsilon ) && XMVector3NearEqual( a.GetTranslation(), b.GetTranslation(), translationEpsilon );
}

void EveVolumeObject::PushToConsumer()
{
	if( !m_consumer )
	{
		return;
	}

	if( !m_boxVolume )
	{
		RemoveFromConsumer();
		return;
	}

	// a consumer starts enabled, so the enabled state goes before the shape
	if( !m_hasSentTransform || m_enabled != m_lastSentEnabled )
	{
		m_consumer->SetEnabled( m_enabled );
		m_lastSentEnabled = m_enabled;
	}

	// relative to the ego ball, so this changes whenever the player moves
	const Matrix unitBoxToWorld = m_boxVolume->GetBoxTransform() * m_worldTransform;
	if( !m_hasSentTransform || !TransformsNearlyEqual( unitBoxToWorld, m_lastSentTransform ) )
	{
		SendTransform( unitBoxToWorld );
	}
}

// IEveSpaceObject2
void EveVolumeObject::UpdateSyncronous( const EveUpdateContext& updateContext )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	UpdateWorldTransform( updateContext.GetTime() );

	RebuildBoundingSphere();

	PushToConsumer();
}

void EveVolumeObject::UpdateAsyncronous( const EveUpdateContext& updateContext )
{
}

void EveVolumeObject::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform )
{
}

void EveVolumeObject::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors )
{
}

bool EveVolumeObject::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	Vector3 worldCenter = Transform( m_boundingSphere.center, m_worldTransform ).GetXYZ();
	sphere = Vector4( worldCenter.x, worldCenter.y, worldCenter.z, std::max( m_boundingSphere.radius, 1.0f ) );
	return true;
}

void EveVolumeObject::UpdateModelCenterWorldPosition( Vector3& position, Be::Time t )
{
	UpdateWorldTransform( t );
	GetModelCenterWorldPosition( position );
}

void EveVolumeObject::GetModelCenterWorldPosition( Vector3& position ) const
{
	position = Transform( m_boundingSphere.center, m_worldTransform ).GetXYZ();
}

bool EveVolumeObject::GetLocalBoundingBox( Vector3& min, Vector3& max )
{
	// Fall back to a unit box when no box is set up yet, so the object stays pickable in Graphite.
	float radius = std::max( m_boundingSphere.radius, 1.0f );
	Vector3 extent( radius, radius, radius );

	min = m_boundingSphere.center - extent;
	max = m_boundingSphere.center + extent;
	return true;
}

void EveVolumeObject::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

Vector3 EveVolumeObject::GetWorldPosition()
{
	return m_worldTransform.GetTranslation();
}

Quaternion EveVolumeObject::GetWorldRotation()
{
	return Normalize( RotationQuaternion( m_worldTransform ) );
}

bool EveVolumeObject::Initialize()
{
	UpdateWorldTransform( Be::Time( 0.0 ) );
	RebuildBoundingSphere();
	return true;
}

void EveVolumeObject::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Volume Objects" );
}

void EveVolumeObject::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( m_boxVolume && renderer.HasOption( GetRawRoot(), "Volume Objects" ) )
	{
		// cyan when active, grey when disabled
		const Color color = m_enabled ? 0xFF33DDFF : 0xFF666666;

		m_boxVolume->RenderDebugInfo( renderer, m_worldTransform, color );
	}
}

// EveEntity
void EveVolumeObject::RegisterComponents()
{
	// ReregisterEntities: put the shape straight back
	if( m_resendOnRegister )
	{
		PushToConsumer();
	}
	m_resendOnRegister = false;
}

void EveVolumeObject::UnRegisterComponents()
{
	m_resendOnRegister = m_hasSentTransform;
	RemoveFromConsumer();
}
