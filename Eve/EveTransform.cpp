#include "StdAfx.h"
#include "EveTransform.h"
#include "Tr2Model.h"
#include "Vector3d.h"
#include "Utilities/BoundingSphere.h"
#include "Utilities/BoundingBox.h"
#include "TriFrustum.h"
#include "Particle/Tr2ParticleSystem.h"
#include "Particle/ITr2GenericEmitter.h"
#include "EveUpdateContext.h"
#include "EveLODHelper.h"
#include "Curves/TriCurveSet.h"
#include "Tr2Mesh.h"
#include "Tr2MeshLod.h"

extern float g_eveSpaceSceneLowDetailThreshold;
extern float g_eveSpaceSceneMediumDetailThreshold;
extern float g_eveSpaceSceneLowUpdateRate;

EveTransform::EveTransform( IRoot* lockobj ) :
	Tr2Transform( lockobj ),
	PARENTLOCK( m_children ),
	PARENTLOCK( m_observers ),
	PARENTLOCK( m_particleEmitters ),
	PARENTLOCK( m_particleSystems ),
	m_debugShowBoundingBox( true ),
	m_debugRenderDebugInfoForChildren( true ),
	m_isVisible( true ),
	m_hideOnLowQuality( false ),
	m_visibilityThreshold( 2.0f ),
	m_lastRelativePosition(0,0,0),
	m_lastDeltaTime(0.f),
	m_lastCurveUpdateDelta( g_eveSpaceSceneLowUpdateRate ),
	m_useLodLevel( true ),
	m_lodLevel( TR2_LOD_LOW ),
	m_viewUpdatedThisFrame( false )
{
}

bool EveTransform::Initialize()
{
	if( m_meshLod )
	{
		m_meshLod->SelectLod( TR2_LOD_LOW );
		m_mesh = m_meshLod;
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
	D3DXMatrixTranspose( &data->m_world, &m_worldTransform );
	// attention: need the transposed, but shader also needs column_major, so it is transpose(transpose(m)) == m
	if( D3DXMatrixInverse( &data->m_worldInverseTranspose, NULL, &m_worldTransform ) == NULL)
	{
		// ok, so a complete row is 0.f -> find it and "fix" it
		Matrix wm = m_worldTransform;
		if( wm._11 == 0.f && wm._12 == 0.f && wm._13 == 0.f )
			wm._11 = 0.1f;
		else if( wm._21 == 0.f && wm._22 == 0.f && wm._23 == 0.f )
			wm._22 = 0.1f;
		else if( wm._31 == 0.f && wm._32 == 0.f && wm._33 == 0.f )
			wm._33 = 0.1f;
		D3DXMatrixInverse( &data->m_worldInverseTranspose, NULL, &wm );
	}

	return data;
}

void EveTransform::Update( EveUpdateContext& updateContext )
{
	UpdateSyncronous( updateContext );
	UpdateAsyncronous( updateContext );
}

void EveTransform::UpdateSyncronous( EveUpdateContext& updateContext )
{
	m_viewUpdatedThisFrame = false;
}

void EveTransform::UpdateAsyncronous( EveUpdateContext& updateContext )
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


	if( !m_isVisible )
	{
		// This means emitters start working one frame after object is visible.
		return;
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

	m_isVisible = false;
}

void EveTransform::UpdateViewDependentData( const Matrix& parentTransform, bool updateChildren )
{
	if( !m_viewUpdatedThisFrame )
	{
		Tr2Transform::UpdateViewDependentData( parentTransform );

		for( auto it = m_particleSystems.begin(); it != m_particleSystems.end(); ++it )
		{
			(*it)->UpdateViewDependentData( m_worldTransform );
		}

		TriObserverLocalVector::iterator observersEnd = m_observers.end();
		for( TriObserverLocalVector::iterator it = m_observers.begin(); it != observersEnd; ++it )
		{
			(*it)->Update( m_worldTransform );
		}
		m_viewUpdatedThisFrame = true;
	}

	if( updateChildren )
	{
		for( IEveTransformVector::const_iterator it = m_children.begin(); it != m_children.end(); ++it )
		{
			IEveTransform* p = *it;
			p->UpdateViewDependentData( m_worldTransform, true );
		}
	}
}

void EveTransform::RenderDebugInfo( Tr2RenderContext& renderContext )
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

	if( m_debugShowBoundingBox )
	{
		Vector3 minBounds( -0.5f, -0.5f, -0.5f );
		Vector3 maxBounds( 0.5f, 0.5f, 0.5f );
		uint32_t color = 0xff0000ff;

		if( m_mesh )
		{
			if( m_mesh->GetBoundingBox( minBounds, maxBounds ) )
			{
				color = 0xffffffff;
			}

			Vector4 boundingSphere;
			if( m_mesh->GetBoundingSphere( boundingSphere ) )
			{
				BoundingSphereTransform( m_worldTransform, boundingSphere );

				Tr2Renderer::DrawSphere( BoundingSphereGetCenter( boundingSphere ), boundingSphere.w, 0xff808000 );
			}

		}

		BoundingBoxTransform( minBounds, maxBounds, m_worldTransform );
		Tr2Renderer::DrawBox( minBounds, maxBounds, color );

		Tr2Renderer::Printf( TRI_DBG_FONT_SMALL, maxBounds, 0xffffffff, m_name.c_str() );
	}

	if( m_debugRenderDebugInfoForChildren )
	{
		for( IEveTransformVector::const_iterator it = m_children.begin(); it != m_children.end(); ++it )
		{
			(*it)->RenderDebugInfo( renderContext );
		}
	}
}

