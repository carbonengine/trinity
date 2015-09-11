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
	PARENTLOCK( m_particleEmitters ),
	PARENTLOCK( m_particleSystems ),
	m_boundingSphere( 0, 0, 0, -1 ),
	m_translation( 0, 0, 0 ),
	m_rotation( 0, 0, 0, 1 ),
	m_scaling( 1, 1, 1 ),
	m_display( true )
{
}

EveChildParticleSystem::~EveChildParticleSystem()
{
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

void EveChildParticleSystem::UpdateSyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent )
{
}

void EveChildParticleSystem::UpdateAsyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent )
{
	Matrix localTransform, localToWorldTransform;
	D3DXMatrixTransformation( &localTransform, 0, 0, &m_scaling, 0, &m_rotation, &m_translation );
	parent->GetLocalToWorldTransform( localToWorldTransform );
	D3DXMatrixMultiply(&m_worldTransform, &localTransform, &localToWorldTransform );
	
	Vector3 minBounds, maxBounds;
	if( m_mesh && m_mesh->GetBoundingBox( minBounds, maxBounds ) )
	{
		BoundingSphereFromBox( m_boundingSphere, minBounds, maxBounds, &m_worldTransform );
	}

	for( auto it = m_particleEmitters.begin(); it != m_particleEmitters.end(); ++it )
	{
		(*it)->Update( updateContext.GetTime() );
	}
}