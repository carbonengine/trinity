#include "StdAfx.h"
#include "EveChildMesh.h"

#include "Tr2MeshArea.h"
#include "Tr2MeshBase.h"

#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/EveTransform.h"
#include "Resources/TriGeometryRes.h"


EveChildMesh::EveChildMesh( IRoot* lockobj ):
	m_display( true ),
	m_useSpaceObjectData( true ),
	EveChildTransform()
{
}

EveChildMesh::~EveChildMesh()
{
}

bool EveChildMesh::Initialize()
{
	if( m_staticTransform )
	{
		RebuildLocalTransform();
	}
	return true;
}

void EveChildMesh::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform )
{
	if( !m_display )
	{
		return;
	}
	renderables.push_back( this );
}

bool EveChildMesh::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	// TODO: leaving as is because for now S.Manekeller wants a child mesh to play around with
	// Fix asap <Logi 27. aug 2015>
	Vector3 transl = m_worldTransform.GetTranslation();
	sphere.x = transl.x;
	sphere.y = transl.y;
	sphere.z = transl.z;
	sphere.w = 100000;
	return true;
}

bool EveChildMesh::HasTransparentBatches()
{
	if( m_display && m_mesh )
	{
		return !(m_mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT )->empty());
	}

	return false;
}

void EveChildMesh::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData )
{
	if( m_display && m_mesh )
	{
		m_mesh->GetBatches( batches, m_mesh->GetAreas( batchType ), perObjectData );
	}
}

void EveChildMesh::GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData )
{
	// TODO: Figure out what we want to do with shadows
	// Fix asap <Logi 27. aug 2015>
	if( m_display && m_mesh )
	{
		m_mesh->GetBatches( batches, m_mesh->GetAreas( TRIBATCHTYPE_OPAQUE ), perObjectData );
	}
}

float EveChildMesh::GetSortValue()
{
	Vector3 d = Tr2Renderer::GetViewPosition() - m_worldTransform.GetTranslation();
	float distance = D3DXVec3Length( &d );
	return distance;
}

Tr2PerObjectData* EveChildMesh::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	if( !m_useSpaceObjectData )
	{
		EveBasicPerObjectData* perObjectData = accumulator->Allocate<EveBasicPerObjectData>();

		if( !perObjectData )
		{
			return NULL;
		}

		perObjectData->m_world = m_vsData.worldTransform;
		D3DXMatrixInverse( &perObjectData->m_worldInverseTranspose, NULL, &m_worldTransform );
		return perObjectData;
	}
	
	Tr2PerObjectDataWithPersistentBuffers<EveChildMesh>* perObjectData = accumulator->Allocate<Tr2PerObjectDataWithPersistentBuffers<EveChildMesh>>();
	if( !perObjectData )
	{
		return NULL;
	}
	perObjectData->Initialize( this, &m_perObjectDataVs, &m_perObjectDataPs );

	return perObjectData;
}

uint32_t EveChildMesh::GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const
{
	if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
	{
		return sizeof( m_psData );
	}
	else
	{
		return sizeof( m_vsData );
	}
}

void EveChildMesh::UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* data )
{
	if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
	{
		uint8_t* perObjectPS = (uint8_t*)data;
		memcpy( perObjectPS, &m_psData, sizeof( m_psData ) );
	}
	else
	{
		uint8_t* perObjectVS = (uint8_t*)data;
		memcpy( perObjectVS, &m_vsData, sizeof( m_vsData ) );
	}
}

void EveChildMesh::UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* parent )
{
}

void EveChildMesh::UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* parent )
{
	m_perObjectDataVs.InvalidateBufferData();
	m_perObjectDataPs.InvalidateBufferData();

	EveSpaceObject2Ptr parentSpaceObject;
	if( parent->QueryInterface( BlueInterfaceIID<EveSpaceObject2>(), (void**)&parentSpaceObject ) )
	{
		parentSpaceObject->GetPerObjectStructs( m_vsData, m_psData );
		m_vsData.worldTransformLast = m_worldTransform;
	}

	Matrix localToWorldTransform;
	parent->GetLocalToWorldTransform( localToWorldTransform );
	UpdateTransform( localToWorldTransform );
	D3DXMatrixTranspose( &m_vsData.worldTransform, &m_worldTransform );
}

void EveChildMesh::UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObjectChild* parent )
{
}

void EveChildMesh::UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObjectChild* parent )
{
	m_perObjectDataVs.InvalidateBufferData();
	m_perObjectDataPs.InvalidateBufferData();

	EveSpaceObject2Ptr parentSpaceObject;
	if( parent->QueryInterface( BlueInterfaceIID<EveSpaceObject2>(), (void**)&parentSpaceObject ) )
	{
		parentSpaceObject->GetPerObjectStructs( m_vsData, m_psData );
	}

	m_vsData.worldTransformLast = m_worldTransform;

	Matrix localToWorldTransform;
	parent->GetLocalToWorldTransform( localToWorldTransform );
	UpdateTransform( localToWorldTransform );
	D3DXMatrixTranspose( &m_vsData.worldTransform, &m_worldTransform );
}

void EveChildMesh::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

void EveChildMesh::SetMesh( Tr2MeshBase* mesh )
{
	m_mesh = mesh;
}