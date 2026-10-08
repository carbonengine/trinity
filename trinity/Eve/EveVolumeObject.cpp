// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveVolumeObject.h"

EveVolumeObject::EveVolumeObject( IRoot* lockobj ) :
	PARENTLOCK( m_volumes ),
	m_worldTransform( IdentityMatrix() ),
	m_enabled( true )
{
}

EveVolumeObject::~EveVolumeObject()
{
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

// IEveSpaceObject2
void EveVolumeObject::UpdateSyncronous( const EveUpdateContext& updateContext )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	UpdateWorldTransform( updateContext.GetTime() );

	RebuildBoundingSphere();
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
