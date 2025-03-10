#include "StdAfx.h"
#include "EveTransform.h"
#include "Tr2Model.h"
#include "Utilities/Vector3d.h"
#include "Utilities/BoundingSphere.h"
#include "Utilities/BoundingBox.h"
#include "TriFrustum.h"
#include "Particle/Tr2ParticleSystem.h"
#include "Particle/ITr2GenericEmitter.h"
#include "EveUpdateContext.h"
#include "EveLODHelper.h"
#include "Curves/TriCurveSet.h"
#include "Tr2Mesh.h"
#include "Tr2MeshBase.h"

extern float g_eveSpaceSceneLowUpdateRate;

EveTransform::EveTransform( IRoot* lockobj ) :
	Tr2Transform( lockobj ),
	PARENTLOCK( m_children ),
	PARENTLOCK( m_observers ),
	PARENTLOCK( m_particleEmitters ),
	PARENTLOCK( m_particleSystems ),
	m_isVisible( true ),
	m_hideOnLowQuality( false ),
	m_visibilityThreshold( 2.0f ),
	m_lastRelativePosition(0,0,0),
	m_lastDeltaTime(0.f),
	m_lastCurveUpdateDelta( g_eveSpaceSceneLowUpdateRate ),
	m_useLodLevel( true ),
	m_lodLevel( TR2_LOD_LOW ),
	m_overrideBoundsMin( 0, 0, 0 ),
	m_overrideBoundsMax( 0, 0, 0 )
{
}

bool EveTransform::Initialize()
{
	if( !m_mesh )
	{
		m_mesh = m_meshLod;
		m_meshLod = nullptr;
	}
	return true;
}

Tr2PerObjectData* EveTransform::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	EveBasicPerObjectData* data = accumulator->Allocate<EveBasicPerObjectData>();

	if( !data )
	{
		return NULL;
	}

	// column_major for shaders
	data->m_world = Transpose( m_worldTransform );
	// column_major for shaders
	data->m_worldLast = Transpose( m_lastWorldTransform );
	// attention: need the transposed, but shader also needs column_major, so it is transpose(transpose(m)) == m
	if( !Inverse( data->m_worldInverse, data->m_world ) )
	{
		// ok, so a complete row is 0.f -> find it and "fix" it
		Matrix wm = data->m_world;
		if( wm._11 == 0.f && wm._12 == 0.f && wm._13 == 0.f )
			wm._11 = 0.1f;
		else if( wm._21 == 0.f && wm._22 == 0.f && wm._23 == 0.f )
			wm._22 = 0.1f;
		else if( wm._31 == 0.f && wm._32 == 0.f && wm._33 == 0.f )
			wm._33 = 0.1f;
		data->m_worldInverse = Inverse( wm );
	}

	return data;
}

void EveTransform::Update( const EveUpdateContext& updateContext )
{
	UpdateSyncronous( updateContext );
	UpdateAsyncronous( updateContext );
}

void EveTransform::UpdateSyncronous( const EveUpdateContext& updateContext )
{
}

void EveTransform::UpdateAsyncronous( const EveUpdateContext& updateContext )
{
	// is this one here enabled?
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	Be::Time time = updateContext.GetTime();
	m_lastDeltaTime = updateContext.GetDeltaT();

	if( !m_curveSets.empty() )
	{
		m_lastCurveUpdateDelta += m_lastDeltaTime;

		if( !m_useLodLevel || EveLODHelper::ShouldUpdate( m_lodLevel, m_lastCurveUpdateDelta ) )
		{
			m_lastCurveUpdateDelta = 0;
			for( TriCurveSetVector::const_iterator it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
			{
				(*it)->Update( TimeAsDouble( time ) );
			}
		}
	}

	for( IEveTransformVector::const_iterator it = m_children.begin(); it != m_children.end(); ++it )
	{
		IEveTransform* p = *it;
		p->Update( updateContext );
	}

	for( auto it = m_particleSystems.begin(); it != m_particleSystems.end(); ++it )
	{
		(*it)->UpdateTransform( m_worldTransform );
	}
	if( !m_particleEmitters.empty() )
	{
		ITr2GenericEmitter::UpdateArguments args( time, updateContext.GetGpuParticleSystem(), m_worldTransform, updateContext.GetOriginShift() );
		for( auto it = m_particleEmitters.begin(); it != m_particleEmitters.end(); ++it )
		{
			(*it)->Update( args );
		}
	}
	if( !m_particleSystems.empty() )
	{
		ITr2GenericEmitter::UpdateArguments args( time, updateContext.GetGpuParticleSystem(), IdentityMatrix(), updateContext.GetOriginShift() );
		for( auto it = m_particleSystems.begin(); it != m_particleSystems.end(); ++it )
		{
			( *it )->Update( args );
		}
	}
}

void EveTransform::UpdateViewDependentData( const TriFrustum& frustum, const Matrix& parentTransform )
{
	Tr2Transform::UpdateViewDependentData( frustum, parentTransform );

	for( auto it = m_particleSystems.begin(); it != m_particleSystems.end(); ++it )
	{
		(*it)->UpdateViewDependentData( &frustum, m_worldTransform );
	}

	TriObserverLocalVector::iterator observersEnd = m_observers.end();
	for( TriObserverLocalVector::iterator it = m_observers.begin(); it != observersEnd; ++it )
	{
		(*it)->Update( m_worldTransform );
	}
}

void EveTransform::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Bounding Box" );
	options.insert( "Bounding Sphere" );
	options.insert( "Names" );

	for ( auto it = m_observers.begin(); it != m_observers.end(); ++it )
	{
		( *it )->GetDebugOptions( options );
	}

	if( m_mesh )
	{
		m_mesh->GetDebugOptions( options );
	}
}

