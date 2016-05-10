////////////////////////////////////////////////////////////
//
//    Created:   February 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "EveCloud.h"
#include "TriFrustum.h"
#include "TriRenderBatch.h"
#include "include/ITriFunction.h"
#include "Utilities/BoundingSphere.h"
#include "Utilities/BoundingBox.h"
#include "Tr2PerObjectData.h"
#include "EveCloudEditableVolume.h"
#include "../EveUpdateContext.h"

using namespace Tr2RenderContextEnum;

namespace
{

class EveCloudPerObjectData : public Tr2PerObjectData
{
public:
	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
	{
		static const unsigned mask = 
			( 1 << VERTEX_SHADER )					|
			SHADER_TYPE_EXISTS( COMPUTE_SHADER )	|
			SHADER_TYPE_EXISTS( GEOMETRY_SHADER )	|
			SHADER_TYPE_EXISTS( HULL_SHADER )		|
			SHADER_TYPE_EXISTS( DOMAIN_SHADER)		;
		FillAndSetConstants(	
			*buffers[Tr2RenderContextEnum::CBUFFER_FFE],
			&m_data, sizeof( m_data ),
			mask,
			Tr2Renderer::GetPerObjectVSStartRegister(),
			renderContext );
	}

	struct Data
	{
		Matrix m_world;
		Matrix m_worldView;
		Matrix m_worldViewInv;
		Matrix m_projectionInv;
		Vector4 m_nearPlaneLocal;
		Vector3 m_eyePosLocal;
		float m_screenDepth;
		Vector4 m_screenSize;
	} m_data;
};

// --------------------------------------------------------------------------------------
// Description:
//   Calculates axis aligned bounding box of a given box in projection space. The 
//   function projects all the vertices, discards vertices outside frusum (ignoring far
//   plane), clips edges with near plane and computes AABB clipped to [-1, 1] range. 
//   Function assumes non-orthographic projection.
// --------------------------------------------------------------------------------------
void GetProjectedCubeBounds(  AxisAlignedBoundingBox& box, const Matrix& worldView, const Matrix& proj, float nearPlane, const Vector3 min, const Vector3 max  )
{
	Vector3 sides[6][4] = {
		{
			Vector3( min.x, min.y, min.z ),
			Vector3( min.x, max.y, min.z ),
			Vector3( min.x, max.y, max.z ),
			Vector3( min.x, min.y, max.z ),
		},
		{
			Vector3( max.x, min.y, min.z ),
			Vector3( max.x, min.y, max.z ),
			Vector3( max.x, max.y, max.z ),
			Vector3( max.x, max.y, min.z ),
		},
		{
			Vector3( min.x, min.y, min.z ),
			Vector3( max.x, min.y, min.z ),
			Vector3( max.x, min.y, max.z ),
			Vector3( min.x, min.y, max.z ),
		},
		{
			Vector3( min.x, max.y, min.z ),
			Vector3( min.x, max.y, max.z ),
			Vector3( max.x, max.y, max.z ),
			Vector3( max.x, max.y, min.z ),
		},
		{
			Vector3( min.x, min.y, min.z ),
			Vector3( max.x, min.y, min.z ),
			Vector3( max.x, max.y, min.z ),
			Vector3( min.x, max.y, min.z ),
		},
		{
			Vector3( min.x, min.y, max.z ),
			Vector3( min.x, max.y, max.z ),
			Vector3( max.x, max.y, max.z ),
			Vector3( max.x, min.y, max.z ),
		},
	};

	for( int i = 0; i < 6; ++i )
	{
		for( int j = 0; j < 4; ++j )
		{
			D3DXVec3TransformCoord( &sides[i][j], &sides[i][j], &worldView );
		}
	}

	Vector4 points[48];
	int count = 0;

	D3DXPLANE plane( 0, 0, 1, nearPlane );

	for( int side = 0; side < 6; ++side )
	{
		for( int edge = 0; edge < 4; ++edge )
	{
			const Vector3& vertex1 = sides[side][edge];
			const Vector3& vertex2 = sides[side][( edge + 1 ) % 4];
			float v0 = D3DXPlaneDotCoord( &plane, &vertex1 );
			float v1 = D3DXPlaneDotCoord( &plane, &vertex2 );
			if( v0 <= 0 )
		{
				points[count++] = Vector4( sides[side][edge], 0.f );
		}
			if( v0 * v1 < 0 )
		{
				Vector3 result;
				D3DXPlaneIntersectLine( &result, &plane, &vertex1, &vertex2 );
				points[count++] = Vector4( result, 0.f );
			}
		}
		}

	D3DXVec4TransformArray( points, sizeof( Vector4 ), points, sizeof( Vector4 ), &proj, count );
	for( int i = 0; i < count; ++i )
	{
		points[i] /= points[i].w;
		points[i].x = std::min( std::max( points[i].x, -1.f ), 1.f );
		points[i].y = std::min( std::max( points[i].y, -1.f ), 1.f );
	}

	box = AxisAlignedBoundingBox( Vector3( &points[0].x ), Vector3( &points[0].x ) );
	for( int i = 0; i < count; ++i )
	{
		box.IncludePoint( Vector3( &points[i].x ) );
	}
}

}



