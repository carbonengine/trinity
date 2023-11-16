////////////////////////////////////////////////////////////
//
//    Created:   November 2017
//    Copyright: CCP 2017
//
#include "StdAfx.h"
#include "TriRenderBatch.h"
#include "TriFrustum.h"
#include "Shader/Tr2Effect.h"
#include "EveHazeSet.h"
#include "EveHazeSetItem.h"
#include "Utilities/BoundingSphere.h"
#include "Tr2PickingHelperBatch.h"
#include "Tr2DebugRenderer.h"
#include "Tr2Renderer.h"
#include "Utilities/MatrixUtils.h"


// vertex layout struct
struct HazeVertex
{
	Vector4 transform1;
	Vector4 transform2;
	Vector4 transform3;
	Vector4 invTransform1;
	Vector4 invTransform2;
	Vector4 invTransform3;
	Vector4 hazeData;
	Vector4 color;
	uint8_t index;
	uint8_t boneIndex;
	// cppcheck-suppress unusedStructMember
	uint8_t padding0;
	// cppcheck-suppress unusedStructMember
	uint8_t padding1;
};


using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EveHazeSet::EveHazeSet( IRoot* lockobj ) :
	PARENTLOCK( m_hazes ),
	m_display( true ),
	m_vertexCount( 0 ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Cleanup
// --------------------------------------------------------------------------------
EveHazeSet::~EveHazeSet()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Seup this guy from the outside
// --------------------------------------------------------------------------------
void EveHazeSet::Setup( Tr2EffectPtr effect )
{
	m_effect = effect;
}

// --------------------------------------------------------------------------------
// Description:
//   If loading from a .red file, we now can start creating resources
// --------------------------------------------------------------------------------
bool EveHazeSet::Initialize()
{
	PrepareResources();
	CreateBoundingBox();
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   We have to free all device stuff, so release vertex declaration and free
//   all the vertex buffer
// --------------------------------------------------------------------------------
void EveHazeSet::ReleaseResources( TriStorage s )
{
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_vertexBuffer = Tr2BufferAL();
}


// --------------------------------------------------------------------------------
// Description:
//   (Re)-allocate all device stuff: create a vertex declaration for the instanced
//   rendering and a vertexbuffer
// --------------------------------------------------------------------------------
bool EveHazeSet::OnPrepareResources()
{
	// Always clear the transform cache
	m_cachedTransforms.clear();

	if( m_vertexBuffer.IsValid() )
	{
		return true;
	}

	if( m_hazes.empty() )
	{
		return true;
	}

	// register vertex declaration
	static Tr2VertexDefinition s_hazeVertexDecl;
	if( s_hazeVertexDecl.empty() )
	{
		Tr2VertexDefinition& vd = s_hazeVertexDecl;
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 0 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 1 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 2 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 3 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 4 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 5 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 6 );
		vd.Add( vd.FLOAT32_4, vd.COLOR );
		vd.Add( vd.UBYTE_4, vd.TEXCOORD, 7 );
	}
	m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( s_hazeVertexDecl );
	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return false;
	}

	// prepare buffers
	m_vertexCount = 24 * (unsigned int)m_hazes.GetSize();
	std::vector<HazeVertex> verts( m_vertexCount );

	// some index helpers to create a box
	static uint8_t s_boxInds[6][4] = {
		{  0,  1,  2,  3 },
		{  7,  6,  5,  4 },
		{  0,  4,  5,  1 },
		{  3,  2,  6,  7 },
		{  1,  5,  6,  2 },
		{  4,  0,  3,  7 } };

	// fill it
	unsigned int n = (unsigned int)m_hazes.GetSize();
	for( unsigned int i = 0; i < n; ++i )
	{
		// build transformation matrix out of the individual item data
		Matrix itemTransform = TransformationMatrix( m_hazes[i]->m_scaling, m_hazes[i]->m_rotation, m_hazes[i]->m_position );
		Matrix invItemTransform = Inverse( itemTransform );
		for( unsigned int s = 0; s < 6; ++s )
		{
			for( unsigned int j = 0; j < 4; ++j )
			{
				HazeVertex& vertex = verts[i * 24 + s * 4 + j];

				vertex.transform1 = Vector4( itemTransform._11, itemTransform._21, itemTransform._31, itemTransform._41 );
				vertex.transform2 = Vector4( itemTransform._12, itemTransform._22, itemTransform._32, itemTransform._42 );
				vertex.transform3 = Vector4( itemTransform._13, itemTransform._23, itemTransform._33, itemTransform._43 );
				vertex.invTransform1 = Vector4( invItemTransform._11, invItemTransform._21, invItemTransform._31, invItemTransform._41 );
				vertex.invTransform2 = Vector4( invItemTransform._12, invItemTransform._22, invItemTransform._32, invItemTransform._42 );
				vertex.invTransform3 = Vector4( invItemTransform._13, invItemTransform._23, invItemTransform._33, invItemTransform._43 );
				vertex.hazeData = m_hazes[i]->m_hazeData;
				vertex.color = Vector4( m_hazes[i]->m_color.r, m_hazes[i]->m_color.g, m_hazes[i]->m_color.b, m_hazes[i]->m_color.a );
				vertex.index = s_boxInds[s][j];
				vertex.boneIndex = m_hazes[i]->m_boneIndex;
			}
		}
		// We cache this for updating view distance info
		m_cachedTransforms.push_back( itemTransform );
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();
	CR_RETURN_VAL( m_vertexBuffer.Create( sizeof( HazeVertex ), m_vertexCount, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, &verts[0], renderContext ), false );

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Setup rendering and call DIP
// --------------------------------------------------------------------------------
void EveHazeSet::SubmitGeometry( Tr2RenderContext& renderContext )
{
	renderContext.m_esm.ApplyVertexDeclaration( m_vertexDeclHandle );
	renderContext.m_esm.ApplyStreamSource( 0, m_vertexBuffer, 0, sizeof( HazeVertex ) );
	auto ib = Tr2Renderer::GetQuadListIndexBuffer( m_vertexCount / 4 );
	if( !ib )
	{
		return;
	}
	renderContext.m_esm.ApplyIndexBuffer( *ib );
	renderContext.SetTopology( TOP_TRIANGLES );
	renderContext.DrawIndexedPrimitive( m_vertexCount, 0, m_vertexCount / 2 );
}

// --------------------------------------------------------------------------------------
// Description:
//   Update visibility based on if bounding box surrounding the hazes is visible or not.
// --------------------------------------------------------------------------------------
bool EveHazeSet::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount )
{
	auto aabb = GetItemSetAabb( m_aabb, m_boundingBoxes, bones, boneCount );
	if( !aabb.IsInitialized() )
	{
		return false;
	}
	aabb.Transform( parentTransform );
	return frustum.IsBoxVisible( aabb.m_min, aabb.m_max );
}

// --------------------------------------------------------------------------------------
// Description:
//   Create a bounding box around all HazeSetItems
// --------------------------------------------------------------------------------------
void EveHazeSet::CreateBoundingBox()
{
	CreateItemSetBoundingBoxes( m_aabb, m_boundingBoxes, true, begin( m_hazes ), end( m_hazes ) );
}

// --------------------------------------------------------------------------------
// Description:
//   Trinity's way of providing batches to render
// --------------------------------------------------------------------------------
void EveHazeSet::GetBatches( ITriRenderBatchAccumulator* accumulator, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( batchType != TRIBATCHTYPE_ADDITIVE || !m_vertexBuffer.IsValid() || reason == Tr2RenderReason::TR2RENDERREASON_REFLECTION)
	{
		return;
	} 

	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return;
	}

	if( !m_display )
	{
		return;
	}

	// only additive
	if( batchType == TRIBATCHTYPE_ADDITIVE )
	{
		if( m_effect )
		{
			TriForwardingBatch* batch = accumulator->Allocate<TriForwardingBatch>();
			if( batch )
			{
				batch->SetPerObjectData( perObjectData );
				batch->SetShaderMaterial( m_effect );
				batch->SetGeometryProvider( this );
				accumulator->Commit( batch );
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Rebuild resources after adding/removing/changing individual hazes
// --------------------------------------------------------------------------------
void EveHazeSet::Rebuild()
{
	ReleaseResources( 0 );
	PrepareResources();
	CreateBoundingBox();
}

// --------------------------------------------------------------------------------
// Description:
//   Add a new haze (aka item) to this set
// Arguments:
//   the new haze
// --------------------------------------------------------------------------------
void EveHazeSet::AddHazeItem( EveHazeSetItemPtr item )
{
	m_hazes.Insert( -1, item );
}

// --------------------------------------------------------------------------------
void EveHazeSet::GetPickingBatches( ITriRenderBatchAccumulator* batches, uint16_t& areaIDOffset, const Tr2PerObjectData* perObjectData )
{
	for( auto it = m_cachedTransforms.begin(); it != m_cachedTransforms.end(); ++it )
	{
		if( auto batch = batches->Allocate<Tr2PickingHelperBatch>() )
		{
			batch->SetPerObjectData( perObjectData );
			batch->AddBox( *it );
			batch->SetAreaID( areaIDOffset );
			batches->Commit( batch );
		}
		else
		{
			break;
		}
		++areaIDOffset;
	}
}

// --------------------------------------------------------------------------------
void EveHazeSet::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Haze Sets" );
}

// --------------------------------------------------------------------------------
void EveHazeSet::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount )
{
	if( renderer.HasOption( GetRawRoot(), "Haze Sets" ) )
	{
		for( auto it = m_hazes.begin(); it != m_hazes.end(); ++it )
		{
			auto haze = *it;
			int32_t boneIndex = haze->m_boneIndex;
			Quaternion rotation( haze->m_rotation );
			Vector3 position( haze->m_position );
			
			if( boneIndex > -1 && boneIndex < int( boneCount ) )
			{
				Matrix boneTF = IdentityMatrix();
				TriMatrixCopyFrom3x4( &boneTF, &bones[boneIndex] );
				position = XMVector3TransformCoord( position, boneTF );

				rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationMatrix( boneTF ) );
			}
			Matrix t( XMMatrixTransformation( Vector3( 0, 0, 0 ), Quaternion( 0, 0, 0, 1 ), ( haze )->m_scaling, Vector3( 0, 0, 0 ), rotation, position ) );
			renderer.DrawBox( haze, t * parentTransform, Vector3( -0.5f, -0.5f, -0.5f ), Vector3( 0.5f, 0.5f, 0.5f ), Tr2DebugRenderer::Wireframe, Tr2DebugColor( 0xff00ffff, 0x2200ffff ) );
			renderer.DrawBox( haze, t * parentTransform, Vector3( -0.5f, -0.5f, -0.5f ), Vector3( 0.5f, 0.5f, 0.5f ), Tr2DebugRenderer::Solid, 0 );
		}
	}
}

void EveHazeSet::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	if( nullptr != m_effect )
	{
		m_effect->SetOption( name, value );
	}
}
