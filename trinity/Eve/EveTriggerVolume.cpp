// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveTriggerVolume.h"

EveTriggerVolume::EveTriggerVolume( IRoot* lockobj ) :
	PARENTLOCK( m_volumes ),
	PARENTLOCK( m_exclusionVolumes ),
	PARENTLOCK( m_externalParameters ),
	m_worldTransform( IdentityMatrix() ),
	m_enterThreshold( 0.5f ),
	m_isInside( false ),
	m_currentIntensity( 0.0f )
{
}

EveTriggerVolume::~EveTriggerVolume()
{
}

void EveTriggerVolume::RebuildBoundingSphere()
{
	CCP_STATS_ZONE( __FUNCTION__ );

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

void EveTriggerVolume::SetCallback( const BlueScriptCallback& callback )
{
	m_callback = callback;
}

void EveTriggerVolume::InvokeCallback( bool entered )
{
	BlueScriptCallback callback = m_callback;
	if( !callback )
	{
		return;
	}

	callback.CallVoid( m_name.c_str(), entered ).ReportException();
}

void EveTriggerVolume::UpdateWorldTransform( Be::Time time )
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
void EveTriggerVolume::UpdateSyncronous( const EveUpdateContext& updateContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	UpdateWorldTransform( updateContext.GetTime() );

	RebuildBoundingSphere();

	UpdateTriggerState( updateContext );
}

float EveTriggerVolume::GetMaxIntensity( const PIEveVolumeVector& volumes, const Vector3& position )
{
	float intensity = 0.0f;
	for( const auto& volume : volumes )
	{
		intensity = std::max( intensity, volume->GetIntensity( position ) );
		if( intensity == 1.0f )
		{
			// early exit
			break;
		}
	}
	return intensity;
}

void EveTriggerVolume::UpdateTriggerState( const EveUpdateContext& updateContext )
{
	m_currentIntensity = 0.0f;

	bool inside = false;
	if( m_trackedPosition && !m_volumes.empty() )
	{
		Vector3 trackedPosition;
		m_trackedPosition->Update( &trackedPosition, updateContext.GetTime() );

		Matrix inverseWorldTransform = Inverse( m_worldTransform );
		Vector3 positionInObjectSpace = Transform( trackedPosition, inverseWorldTransform ).GetXYZ();

		// check first if the tracked position is within the bounding sphere
		if( m_boundingSphere.IsPointInside( positionInObjectSpace ) )
		{
			m_currentIntensity = GetMaxIntensity( m_volumes, positionInObjectSpace );

			if( m_currentIntensity != 0.0f )
			{
				// check if the tracked position is within an exclusion volume
				float negativeIntensity = GetMaxIntensity( m_exclusionVolumes, positionInObjectSpace );
				m_currentIntensity = std::max( 0.0f, m_currentIntensity - negativeIntensity );
			}
		}

		inside = m_currentIntensity >= m_enterThreshold;
	}

	if( inside != m_isInside )
	{
		m_isInside = inside;
		InvokeCallback( inside );
	}
}

void EveTriggerVolume::UpdateAsyncronous( const EveUpdateContext& updateContext )
{
}

void EveTriggerVolume::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform )
{
}

void EveTriggerVolume::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors )
{
}

bool EveTriggerVolume::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	Vector3 worldCenter = Transform( m_boundingSphere.center, m_worldTransform ).GetXYZ();
	sphere = Vector4( worldCenter.x, worldCenter.y, worldCenter.z, std::max( m_boundingSphere.radius, 1.0f ) );
	return true;
}

void EveTriggerVolume::UpdateModelCenterWorldPosition( Vector3& position, Be::Time t )
{
	UpdateWorldTransform( t );
	GetModelCenterWorldPosition( position );
}

void EveTriggerVolume::GetModelCenterWorldPosition( Vector3& position ) const
{
	position = Transform( m_boundingSphere.center, m_worldTransform ).GetXYZ();
}

bool EveTriggerVolume::GetLocalBoundingBox( Vector3& min, Vector3& max )
{
	// Fall back to a unit box when no volumes are set up yet, so the object stays pickable in Graphite.
	float radius = std::max( m_boundingSphere.radius, 1.0f );
	Vector3 extent( radius, radius, radius );

	min = m_boundingSphere.center - extent;
	max = m_boundingSphere.center + extent;
	return true;
}

void EveTriggerVolume::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

Vector3 EveTriggerVolume::GetWorldPosition()
{
	return m_worldTransform.GetTranslation();
}

Quaternion EveTriggerVolume::GetWorldRotation()
{
	return Normalize( RotationQuaternion( m_worldTransform ) );
}

bool EveTriggerVolume::Initialize()
{
	UpdateWorldTransform( Be::Time( 0.0 ) );
	RebuildBoundingSphere();
	return true;
}

void EveTriggerVolume::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Trigger Volumes" );
	options.insert( "Trigger Exclusion Volumes" );
	options.insert( "Trigger Bounding Sphere" );
}

void EveTriggerVolume::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( renderer.HasOption( GetRawRoot(), "Trigger Volumes" ) )
	{
		// green when the tracked position is inside, white otherwise
		Color color = 0xFFFFFFFF;
		if( m_isInside )
		{
			color = 0xFF33FF33;
		}

		for( const auto& volume : m_volumes )
		{
			volume->RenderDebugInfo( renderer, m_worldTransform, color );
		}
	}

	if( renderer.HasOption( GetRawRoot(), "Trigger Exclusion Volumes" ) )
	{
		for( const auto& volume : m_exclusionVolumes )
		{
			volume->RenderDebugInfo( renderer, m_worldTransform, 0xFFFF3333 );
		}
	}

	if( renderer.HasOption( GetRawRoot(), "Trigger Bounding Sphere" ) )
	{
		renderer.DrawSphere( this, TranslationMatrix( m_boundingSphere.center ) * m_worldTransform, m_boundingSphere.radius, 10, Tr2DebugRenderer::Wireframe, 0xff333333 );
	}
}
