////////////////////////////////////////////////////////////
//
//    Created:   June 2015
//    Copyright: CCP 2015
//
#include "StdAfx.h"
#include "EveChildParticleSystem.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Resources/TriGeometryRes.h"
#include "Eve/EveTransform.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Tr2InstancedMesh.h"
#include "TriFrustum.h"
#include "Eve/EveUpdateContext.h"

#include "Particle/Tr2ParticleSystem.h"
#include "Particle/ITr2GenericEmitter.h"

#include "Utilities/BoundingSphere.h"

EveChildParticleSystem::EveChildParticleSystem( IRoot* lockobj ):
	EveChildTransform(),
	PARENTLOCK( m_particleEmitters ),
	PARENTLOCK( m_particleSystems ),
	m_boundingSphere( 0, 0, 0, -1 ),
	m_display( true )
{
}

EveChildParticleSystem::~EveChildParticleSystem()
{
}

bool EveChildParticleSystem::Initialize()
{
	if( m_staticTransform )
	{
		RebuildLocalTransform();
	}
	return true;
}

void EveChildParticleSystem::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform )
{
	if( !m_display || !frustum.IsSphereVisible( &m_boundingSphere ) )
	{
		return;
	}

	for( auto it = m_particleSystems.begin(); it != m_particleSystems.end(); ++it )
	{
		(*it)->UpdateViewDependentData( m_worldTransform );
	}

	renderables.push_back( this );
}

bool EveChildParticleSystem::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	if( m_boundingSphere.w == -1 ) 
	{
		return false;
	}
	sphere = m_boundingSphere;
	return true;
}

bool EveChildParticleSystem::HasTransparentBatches()
{
	if( m_display && m_mesh )
	{
		return !(m_mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT )->empty());
	}

	return false;
}

void EveChildParticleSystem::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData )
{
	if( m_display && m_mesh )
	{
		m_mesh->GetBatches( batches, m_mesh->GetAreas( batchType ), perObjectData );
	}
}

void EveChildParticleSystem::GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData )
{
	if( m_display && m_mesh )
	{
		m_mesh->GetBatches( batches, m_mesh->GetAreas( TRIBATCHTYPE_OPAQUE ), perObjectData );
	}
}

float EveChildParticleSystem::GetSortValue()
{
	Vector3 d = Tr2Renderer::GetViewPosition() - m_worldTransform.GetTranslation();
	float distance = D3DXVec3Length( &d );
	return distance;
}

Tr2PerObjectData* EveChildParticleSystem::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	EveBasicPerObjectData* data = accumulator->Allocate<EveBasicPerObjectData>();

	if( !data )
	{
		return NULL;
	}

	// column_major for shaders
	D3DXMatrixTranspose( &data->m_world, &m_worldTransform );
	D3DXMatrixInverse( &data->m_worldInverseTranspose, NULL, &m_worldTransform );

	return data;
}

void EveChildParticleSystem::UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* parent )
{
}

void EveChildParticleSystem::UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* parent )
{
	Matrix localToWorldTransform;
	parent->GetLocalToWorldTransform( localToWorldTransform );
	UpdateTransform( localToWorldTransform );
	
	Vector3 minBounds, maxBounds;
	if( m_mesh && m_mesh->GetBoundingBox( minBounds, maxBounds ) )
	{
		BoundingSphereFromBox( m_boundingSphere, minBounds, maxBounds, &m_worldTransform );
	}

	for( auto it = m_particleSystems.begin(); it != m_particleSystems.end(); ++it )
	{
		(*it)->UpdateTransform( m_worldTransform );
	}
	if( !m_particleEmitters.empty() )
	{
		ITr2GenericEmitter::UpdateArguments args( 
			updateContext.GetTime(), 
			updateContext.GetGpuParticleSystem(), 
			m_worldTransform, 
			updateContext.GetOriginShift() );
		for( auto it = m_particleEmitters.begin(); it != m_particleEmitters.end(); ++it )
		{
			(*it)->Update( args );
		}
	}
}

void EveChildParticleSystem::UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObjectChild* parent )
{
}

void EveChildParticleSystem::UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObjectChild* parent )
{
	Matrix localToWorldTransform;
	parent->GetLocalToWorldTransform( localToWorldTransform );
	UpdateTransform( localToWorldTransform );
	
	Vector3 minBounds, maxBounds;
	if( m_mesh && m_mesh->GetBoundingBox( minBounds, maxBounds ) )
	{
		BoundingSphereFromBox( m_boundingSphere, minBounds, maxBounds, &m_worldTransform );
	}

	for( auto it = m_particleSystems.begin(); it != m_particleSystems.end(); ++it )
	{
		(*it)->UpdateTransform( m_worldTransform );
	}
	if( !m_particleEmitters.empty() )
	{
		ITr2GenericEmitter::UpdateArguments args( 
			updateContext.GetTime(), 
			updateContext.GetGpuParticleSystem(), 
			m_worldTransform, 
			updateContext.GetOriginShift() );
		for( auto it = m_particleEmitters.begin(); it != m_particleEmitters.end(); ++it )
		{
			(*it)->Update( args );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Returns the Local to World transformation matrix
// --------------------------------------------------------------------------------
void EveChildParticleSystem::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}