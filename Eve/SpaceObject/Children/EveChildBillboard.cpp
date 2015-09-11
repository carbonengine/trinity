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
	m_translation( 0, 0, 0 ),
	m_scaling( 0, 0, 0 )
{
}

EveChildBillboard::~EveChildBillboard()
{
}


void EveChildBillboard::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform )
{
	Vector4 boundingSphere;
	if( !m_display )
	{
		return;
	}

	// Do the billboard magic
	const Matrix& invView = Tr2Renderer::GetInverseViewTransform();
	m_worldTransform._11 = invView._11 * m_scaling.x;
	m_worldTransform._12 = invView._12 * m_scaling.x;
	m_worldTransform._13 = invView._13 * m_scaling.x;
	m_worldTransform._21 = invView._21 * m_scaling.y;
	m_worldTransform._22 = invView._22 * m_scaling.y;
	m_worldTransform._23 = invView._23 * m_scaling.y;
	m_worldTransform._31 = invView._31 * m_scaling.z;
	m_worldTransform._32 = invView._32 * m_scaling.z;
	m_worldTransform._33 = invView._33 * m_scaling.z;

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


void EveChildBillboard::UpdateSyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent )
{
}

void EveChildBillboard::UpdateAsyncronous( EveUpdateContext& updateContext, EveSpaceObject2* parent )
{
	Matrix localTransform, localToWorldTransform;
	D3DXMatrixTransformation( &localTransform, 0, 0, &m_scaling, 0, 0, &m_translation );
	parent->GetLocalToWorldTransform( localToWorldTransform );
	D3DXMatrixMultiply( &m_worldTransform, &localTransform, &localToWorldTransform );
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