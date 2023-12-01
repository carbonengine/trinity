////////////////////////////////////////////////////////////
//
//    Created:   March 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "TriRenderBatch.h"
#include "TriFrustum.h"
#include "Shader/Tr2Effect.h"
#include "EvePlaneSet.h"
#include "EvePlaneSetItem.h"
#include "Utilities/BoundingSphere.h"
#include "Tr2PickingHelperBatch.h"
#include "Tr2DebugRenderer.h"
#include "Tr2Renderer.h"
#include "Utilities/MatrixUtils.h"


static const char* PLANESET_PICK_EFFECT_PATH = "res:/Graphics/Effect/Managed/Space/SpaceObject/FX/PlanePicking.fx";


// vertex layout struct
struct PlaneVertex
{
	Vector4 transform1;
	Vector4 transform2;
	Vector4 transform3;
	Vector4 color;
	Vector4 layer1Transform;
	Vector4 layer2Transform;
	Vector4 layer1Scroll;
	Vector4 layer2Scroll;
	uint8_t index;
	uint8_t boneIndex;
	uint8_t maskMapAtlasIndex;
	uint8_t pickBufferID;
	Vector4 blinkData;
};



using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members
// --------------------------------------------------------------------------------
EvePlaneSet::EvePlaneSet( IRoot* lockobj ) :
	PARENTLOCK( m_planes ),
	m_display( true ),
	m_hideOnLowQuality( false ),
	m_pickBufferID( 0 ),
	m_vertexCount( 0 ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
{
	// create picking effect
	m_pickEffect.CreateInstance();
	m_pickEffect->SetEffectPathName( PLANESET_PICK_EFFECT_PATH );
}

// --------------------------------------------------------------------------------
// Description:
//   Cleanup
// --------------------------------------------------------------------------------
EvePlaneSet::~EvePlaneSet()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Set the main effect of this set from the outside
// --------------------------------------------------------------------------------
void EvePlaneSet::SetEffect( Tr2EffectPtr effect )
{
	m_effect = effect;
}

// --------------------------------------------------------------------------------
// Description:
//   If loading from a .red file, we now can start creating resources
// --------------------------------------------------------------------------------
bool EvePlaneSet::Initialize()
{
	PrepareResources();
	CreateBoundingBoxes();
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   If someone changed some data we must re-create buffers etc.
// --------------------------------------------------------------------------------
bool EvePlaneSet::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_pickBufferID ) )
	{
		Rebuild();
	}
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Allow access to change the pickbuffer ID. (this is so the SOF can set the
//   pickbuffers for hangar/space videos
// --------------------------------------------------------------------------------
void EvePlaneSet::SetPickBufferID( uint8_t pickBufferID )
{
	m_pickBufferID = pickBufferID;

	if( m_planes.empty() )
	{
		return;
	}

	Rebuild();
}

// --------------------------------------------------------------------------------
// Description:
//   We have to free all device stuff, so release vertex declaration and free
//   all the vertex buffer
// --------------------------------------------------------------------------------
void EvePlaneSet::ReleaseResources( TriStorage s )
{
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_vertexBuffer = Tr2BufferAL();
}

// --------------------------------------------------------------------------------
// Description:
//   (Re)-allocate all device stuff: create a vertex declaration for the instanced
//   rendering and a vertexbuffer
// --------------------------------------------------------------------------------
bool EvePlaneSet::OnPrepareResources()
{
	// Always clear the transform cache
	m_cachedTransforms.clear();

	if( m_vertexBuffer.IsValid() )
	{
		return true;
	}

	if( m_planes.empty() )
	{
		return true;
	}

	// register vertex declaration
	static Tr2VertexDefinition s_spriteVertexDecl;
	if( s_spriteVertexDecl.empty() )
	{
		Tr2VertexDefinition& vd = s_spriteVertexDecl;
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 0 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 1 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 2 );
		vd.Add( vd.FLOAT32_4, vd.COLOR );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 3 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 4 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 5 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 6 );
		vd.Add( vd.UBYTE_4, vd.TEXCOORD, 7 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 8 );
	}
	m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( s_spriteVertexDecl );
	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return false;
	}

	// prepare buffers
	m_vertexCount = (unsigned int)m_planes.GetSize() * 4;
	std::vector<PlaneVertex> verts( m_vertexCount );

	// fill it
	unsigned int n = (unsigned int)m_planes.GetSize();
	for( unsigned int i = 0; i < n; ++i )
	{
		// build transformation matrix out of the individual item data
		Matrix itemTransform = TransformationMatrix( m_planes[i]->m_scaling, m_planes[i]->m_rotation, m_planes[i]->m_position );
		for( unsigned int j = 0; j < 4; ++j )
		{
			PlaneVertex& vertex = verts[i * 4 + j];

			vertex.transform1 = Vector4( itemTransform._11, itemTransform._21, itemTransform._31, itemTransform._41 );
			vertex.transform2 = Vector4( itemTransform._12, itemTransform._22, itemTransform._32, itemTransform._42 );
			vertex.transform3 = Vector4( itemTransform._13, itemTransform._23, itemTransform._33, itemTransform._43 );
			vertex.color = Vector4( m_planes[i]->m_color.r, m_planes[i]->m_color.g, m_planes[i]->m_color.b, m_planes[i]->m_color.a );
			vertex.layer1Transform = m_planes[i]->m_layer1Transform;
			vertex.layer2Transform = m_planes[i]->m_layer2Transform;
			vertex.layer1Scroll = m_planes[i]->m_layer1Scroll;
			vertex.layer2Scroll = m_planes[i]->m_layer2Scroll;
			vertex.index = j;
			vertex.boneIndex = m_planes[i]->m_boneIndex;
			vertex.maskMapAtlasIndex = m_planes[i]->m_maskAtlasID;
			vertex.pickBufferID = m_pickBufferID;
			vertex.blinkData = m_planes[i]->m_blinkData;
		}
		// We cache this for updating view distance info
		m_cachedTransforms.push_back( itemTransform );
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();
	CR_RETURN_VAL( m_vertexBuffer.Create( sizeof( PlaneVertex ), m_vertexCount, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, &verts[0], renderContext ), false );

	Tr2Renderer::GetQuadListIndexBuffer( m_vertexCount / 4 );

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Setup instanced reandering and call DIP
// --------------------------------------------------------------------------------
void EvePlaneSet::SubmitGeometry( Tr2RenderContext& renderContext )
{
	renderContext.m_esm.ApplyVertexDeclaration( m_vertexDeclHandle );
	renderContext.m_esm.ApplyStreamSource( 0, m_vertexBuffer, 0, sizeof( PlaneVertex ) );
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
//   Get bounding box around planes, update visibility based on if box is visible or not
// --------------------------------------------------------------------------------------
bool EvePlaneSet::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount )
{
	auto aabb = GetAabb( bones, boneCount );
	if( !aabb.IsInitialized() )
	{
		return false;
	}
	aabb.Transform( parentTransform );

	return frustum.IsBoxVisible( aabb.m_min, aabb.m_max );
}

// --------------------------------------------------------------------------------------
// Description:
//   Get bounding box surrounding planes
// --------------------------------------------------------------------------------------
AxisAlignedBoundingBox EvePlaneSet::GetAabb( const granny_matrix_3x4* bones, size_t boneCount ) const
{
	return GetItemSetAabb( m_aabb, m_boundingBoxes, bones, boneCount );
}

// --------------------------------------------------------------------------------
// Description:
//   Trinity's way of providing batches to render
// --------------------------------------------------------------------------------
void EvePlaneSet::GetBatches( ITriRenderBatchAccumulator* accumulator, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( batchType != TRIBATCHTYPE_ADDITIVE && batchType != TRIBATCHTYPE_PICKING )
	{
		return;
	}

	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	if( !m_vertexBuffer.IsValid() )
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

	// handle different batch types
	switch( batchType )
	{
	case TRIBATCHTYPE_ADDITIVE:
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
		break;
	case TRIBATCHTYPE_PICKING:
		if( m_pickEffect && m_pickBufferID )
		{
			TriForwardingBatch* batch = accumulator->Allocate<TriForwardingBatch>();
			if( batch )
			{
				batch->SetPerObjectData( perObjectData );
				batch->SetShaderMaterial( m_pickEffect );
				batch->SetGeometryProvider( this );
				accumulator->Commit( batch );
			}
		}
		break;
	default:
		return;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Rebuild resources after adding/removing/changing individual planes
// --------------------------------------------------------------------------------
void EvePlaneSet::Rebuild()
{
	ReleaseResources( 0 );
	PrepareResources();
	CreateBoundingBoxes();
}

// --------------------------------------------------------------------------------------
// Description:
//   Create bounding boxes around planes and group together those who have the same bone index
// --------------------------------------------------------------------------------------
void EvePlaneSet::CreateBoundingBoxes()
{
	CreateItemSetBoundingBoxes( m_aabb, m_boundingBoxes, true, begin( m_planes ), end( m_planes ) );
}

// --------------------------------------------------------------------------------
// Description:
//   Add a new plane (aka item) to this set
// Arguments:
//   the new plane
// --------------------------------------------------------------------------------
void EvePlaneSet::AddPlaneItem( EvePlaneSetItemPtr item )
{
	m_planes.Insert( -1, item );
}

void EvePlaneSet::GetPickingBatches( ITriRenderBatchAccumulator* batches, uint16_t& areaIDOffset, const Tr2PerObjectData* perObjectData )
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

EvePlaneSetItemVector* EvePlaneSet::GetPlanes()
{
	return &m_planes;
}

void EvePlaneSet::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Plane Sets" );
	options.insert( "Plane Sets Bounds" );
}

void EvePlaneSet::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount )
{
	if( renderer.HasOption( GetRawRoot(), "Plane Sets" ) )
	{
		for( auto it = m_planes.begin(); it != m_planes.end(); ++it )
		{
			Quaternion rotation( ( *it )->m_rotation );
			Vector3 position( ( *it )->m_position );
			int boneIndex = ( *it )->m_boneIndex;

			if( boneIndex > 0 && boneIndex < int( boneCount ) )
			{
				Matrix boneTF = IdentityMatrix();
				TriMatrixCopyFrom3x4( &boneTF, &bones[boneIndex] );
				position = XMVector3TransformCoord( position, boneTF );

				rotation = XMQuaternionMultiply( rotation, XMQuaternionRotationMatrix( boneTF ) );
			}

			Matrix t( XMMatrixTransformation( Vector3( 0, 0, 0 ), Quaternion( 0, 0, 0, 1 ), ( *it )->m_scaling, Vector3( 0, 0, 0 ), rotation, position ) );
			renderer.DrawBox(
				*it,
				t * parentTransform,
				Vector3( -0.5f, -0.5f, -0.005f ),
				Vector3( 0.5f, 0.5f, 0.005f ),
				Tr2DebugRenderer::Wireframe,
				Tr2DebugColor( Color( 0.0f, 0.7f, 0.9f, 0.5f ), Color( 0.0f, 0.7f, 0.9f, 0.1f ) ) );
			renderer.DrawBox(
				*it,
				t * parentTransform,
				Vector3( -0.5f, -0.5f, -0.005f ),
				Vector3( 0.5f, 0.5f, 0.005f ),
				Tr2DebugRenderer::Solid,
				0 );
		}
	}

	if( renderer.HasOption( GetRawRoot(), "Plane Sets Bounds" ) )
	{
		auto aabb = GetAabb( bones, boneCount );
		renderer.DrawBox(
			Tr2DebugObjectReference( this ),
			parentTransform,
			aabb.m_min,
			aabb.m_max,
			Tr2DebugRenderer::Wireframe,
			0xff00ff00 );
	}
}

void EvePlaneSet::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	if( nullptr != m_effect )
	{
		m_effect->SetOption( name, value );
	}
}