void EveTransform::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	// is this one here enabled?
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	if( !m_isVisible )
	{
		return;
	}

	for ( auto it = m_observers.begin(); it != m_observers.end(); ++it )
	{
		( *it )->RenderDebugInfo( renderer );
	}


	if( renderer.HasOption( this, "Bounding Box" ) )
	{
		Vector3 minBounds( -0.5f, -0.5f, -0.5f );
		Vector3 maxBounds( 0.5f, 0.5f, 0.5f );
		uint32_t color = 0xff0000ff;
		if( m_overrideBoundsMax.x != m_overrideBoundsMin.x || m_overrideBoundsMax.y != m_overrideBoundsMin.y || m_overrideBoundsMax.z != m_overrideBoundsMin.z )
		{
			minBounds = m_overrideBoundsMin;
			maxBounds = m_overrideBoundsMax;
			color = 0xffffffff;
		}
		else if( m_mesh && m_mesh->GetBoundingBox( minBounds, maxBounds ) )
		{
			color = 0xffffffff;
		}
		renderer.DrawBox( this, m_worldTransform, minBounds, maxBounds, Tr2DebugRenderer::Wireframe, color );
	}
	if( renderer.HasOption( this, "Bounding Sphere" ) )
	{
		Vector4 sphere;
		if( GetBoundingSphere( sphere ) )
		{
			renderer.DrawSphere( this, Vector3( sphere.x, sphere.x, sphere.x ), sphere.w, 10, Tr2DebugRenderer::Wireframe, 0xff808000 );
		}
	}

	if( renderer.HasOption( this, "Names" ) )
	{
		Vector3 minBounds( -0.5f, -0.5f, -0.5f );
		Vector3 maxBounds( 0.5f, 0.5f, 0.5f );
		if( m_mesh )
		{
			m_mesh->GetBoundingBox( minBounds, maxBounds );
		}
		renderer.DrawText( TRI_DBG_FONT_SMALL, maxBounds, 0xffffffff, m_name.c_str() );
	}

	if( m_mesh )
	{
		m_mesh->RenderDebugInfo( m_worldTransform, renderer );
	}


	for( IEveTransformVector::const_iterator it = m_children.begin(); it != m_children.end(); ++it )
	{
		if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( *it ) )
		{
			renderable->RenderDebugInfo( renderer );
		}
	}
}

Tr2Lod EveTransform::GetLODLevel() const
{
	return m_lodLevel;
}

void EveTransform::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors )
{
	// is this one here enabled?
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	if( !m_display )
	{
		return;
	}

	for( auto it = m_particleSystems.begin(); it != m_particleSystems.end(); it++ )
	{
		(*it)->SortParticles();
	}

	if( m_isVisible && m_mesh )
	{
		renderables.push_back( this );
	}

	for( IEveTransformVector::const_iterator it = m_children.begin(); it != m_children.end(); ++it )
	{
		IEveTransform* p = *it;
		p->GetRenderables( renderables );
	}
}

void EveTransform::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform )
{
	m_lodLevel = TR2_LOD_LOW;
	m_isVisible = false;

	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	if( !m_display )
	{
		return;
	}

	UpdateViewDependentData( updateContext.GetFrustum(), parentTransform );
	
	if( m_mesh )
	{
		Vector4 boundingSphere;
		if( GetBoundingSphere( boundingSphere ) )
		{
			// check visibility with camera or, if threshold set to negative, no culling
			if( updateContext.GetFrustum().IsSphereVisible( &boundingSphere ) || ( m_visibilityThreshold < 0.f ) )
			{
				float estimatedSize = updateContext.GetFrustum().GetPixelSizeAccross( &boundingSphere );

				m_mesh->UseWithScreenSize( estimatedSize, boundingSphere.w );

				if( estimatedSize >= updateContext.GetMediumDetailThreshold() )
				{
					m_lodLevel = TR2_LOD_HIGH;
				}
				else if( estimatedSize >= updateContext.GetLowDetailThreshold() )
				{
					m_lodLevel = TR2_LOD_MEDIUM;
				}

				if( estimatedSize > m_visibilityThreshold )
				{
					m_isVisible = true;
				}
			}
		}
	}
	else
	{
		m_isVisible = true;
	}

	if( !m_particleSystems.empty() )
	{
		// Because bounding info is currently unreliable for particle systems
		// we use LOD high.
		// This is hopefully a temporary measure. <6. september, 2012, Logi>
		m_lodLevel = TR2_LOD_HIGH;
	}

	for( IEveTransformVector::const_iterator it = m_children.begin(); it != m_children.end(); ++it )
	{
		IEveTransform* p = *it;
		p->UpdateVisibility( updateContext, m_worldTransform );
		
		// Use the highest child LOD level.
		m_lodLevel = EveLODHelper::MergeLOD( m_lodLevel, p->GetLODLevel() );
	}
}

