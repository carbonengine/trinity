// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveVolumeObject.h"
#include "Eve/Volume/EveBoxVolume.h"

EveVolumeObject::EveVolumeObject( IRoot* lockobj ) :
	EveEntity( lockobj ),
	PARENTLOCK( m_volumes ),
	m_worldTransform( IdentityMatrix() ),
	m_enabled( true ),
	m_boxChangeCallbackID( 0 ),
	m_warnedAboutVolumes( false ),
	m_lastSentTransform( IdentityMatrix() ),
	m_hasSentTransform( false ),
	m_lastSentEnabled( true ),
	m_transformDirty( false ),
	m_resendOnRegister( false )
{
}

EveVolumeObject::~EveVolumeObject()
{
	if( m_boxVolume && m_boxChangeCallbackID != 0 )
	{
		m_boxVolume->UnregisterForChanges( m_boxChangeCallbackID );
	}

	// scenes don't unregister their objects when they are destroyed
	RemoveFromConsumer();
}

void EveVolumeObject::RebuildBoundingSphere()
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	m_boundingSphere = CcpMath::Sphere();

	for( const auto& volume : m_volumes )
	{
		auto volumeSphere = volume->GetBoundingSphere();

		if( !volumeSphere.IsInitialized() )
		{
			continue;
		}

		if( !m_boundingSphere.IsInitialized() || volumeSphere.IsSphereInside( m_boundingSphere ) )
		{
			m_boundingSphere = volumeSphere;
			continue;
		}

		if( m_boundingSphere.IsSphereInside( volumeSphere ) )
		{
			continue;
		}

		Vector3 delta = volumeSphere.center - m_boundingSphere.center;
		float deltaLen = Length( delta );

		m_boundingSphere.center += 0.5f * ( 1.f + ( volumeSphere.radius - m_boundingSphere.radius ) / deltaLen ) * delta;
		m_boundingSphere.radius = 0.5f * ( m_boundingSphere.radius + volumeSphere.radius + deltaLen );
	}
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

void EveVolumeObject::ResolveBoxVolume()
{
	if( m_boxVolume )
	{
		bool stillListed = false;
		for( const auto& volume : m_volumes )
		{
			if( volume == static_cast<IEveVolume*>( m_boxVolume.p ) )
			{
				stillListed = true;
				break;
			}
		}

		if( !stillListed )
		{
			if( m_boxChangeCallbackID != 0 )
			{
				m_boxVolume->UnregisterForChanges( m_boxChangeCallbackID );
				m_boxChangeCallbackID = 0;
			}
			m_boxVolume = nullptr;
			m_warnedAboutVolumes = false;
			m_transformDirty = true;
			RemoveFromConsumer();
		}
	}

	if( m_boxVolume || m_volumes.empty() )
	{
		return;
	}

	for( const auto& volume : m_volumes )
	{
		if( EveBoxVolumePtr box = BlueCastPtr( volume ) )
		{
			if( !m_boxVolume )
			{
				m_boxVolume = box;
			}
			else if( !m_warnedAboutVolumes )
			{
				CCP_LOGWARN( "EveVolumeObject '%s': only the first box volume is used, extra volumes are ignored.", m_name.c_str() );
				m_warnedAboutVolumes = true;
			}
		}
		else if( !m_warnedAboutVolumes )
		{
			CCP_LOGWARN( "EveVolumeObject '%s': only box volumes are supported, other volume types are ignored.", m_name.c_str() );
			m_warnedAboutVolumes = true;
		}
	}

	if( m_boxVolume )
	{
		m_boxChangeCallbackID = m_boxVolume->RegisterForChanges( [this]()
		{
			m_transformDirty = true;
		} );
	}
}

void EveVolumeObject::SendTransform( const Matrix& unitBoxToWorld )
{
	m_pushedConsumer->SetTransform( unitBoxToWorld );
	m_lastSentTransform = unitBoxToWorld;
	m_hasSentTransform = true;
	m_transformDirty = false;
}

void EveVolumeObject::RemoveFromConsumer()
{
	if( m_pushedConsumer && m_hasSentTransform )
	{
		m_pushedConsumer->Remove();
	}
	m_hasSentTransform = false;
}

bool EveVolumeObject::TransformsNearlyEqual( const Matrix& a, const Matrix& b )
{
	const XMVECTOR axisEpsilon = XMVectorReplicate( 1e-4f );
	const XMVECTOR translationEpsilon = XMVectorReplicate( 1e-2f ); // 1 cm

	return XMVector3NearEqual( a.GetX(), b.GetX(), axisEpsilon )
		&& XMVector3NearEqual( a.GetY(), b.GetY(), axisEpsilon )
		&& XMVector3NearEqual( a.GetZ(), b.GetZ(), axisEpsilon )
		&& XMVector3NearEqual( a.GetTranslation(), b.GetTranslation(), translationEpsilon );
}

void EveVolumeObject::PushToConsumer()
{
	if( m_pushedConsumer.p != m_consumer.p )
	{
		RemoveFromConsumer();
		m_pushedConsumer = m_consumer;
		// make sure a new consumer gets the enabled state
		m_lastSentEnabled = !m_enabled;
	}

	if( !m_pushedConsumer || !m_boxVolume )
	{
		return;
	}

	// a consumer starts enabled, so the enabled state goes before the shape
	if( m_enabled != m_lastSentEnabled )
	{
		m_pushedConsumer->SetEnabled( m_enabled );
		m_lastSentEnabled = m_enabled;
	}

	// relative to the ego ball, so this changes whenever the player moves
	const Matrix unitBoxToWorld = m_boxVolume->GetBoxTransform() * m_worldTransform;
	if( !m_hasSentTransform || m_transformDirty || !TransformsNearlyEqual( unitBoxToWorld, m_lastSentTransform ) )
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

	ResolveBoxVolume();

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
	// Fall back to a unit box when no volumes are set up yet, so the object stays pickable in Graphite.
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
	ResolveBoxVolume();
	return true;
}

void EveVolumeObject::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Volume Objects" );
}

void EveVolumeObject::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( renderer.HasOption( GetRawRoot(), "Volume Objects" ) )
	{
		// cyan when active, grey when disabled
		const Color color = m_enabled ? 0xFF33DDFF : 0xFF666666;

		for( const auto& volume : m_volumes )
		{
			volume->RenderDebugInfo( renderer, m_worldTransform, color );
		}
	}
}

// EveEntity
void EveVolumeObject::RegisterComponents()
{
	// ReregisterEntities: put the shape straight back
	if( m_resendOnRegister && m_pushedConsumer && m_boxVolume )
	{
		SendTransform( m_lastSentTransform );
	}
	m_resendOnRegister = false;
}

void EveVolumeObject::UnRegisterComponents()
{
	m_resendOnRegister = m_hasSentTransform;
	RemoveFromConsumer();
}
