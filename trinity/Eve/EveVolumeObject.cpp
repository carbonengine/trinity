// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveVolumeObject.h"

#include <cmath>

namespace
{
	bool NearlyEqual( float a, float b, float epsilon )
	{
		return std::fabs( a - b ) <= epsilon;
	}

	/// True when two transforms are close enough that a consumer would not notice the difference.
	bool TransformsNearlyEqual( const Matrix& a, const Matrix& b )
	{
		constexpr float axisEpsilon = 1e-4f;        // rotation and scale, in unit-box space
		constexpr float translationEpsilon = 1e-2f; // 1 cm

		return NearlyEqual( a._11, b._11, axisEpsilon ) && NearlyEqual( a._12, b._12, axisEpsilon ) && NearlyEqual( a._13, b._13, axisEpsilon )
			&& NearlyEqual( a._21, b._21, axisEpsilon ) && NearlyEqual( a._22, b._22, axisEpsilon ) && NearlyEqual( a._23, b._23, axisEpsilon )
			&& NearlyEqual( a._31, b._31, axisEpsilon ) && NearlyEqual( a._32, b._32, axisEpsilon ) && NearlyEqual( a._33, b._33, axisEpsilon )
			&& NearlyEqual( a._41, b._41, translationEpsilon ) && NearlyEqual( a._42, b._42, translationEpsilon ) && NearlyEqual( a._43, b._43, translationEpsilon );
	}
}

EveVolumeObject::EveVolumeObject( IRoot* lockobj ) :
	PARENTLOCK( m_volumes ),
	m_worldTransform( IdentityMatrix() ),
	m_enabled( true ),
	m_boxChangeCallbackID( 0 ),
	m_warnedAboutVolumes( false ),
	m_lastSentTransform( IdentityMatrix() ),
	m_hasSentTransform( false ),
	m_lastSentEnabled( true ),
	m_transformDirty( false )
{
}

EveVolumeObject::~EveVolumeObject()
{
	if( m_boxVolume && m_boxChangeCallbackID != 0 )
	{
		m_boxVolume->UnregisterForChanges( m_boxChangeCallbackID );
	}

	if( m_consumer )
	{
		m_consumer->Remove();
	}
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
	// The box may be replaced or removed while editing; drop it when it is no longer in the list.
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
		}
	}

	if( m_boxVolume || m_volumes.empty() )
	{
		return;
	}

	// The consumer takes exactly one shape, so the first box volume defines it.
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
		// Fires when the box is edited (e.g. in Graphite); the next update re-sends the transform.
		m_boxChangeCallbackID = m_boxVolume->RegisterForChanges( [this]()
		{
			m_transformDirty = true;
		} );
	}
}

void EveVolumeObject::SendTransform( const Matrix& unitBoxToWorld )
{
	m_consumer->SetTransform( unitBoxToWorld );
	m_lastSentTransform = unitBoxToWorld;
	m_hasSentTransform = true;
	m_transformDirty = false;
}

void EveVolumeObject::PushToConsumer()
{
	if( !m_consumer || !m_boxVolume )
	{
		return;
	}

	// Enabled state goes first: a consumer starts enabled, so a disabled object must say so before it
	// hands over a shape, otherwise the consumer would activate for one frame and deactivate again.
	if( m_enabled != m_lastSentEnabled )
	{
		m_consumer->SetEnabled( m_enabled );
		m_lastSentEnabled = m_enabled;
	}

	// The box lives in this object's space; compose with the world transform, as EveBoxVolume::RenderDebugInfo does.
	// The world transform is relative to the ego ball, so this changes every frame the player moves.
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