Tr2Lod EveTransform::GetLODLevel() const
{
	return m_lodLevel;
}

void EveTransform::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform )
{
	m_lodLevel = TR2_LOD_LOW;

	// is this one here enabled?
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	if( !m_display )
	{
		return;
	}
	
	UpdateViewDependentData( parentTransform );
	
	if( m_mesh )
	{
		m_isVisible = false;
		Vector4 boundingSphere;
		if( GetBoundingSphere( boundingSphere ) )
		{
			// check visibility with camera or, if threshold set to negative, no culling
			if( frustum.IsSphereVisible( &boundingSphere ) || ( m_visibilityThreshold < 0.f ) )
			{
				float estimatedSize = frustum.GetPixelSizeAccross( &boundingSphere );
				if( estimatedSize >= g_eveSpaceSceneMediumDetailThreshold )
				{
					m_lodLevel = TR2_LOD_HIGH;
				}
				else if( estimatedSize >= g_eveSpaceSceneLowDetailThreshold )
				{
					m_lodLevel = TR2_LOD_MEDIUM;
				}

				if( m_meshLod )
				{
					m_meshLod->SelectLod( static_cast<Tr2Lod>( m_lodLevel ) );
				}

				if( estimatedSize > m_visibilityThreshold )
				{
					renderables.push_back( this );
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
		p->GetRenderables( frustum, renderables, m_worldTransform );
		
		// Use the highest child LOD level.
		m_lodLevel = EveLODHelper::MergeLOD( m_lodLevel, p->GetLODLevel() );
	}
}

bool EveTransform::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	bool valid = false;
	Vector3 minBounds, maxBounds;
	if( m_mesh && m_mesh->GetBoundingBox( minBounds, maxBounds ) )
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

const Vector3* EveTransform::GetWorldPosition()
{
	return (Vector3*)&m_worldTransform._41;
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
void EveTransform::PlayCurveSet( const std::string& name )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( (*it)->GetName() == name )
		{
			(*it)->Play();
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
		if( (*it)->GetName() == name )
		{
			maxDuration = max( maxDuration, (*it)->GetMaxCurveDuration() );
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