void EveTransform::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	GetRenderables( renderables, nullptr );
}

bool EveTransform::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	bool valid = false;
	Vector3 minBounds, maxBounds;
	if( m_overrideBoundsMax.x != m_overrideBoundsMin.x || m_overrideBoundsMax.y != m_overrideBoundsMin.y || m_overrideBoundsMax.z != m_overrideBoundsMin.z )
	{
		minBounds = m_overrideBoundsMin;
		maxBounds = m_overrideBoundsMax;
		BoundingSphereFromBox( sphere, minBounds, maxBounds, &m_worldTransform );
		valid = true;
	}
	else if( m_mesh && m_mesh->GetBoundingBox( minBounds, maxBounds ) )
	{
		BoundingSphereFromBox( sphere, minBounds, maxBounds, &m_worldTransform );
		valid = true;
	}
	
	if( query == EVE_BOUNDS_WITH_CHILDREN )
	{
		Vector4 boundingSphere(0,0,0,100.f);
		for( auto it = m_children.cbegin(); it != m_children.cend(); it++ )
		{
			if( (*it)->GetBoundingSphere( boundingSphere, query ) )
			{
				BoundingSphereSetOrUpdate( boundingSphere, sphere, valid );
				valid = true;
			}
		}
	}

	return valid;
}

void EveTransform::UpdateModelCenterWorldPosition( Vector3 &position, Be::Time t )
{
	// As with EveSpaceObject2, we need to make sure that we fully update the position to give to the camera
	// This has to happen before the Scene2 update, where other objects may be camera aligned
	EveUpdateContext ctx( t );
	Update( ctx );
	position = m_worldTransform.GetTranslation();
}

Vector3 EveTransform::GetWorldPosition()
{
	return m_worldTransform.GetTranslation();
}

Quaternion EveTransform::GetWorldRotation()
{
	return m_rotation;
}

void EveTransform::GetModelCenterWorldPosition( Vector3 &position ) const
{
	position = m_worldTransform.GetTranslation();
}

// --------------------------------------------------------------------------------
// Description:
//   Plays all of the transform's curve sets.
// --------------------------------------------------------------------------------
void EveTransform::PlayCurveSets()
{
	for( TriCurveSetVector::const_iterator it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		(*it)->Play();
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Play curve sets with the appropriate name if they exist.
// --------------------------------------------------------------------------------
void EveTransform::PlayCurveSet( const std::string& name, const std::string& rangeName )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( (*it)->GetName() == name )
		{
			if( rangeName.empty() )
			{
				( *it )->ResetTimeRange();
				( *it )->Play();
			}
			else
			{
				( *it )->PlayTimeRange( rangeName.c_str() );
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Stop curve sets with the appropriate name if they exist.
// --------------------------------------------------------------------------------
void EveTransform::StopCurveSet( const std::string& name )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( (*it)->GetName() == name )
		{
			(*it)->Stop();
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Get maximum curve set duration for the provided name.
// --------------------------------------------------------------------------------
float EveTransform::GetCurveSetDuration( const std::string& name ) const
{
	float maxDuration = 0.f;
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			maxDuration = max( maxDuration, ( *it )->GetMaxCurveDuration() );
		}
	}

	return maxDuration;
}

// --------------------------------------------------------------------------------
float EveTransform::GetRangeDuration( const std::string& name, const std::string& rangeName ) const
{
	float maxDuration = 0.f;
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			maxDuration = std::max( maxDuration, ( *it )->GetRangeDuration( rangeName.c_str() ) );
		}
	}

	return maxDuration;
}

void EveTransform::GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData )
{
	if( ( pickTypes & PICK_TYPE_PICKING ) != 0 )
	{
		GetBatches( batches, TRIBATCHTYPE_PICKING, perObjectData );
	}
	if( ( pickTypes & PICK_TYPE_OPAQUE ) != 0 )
	{
		GetBatches( batches, TRIBATCHTYPE_OPAQUE, perObjectData );
	}
	if( ( pickTypes & PICK_TYPE_TRANSPARENT ) != 0 )
	{
		GetBatches( batches, TRIBATCHTYPE_TRANSPARENT, perObjectData );
		GetBatches( batches, TRIBATCHTYPE_ADDITIVE, perObjectData );
	}
}