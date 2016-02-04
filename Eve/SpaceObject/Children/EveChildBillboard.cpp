////////////////////////////////////////////////////////////
//
//    Created:   June 2015
//    Copyright: CCP 2015
//
#include "StdAfx.h"
#include "EveChildBillboard.h"

#include "Tr2MeshArea.h"
#include "Tr2MeshBase.h"
#include "TriFrustum.h"

#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/EveTransform.h"
#include "Resources/TriGeometryRes.h"
#include "Utilities/BoundingSphere.h"

EveChildBillboard::EveChildBillboard( IRoot* lockobj ):
	EveChildTransform()
{
}

EveChildBillboard::~EveChildBillboard()
{
}

bool EveChildBillboard::Initialize()
{
	if( m_staticTransform )
	{
		RebuildLocalTransform();
	}
	return true;
}

void EveChildBillboard::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform )
{
	Vector4 boundingSphere;
	if( !m_display )
	{
		return;
	}

	if( GetBoundingSphere( boundingSphere ) && frustum.IsSphereVisible( &boundingSphere ) )
	{
		renderables.push_back( this );
	}
}

bool EveChildBillboard::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	Vector3 minBounds, maxBounds;
	if( m_mesh && m_mesh->GetBoundingBox( minBounds, maxBounds ) )
	{
		BoundingSphereFromBox( sphere, minBounds, maxBounds, &m_worldTransform );
	}
	else
	{
		return false;
	}

	return true;
}

bool EveChildBillboard::HasTransparentBatches()
{
	if( m_display && m_mesh )
	{
		return !(m_mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT )->empty());
	}

	return false;
}

void EveChildBillboard::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData )
{
	if( m_display && m_mesh )
	{
		m_mesh->GetBatches( batches, m_mesh->GetAreas( batchType ), perObjectData );
	}
}

void EveChildBillboard::GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData )
{
	if( m_display && m_mesh )
	{
		m_mesh->GetBatches( batches, m_mesh->GetAreas( TRIBATCHTYPE_OPAQUE ), perObjectData );
	}
}

float EveChildBillboard::GetSortValue()
{
	Vector3 d = Tr2Renderer::GetViewPosition() - m_worldTransform.GetTranslation();
	float distance = D3DXVec3Length( &d );
	return distance;
}


void EveChildBillboard::UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* parent )
{
}

void EveChildBillboard::UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* parent )
{
	Matrix localToWorldTransform;
	parent->GetLocalToWorldTransform( localToWorldTransform );
	UpdateTransform( localToWorldTransform );
}

void EveChildBillboard::UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObjectChild* parent )
{
}

void EveChildBillboard::UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObjectChild* parent )
{
	Matrix localToWorldTransform;
	parent->GetLocalToWorldTransform( localToWorldTransform );
	
	// Do the billboard magic
	Matrix invView = Tr2Renderer::GetInverseViewTransform();
	invView._41 = 0.0;
	invView._42 = 0.0;
	invView._43 = 0.0;
	invView._44 = 1.0;
	D3DXMatrixMultiply( &localToWorldTransform, &localToWorldTransform, &invView );

	UpdateTransform( localToWorldTransform );
}

void EveChildBillboard::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

Tr2PerObjectData* EveChildBillboard::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
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