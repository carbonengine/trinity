////////////////////////////////////////////////////////////
//
//    Created:   March 2013
//    Copyright: CCP 2013
//
#include "StdAfx.h"
#include "TriRenderBatch.h"
#include "TriFrustum.h"
#include "Tr2Effect.h"
#include "EvePlaneSet.h"
#include "EvePlaneSetItem.h"
#include "Utilities/BoundingSphere.h"
#include "Utilities/ViewDistanceInfo.h"

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
	Vector4 scaling;
	uint8_t index;
	uint8_t boneIndex;

	uint8_t padding[2];
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
	m_vertexCount( 0 ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
{
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
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   We have to free all device stuff, so release vertex declaration and free
//   all the vertex buffer
// --------------------------------------------------------------------------------
void EvePlaneSet::ReleaseResources( TriStorage s )
{
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_vertexBuffer.Destroy();
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
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 7 );
		vd.Add( vd.UBYTE_4, vd.TEXCOORD, 8 );
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
		Matrix itemTransform;
		D3DXMatrixTransformation( &itemTransform, NULL, NULL, NULL, NULL, &m_planes[i]->m_rotation, &m_planes[i]->m_position );
		for( unsigned int j = 0; j < 4; ++j )
		{
			PlaneVertex& vertex = verts[i * 4 + j];

			vertex.transform1 = Vector4( itemTransform._11, itemTransform._21, itemTransform._31, itemTransform._41 );
			vertex.transform2 = Vector4( itemTransform._12, itemTransform._22, itemTransform._32, itemTransform._42 );
			vertex.transform3 = Vector4( itemTransform._13, itemTransform._23, itemTransform._33, itemTransform._43 );
			vertex.scaling = Vector4( m_planes[i]->m_scaling, 0.f );
			vertex.color = Vector4( m_planes[i]->m_color.r, m_planes[i]->m_color.g, m_planes[i]->m_color.b, m_planes[i]->m_color.a );
			vertex.layer1Transform = m_planes[i]->m_layer1Transform;
			vertex.layer2Transform = m_planes[i]->m_layer2Transform;
			vertex.layer1Scroll = m_planes[i]->m_layer1Scroll;
			vertex.layer2Scroll = m_planes[i]->m_layer2Scroll;
			vertex.index = j;
			vertex.boneIndex = m_planes[i]->m_boneIndex;
		}
		// We cache this for updating view distance info
		m_cachedTransforms.push_back( itemTransform );
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();
	CR_RETURN_VAL( m_vertexBuffer.Create( m_vertexCount * sizeof( PlaneVertex ), USAGE_IMMUTABLE, &verts[0], renderContext ), false );

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

// --------------------------------------------------------------------------------
// Description:
//   Trinity's way of providing batches to render
// --------------------------------------------------------------------------------
void EvePlaneSet::GetBatches( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData )
{
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	if( !m_vertexBuffer.IsValid() || !m_effect )
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

	TriForwardingBatch* batch = accumulator->Allocate<TriForwardingBatch>();
	if( batch )
	{
		batch->SetPerObjectData( perObjectData );
		batch->SetShaderMaterial( m_effect );
		batch->SetGeometryProvider( this );
		accumulator->Commit( batch );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Update view distance info based on an estimate of the plane set's size.
//   Each 'plane' is a square unit quad centered at the origin. See PlaneGlow.fx
// Arguments:
//   frustum - the frustum
//   viewDistance - the ViewDistanceInfo stuct that we want to update
//   parentTransform - the plane set's owner's transform
// See also:
// --------------------------------------------------------------------------------
void EvePlaneSet::UpdateViewDistanceInfo( const TriFrustum& frustum, ViewDistanceInfo& viewDistance, const Matrix& parentTransform ) const
{
	for( auto it = m_cachedTransforms.begin(); it != m_cachedTransforms.end(); it++ )
	{
		// Each 'plane' is a square unit quad centered at the origin.
		Vector4 bs( 0.f, 0.0f, 0.0f, 0.7072f );
		BoundingSphereTransform( *it, bs );
		BoundingSphereTransform( parentTransform, bs );
		viewDistance.UpdateClipPlanes( bs, frustum );
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