EveCloud::EveCloud( IRoot* lockobj )
	:m_localTransform( Tr2Renderer::GetIdentityTransform() ),
	m_worldTransform( Tr2Renderer::GetIdentityTransform() ),
	m_scaling( 1.0f, 1.0f, 1.0f ),
	m_translation( 0.f, 0.f, 0.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.0f ),
	m_display( true ),
	m_declaration( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_preTesselationLevel( 32 ),
	m_min( -0.5f, -0.5f, -0.5f ),
	m_max( 0.5f, 0.5f, 0.5f ),
	m_sortingModifier( 1.0f )
{
	PrepareResources();
}

EveCloud::~EveCloud()
{
}

bool EveCloud::Initialize()
{
	ReleaseResources( TRISTORAGE_ALL );
	PrepareResources();
	return true;
}

bool EveCloud::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_preTesselationLevel ) )
	{
		ReleaseResources( TRISTORAGE_ALL );
		PrepareResources();
	}
	return true;
}

void EveCloud::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform, Tr2Lod parentLod )
{
	if( !m_display || !frustum.IsSphereVisible( &m_boundingSphere ) )
	{
		return;
	}
	renderables.push_back( this );
}

bool EveCloud::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	sphere = m_boundingSphere;
	return true;
}

void EveCloud::GetLocalToWorldTransform( Matrix &transform ) const 
{ 
	transform = m_worldTransform;
}

bool EveCloud::HasTransparentBatches()
{
	return true;
}

float EveCloud::GetSortValue()
{
	Vector3 d = Tr2Renderer::GetViewPosition() - m_worldTransform.GetTranslation();
	float distance = D3DXVec3Length( &d ) - D3DXVec3Length( &m_scaling ) * m_sortingModifier;
	return distance;
}

void EveCloud::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData )
{
	if( batchType != TRIBATCHTYPE_TRANSPARENT )
	{
		return;
	}
	TriForwardingBatch* batch = batches->Allocate<TriForwardingBatch>();
	if( batch )
	{
		batch->SetPerObjectData( perObjectData );
		batch->SetShaderMaterial( m_effect );
		batch->SetGeometryProvider( this );
		batch->SetRenderingMode( Tr2EffectStateManager::RM_ALPHA );
		batches->Commit( batch );
	}
}

void EveCloud::ReleaseResources( TriStorage s )
{
	m_declaration = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	if( m_vertexBuffer.IsValid() && m_vertexBuffer.GetMemoryClass() & s )
	{
		m_vertexBuffer.Destroy();
	}
	if( m_indexBuffer.IsValid() && m_indexBuffer.GetMemoryClass() & s )
	{
		m_indexBuffer.Destroy();
	}
}

bool EveCloud::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	const uint32_t dimension = m_preTesselationLevel;

	if( !m_vertexBuffer.IsValid() )
	{
		std::vector<Vector2> data( ( dimension + 1 ) * ( dimension + 1 ) );
		for( uint32_t j = 0; j <= dimension; ++j )
		{
			for( uint32_t i = 0; i <= dimension; ++i )
			{
				data[i + j * ( dimension + 1 )] = Vector2( ( float( i ) - 0.5f + 0.5f * ( j % 2 ) ) / ( dimension - 0.5f ) * 2 - 1, ( float( j ) / dimension * 2 - 1 ) );
			}
		}
		CR_RETURN_VAL( m_vertexBuffer.Create( sizeof( Vector2 ) * uint32_t( data.size() ), USAGE_IMMUTABLE, &data.front(), renderContext ), false );
	}

	if( !m_indexBuffer.IsValid() )
	{
		std::vector<uint16_t> data( dimension * dimension * 6 );
		uint32_t index = 0;
		for( uint32_t j = 0; j < dimension; ++j )
		{
			for( uint32_t i = 0; i < dimension; ++i )
			{
				if( j % 2 )
				{
					data[index++] = i + j * ( dimension + 1 );
					data[index++] = i + 1 + j * ( dimension + 1 );
					data[index++] = i + 1 + ( j + 1 ) * ( dimension + 1 );
					data[index++] = i + 1 + ( j + 1 ) * ( dimension + 1 );
					data[index++] = i + ( j + 1 ) * ( dimension + 1 );
					data[index++] = i + j * ( dimension + 1 );
				}
				else
				{
					data[index++] = i + j * ( dimension + 1 );
					data[index++] = i + 1 + j * ( dimension + 1 );
					data[index++] = i + ( j + 1 ) * ( dimension + 1 );
					data[index++] = i + ( j + 1 ) * ( dimension + 1 );
					data[index++] = i + 1 + j * ( dimension + 1 );
					data[index++] = i + 1 + ( j + 1 ) * ( dimension + 1 );
				}
			}
		}
		CR_RETURN_VAL( m_indexBuffer.Create( uint32_t( data.size() ), USAGE_IMMUTABLE, IB_16BIT, &data.front(), renderContext ), false );
	}
	if( m_declaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		static Tr2VertexDefinition s_decl;
		if( s_decl.empty() )
		{
			s_decl.Add( s_decl.FLOAT32_2, s_decl.POSITION );
		}

		m_declaration = renderContext.m_esm.GetVertexDeclarationHandle( s_decl );
	}
	return true;
}

Tr2PerObjectData* EveCloud::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	EveCloudPerObjectData* data = accumulator->Allocate<EveCloudPerObjectData>();

	if( !data )
	{
		return NULL;
	}

	D3DXMatrixTranspose( &data->m_data.m_world, &m_worldTransform );

	Matrix worldView = m_worldTransform * Tr2Renderer::GetViewTransform();
	D3DXMatrixTranspose( &data->m_data.m_worldView, &worldView );

	Matrix worldViewProjection = m_worldTransform * Tr2Renderer::GetViewTransform() * Tr2Renderer::GetProjectionTransform();

	Matrix worldViewTransposed;
	D3DXMatrixTranspose( &worldViewTransposed, &worldView );
	Vector4 nearPlane( 0.f, 0.f, -1.f, -Tr2Renderer::GetFrontClip() );
	D3DXVec4Transform( &data->m_data.m_nearPlaneLocal, &nearPlane, &worldViewTransposed );

	Matrix worldViewInv;
	D3DXMatrixInverse( &worldViewInv, nullptr, &worldView );
	D3DXMatrixTranspose( &data->m_data.m_worldViewInv, &worldViewInv );
	Vector3 zero( 0.f, 0.f, 0.f );
	D3DXVec3TransformCoord( &data->m_data.m_eyePosLocal, &zero, &worldViewInv );

	const float fakeNearPlane = 500;
	// We modify the original projection matrix by changing clip planes to avoid precision problems
	Matrix fakeProjection = Tr2Renderer::GetProjectionTransform();
	fakeProjection._33 = -1;
	fakeProjection._43 = -fakeNearPlane * 2;
	Matrix fakeProjectionInv;
	D3DXMatrixInverse( &fakeProjectionInv, nullptr, &fakeProjection );
	D3DXMatrixTranspose( &data->m_data.m_projectionInv, &fakeProjectionInv );

	Vector3 center;
	D3DXVec3TransformCoord( &center, &zero, &worldViewProjection );
	data->m_data.m_screenDepth = center.z;

	AxisAlignedBoundingBox box;
	GetProjectedCubeBounds( box, m_worldTransform * Tr2Renderer::GetViewTransform(), fakeProjection, 1, m_min, m_max );

	data->m_data.m_screenSize.x = box.m_min.x;
	data->m_data.m_screenSize.y = box.m_min.y;
	data->m_data.m_screenSize.z = box.m_max.x;
	data->m_data.m_screenSize.w = box.m_max.y;

	return data;
}

void EveCloud::SubmitGeometry( Tr2RenderContext& renderContext )
{
	renderContext.m_esm.ApplyVertexDeclaration( m_declaration );
	renderContext.m_esm.ApplyIndexBuffer( m_indexBuffer );
	renderContext.m_esm.ApplyStreamSource( 0, m_vertexBuffer, 0, sizeof( Vector2 ) );
	renderContext.SetTopology( TOP_TRIANGLES );
	renderContext.DrawIndexedPrimitive( m_vertexBuffer.GetTotalSizeInBytes() / sizeof( Vector2 ), 0, m_indexBuffer.GetNumIndices() / 3 );
}

IRoot* EveCloud::GetID( uint16_t areaId )
{
	return m_volume ? m_volume->GetID( areaId ) : nullptr;
}

void EveCloud::GetPickingBatches( ITriRenderBatchAccumulator* batches, Tr2PickTypes pickTypes, const Tr2PerObjectData* perObjectData )
{
	if( ( pickTypes & PICK_TYPE_LOCATORS ) != 0 && m_volume )
	{
		m_volume->GetPickingBatches( batches, perObjectData );
	}
}

void EveCloud::UpdateSyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent )
{
	if( m_volume )
	{
		m_volume->Update( updateContext.GetTime() );
	}

	D3DXMatrixTransformation( &m_localTransform, nullptr, nullptr, &m_scaling, nullptr, &m_rotation, &m_translation );

	Matrix parent;
	if( childParent )
	{
		childParent->GetLocalToWorldTransform( parent );
	}
	else
	{
		spaceObjectParent->GetLocalToWorldTransform( parent );
	}
	D3DXMatrixMultiply( &m_worldTransform, &m_localTransform, &parent );
	BoundingSphereFromBox( m_boundingSphere, m_min, m_max, &m_worldTransform );
}

void EveCloud::UpdateAsyncronous( EveUpdateContext& updateContext, IEveSpaceObject2* spaceObjectParent, IEveSpaceObjectChild* childParent )
{
	BoundingSphereFromBox( m_boundingSphere, m_min, m_max, &m_worldTransform );
}

void EveCloud::PlayCurveSet( const std::string& name )
{
}

void EveCloud::StopCurveSet( const std::string& name )
{
}

float EveCloud::GetCurveSetDuration( const std::string& name ) const
{
	return 0;
}
