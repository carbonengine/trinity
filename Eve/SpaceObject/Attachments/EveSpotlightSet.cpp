////////////////////////////////////////////////////////////
//
//    Created:   July 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "EveSpotlightSet.h"
#include "TriRenderBatch.h"
#include "Tr2Effect.h"
#include "Utilities/BoundingSphere.h"

using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------
// Description:
//   A struct representing the vertex data used by the spotlight set.
// --------------------------------------------------------------------------------
struct SpotVertex
{
	Color m_color;
	Vector4 m_transform1;
	Vector4 m_transform2;
	Vector4 m_transform3;
	Vector3 m_scale;
	uint8_t m_index;
	uint8_t m_boneIndex;
	uint8_t m_boosterGainInfluence;

	uint8_t m_padding;
};

// --------------------------------------------------------------------------------
// Description:
//   Custom batch that can be used to submit geometry from the EveSpotlightSet
//   using a different Submit Geometry method.
// --------------------------------------------------------------------------------
class EveSpotlightSpriteBatch : public TriRenderBatch
{
public:
	void SetGeometryProvider( EveSpotlightSet* val )
	{
		m_geom = val;
	}

	void SubmitGeometry( Tr2RenderContext& renderContext )
	{
		m_geom->SubmitSpriteGeometry( renderContext );
	}

private:
	EveSpotlightSetPtr m_geom;
};

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members, set everything to invalid/empty
// --------------------------------------------------------------------------------
EveSpotlightSet::EveSpotlightSet( IRoot* lockobj ) :
	m_display( true ),
	PARENTLOCK( m_spotlightItems ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
{
}

// --------------------------------------------------------------------------------
// Description:
//   Cleanup
// --------------------------------------------------------------------------------
EveSpotlightSet::~EveSpotlightSet()
{
}

// --------------------------------------------------------------------------------
// Description:
//   Adds batches to be rendered to the accumulator.
// --------------------------------------------------------------------------------
void EveSpotlightSet::GetBatches( ITriRenderBatchAccumulator* accumulator, const Tr2PerObjectData* perObjectData )
{
	if ( m_display )
	{
		if( m_coneEffect && m_coneVertexBuffer.IsValid() )
		{
			if( m_vertexDeclHandle != Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
			{
				TriForwardingBatch* batch = accumulator->Allocate<TriForwardingBatch>();
				if( batch )
				{
					batch->SetPerObjectData( perObjectData );
					batch->SetShaderMaterial( m_coneEffect );
					batch->SetGeometryProvider( this );
					accumulator->Commit( batch );
				}
			}
		}

		if( m_glowEffect && m_spriteVertexBuffer.IsValid() )
		{
			if( m_vertexDeclHandle != Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
			{
				EveSpotlightSpriteBatch* spriteBatch = accumulator->Allocate<EveSpotlightSpriteBatch>();
				if( spriteBatch )
				{
					spriteBatch->SetPerObjectData( perObjectData );
					spriteBatch->SetShaderMaterial( m_glowEffect );
					spriteBatch->SetGeometryProvider( this );
					accumulator->Commit( spriteBatch );
				}
			}
		}
	}
}


// --------------------------------------------------------------------------------
// Description:
//   Initializes the device resources.
// --------------------------------------------------------------------------------
bool EveSpotlightSet::Initialize()
{
	PrepareResources();
	return true;
}


// --------------------------------------------------------------------------------
// Description:
//   Draws the geometry
// Arguments:
//   The render context
// --------------------------------------------------------------------------------
void EveSpotlightSet::SubmitGeometry( Tr2RenderContext& renderContext )
{
	renderContext.m_esm.ApplyStreamSource( 0, m_coneVertexBuffer, 0, sizeof( SpotVertex ) );
	renderContext.m_esm.ApplyVertexDeclaration( m_vertexDeclHandle );

	auto ib = Tr2Renderer::GetQuadListIndexBuffer( m_coneVertexCount / 4 );
	if( !ib )
	{
		return;
	}
	renderContext.m_esm.ApplyIndexBuffer( *ib );

	renderContext.SetTopology( TOP_TRIANGLES );
	renderContext.DrawIndexedPrimitive( m_coneVertexCount, 0, m_coneVertexCount / 2 );
}


// --------------------------------------------------------------------------------
// Description:
//   Draws sprite geometry. Called from the special sprinte batch.
// Arguments:
//   The render context
// --------------------------------------------------------------------------------
void EveSpotlightSet::SubmitSpriteGeometry( Tr2RenderContext& renderContext )
{
	renderContext.m_esm.ApplyStreamSource( 0, m_spriteVertexBuffer, 0, sizeof( SpotVertex ) );
	renderContext.m_esm.ApplyVertexDeclaration( m_vertexDeclHandle );

	auto ib = Tr2Renderer::GetQuadListIndexBuffer( m_spriteVertexCount / 4 );
	if( !ib )
	{
		return;
	}
	renderContext.m_esm.ApplyIndexBuffer( *ib );

	renderContext.SetTopology( TOP_TRIANGLES );
	renderContext.DrawIndexedPrimitive( m_spriteVertexCount, 0, m_spriteVertexCount / 2 );
}


// --------------------------------------------------------------------------------
// Description:
//   Makes sure all resources are released and all vertex buffers are destroyed.
// --------------------------------------------------------------------------------
void EveSpotlightSet::ReleaseResources( TriStorage s )
{
	m_coneVertexBuffer.Destroy();
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_spriteVertexBuffer.Destroy();
}

// --------------------------------------------------------------------------------
// Description:
//   Called when resourced need to be prepared. For example after device reset.
// Return value:
//   Boolean confirming the success of the method.
// --------------------------------------------------------------------------------
bool EveSpotlightSet::OnPrepareResources()
{
	if( m_coneVertexBuffer.IsValid() )
	{
		return true;
	}

	if( m_spotlightItems.empty() )
	{
		return true;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();

	// register vertex declaration. Same one is used for the cone and the sprites.
	static Tr2VertexDefinition s_spotVertexDecl;
	if( s_spotVertexDecl.empty() )
	{
		Tr2VertexDefinition& vd = s_spotVertexDecl;
		vd.Add( vd.FLOAT32_4, vd.COLOR );       // color
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 0 ); // 3x4 transform
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 1 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 2 );
		vd.Add( vd.FLOAT32_3, vd.TEXCOORD, 3 ); // scales
		vd.Add( vd.UBYTE_4  , vd.TEXCOORD, 4 ); // index
	}

	m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( s_spotVertexDecl );
	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return false;
	}

	// Number of vertices per quad
	const int vertCount = 4;
	// Get the number of SpotlightSetItems
	int itemCount = (unsigned int)m_spotlightItems.GetSize();
	// Cone uses 4 planes = 4 quads
	const int coneQuadCount = 4;
	m_coneVertexCount = itemCount * coneQuadCount * vertCount;
	// The 2 glow sprites use 2 quads
	const int spriteQuadCount = 2;
	m_spriteVertexCount =  itemCount * spriteQuadCount * vertCount;
	// Both vertex buffers use the same vertex declaration
	unsigned int bytesPerVertex = sizeof( SpotVertex );

	SpotVertex* coneVerts = reinterpret_cast<SpotVertex*>( CCP_ALIGNED_MALLOC( "coneVerts", m_coneVertexCount * bytesPerVertex, 16 ) );
	ON_BLOCK_EXIT( [&] { CCP_ALIGNED_FREE( coneVerts ); } );

	SpotVertex* spriteVerts = reinterpret_cast<SpotVertex*>( CCP_ALIGNED_MALLOC( "spriteVerts", m_spriteVertexCount * bytesPerVertex, 16 ) );
	ON_BLOCK_EXIT( [&] { CCP_ALIGNED_FREE( spriteVerts ); } );

	int n = (unsigned int)m_spotlightItems.GetSize();
	for( int i = 0; i < n; ++i )
	{
		for( int q = 0; q < coneQuadCount; ++q )
		{
			for ( int v = 0; v < vertCount; ++v )
			{
				SpotVertex& vertex = coneVerts[i * coneQuadCount * vertCount + vertCount * q + v];
				vertex.m_color = m_spotlightItems[i]->m_coneColor;
				vertex.m_transform1 = Vector4(
					m_spotlightItems[i]->m_transform._11,
					m_spotlightItems[i]->m_transform._21,
					m_spotlightItems[i]->m_transform._31,
					m_spotlightItems[i]->m_transform._41 );
				vertex.m_transform2 = Vector4(
					m_spotlightItems[i]->m_transform._12,
					m_spotlightItems[i]->m_transform._22,
					m_spotlightItems[i]->m_transform._32,
					m_spotlightItems[i]->m_transform._42 );
				vertex.m_transform3 = Vector4(
					m_spotlightItems[i]->m_transform._13,
					m_spotlightItems[i]->m_transform._23,
					m_spotlightItems[i]->m_transform._33,
					m_spotlightItems[i]->m_transform._43 );
				vertex.m_scale = Vector3( 1.0, 1.0, 1.0 ); // unused by the cone shader
				vertex.m_boneIndex = m_spotlightItems[i]->m_boneIndex;
				vertex.m_boosterGainInfluence = m_spotlightItems[i]->m_boosterGainInfluence ? 255 : 0;
			}
			coneVerts[i * coneQuadCount * vertCount + vertCount * q + 0].m_index = (uint8_t)( q * vertCount + 1 );
			coneVerts[i * coneQuadCount * vertCount + vertCount * q + 1].m_index = (uint8_t)( q * vertCount + 0 );
			coneVerts[i * coneQuadCount * vertCount + vertCount * q + 2].m_index = (uint8_t)( q * vertCount + 2 );
			coneVerts[i * coneQuadCount * vertCount + vertCount * q + 3].m_index = (uint8_t)( q * vertCount + 3 );
		}

		for( int k = 0; k < spriteQuadCount; ++k )
		{
			for( int v = 0; v < vertCount; ++v )
			{
				SpotVertex& spriteVertex = spriteVerts[i * spriteQuadCount * vertCount + vertCount * k + v];
				if( k % 2 == 0 )
				{   // the uniformly scaled glow sprite
					spriteVertex.m_color = m_spotlightItems[i]->m_spriteColor;
					spriteVertex.m_scale = Vector3(
						m_spotlightItems[i]->m_spriteScale.x,
						1.0,
						1.0);
				}
				else
				{   // the non-uniformly scaled flare sprite
					spriteVertex.m_color = m_spotlightItems[i]->m_flareColor;
					spriteVertex.m_scale = Vector3(
						1.0,
						m_spotlightItems[i]->m_spriteScale.y,
						m_spotlightItems[i]->m_spriteScale.z);
				}
				spriteVertex.m_boneIndex = m_spotlightItems[i]->m_boneIndex;
				spriteVertex.m_boosterGainInfluence = m_spotlightItems[i]->m_boosterGainInfluence ? 255 : 0;
				spriteVertex.m_transform1 = Vector4(
					m_spotlightItems[i]->m_transform._11,
					m_spotlightItems[i]->m_transform._21,
					m_spotlightItems[i]->m_transform._31,
					m_spotlightItems[i]->m_transform._41 );
				spriteVertex.m_transform2 = Vector4(
					m_spotlightItems[i]->m_transform._12,
					m_spotlightItems[i]->m_transform._22,
					m_spotlightItems[i]->m_transform._32,
					m_spotlightItems[i]->m_transform._42 );
				spriteVertex.m_transform3 = Vector4(
					m_spotlightItems[i]->m_transform._13,
					m_spotlightItems[i]->m_transform._23,
					m_spotlightItems[i]->m_transform._33,
					m_spotlightItems[i]->m_transform._43 );
			}
			spriteVerts[i * spriteQuadCount * vertCount + vertCount * k + 0].m_index = (uint8_t)( k * vertCount + 1 );
			spriteVerts[i * spriteQuadCount * vertCount + vertCount * k + 1].m_index = (uint8_t)( k * vertCount + 0 );
			spriteVerts[i * spriteQuadCount * vertCount + vertCount * k + 2].m_index = (uint8_t)( k * vertCount + 2 );
			spriteVerts[i * spriteQuadCount * vertCount + vertCount * k + 3].m_index = (uint8_t)( k * vertCount + 3 );
		}
	}


	// create cone VB
	CR_RETURN_VAL( m_coneVertexBuffer.Create( m_coneVertexCount * bytesPerVertex, USAGE_IMMUTABLE, coneVerts, renderContext ), false );
	CR_RETURN_VAL( m_spriteVertexBuffer.Create( m_spriteVertexCount * bytesPerVertex, USAGE_IMMUTABLE, spriteVerts, renderContext ), false );

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Rebuild resources after adding/removing/changing individual sprites
// --------------------------------------------------------------------------------
void EveSpotlightSet::Rebuild()
{
	ReleaseResources( 0 );
	PrepareResources();
}

// --------------------------------------------------------------------------------
// Description:
//   Update view distance info based on an estimate of the spotlight set's cones.
//   Cone size is mostly determined by the shader used, if that changes this
//   estimation may become invalid.
// Arguments:
//   frustum - the frustum
//   viewDistance - the ViewDistanceInfo stuct that we want to update
//   parentTransform - the spotlight set's owner's transform
// --------------------------------------------------------------------------------
void EveSpotlightSet::UpdateViewDistanceInfo( const TriFrustum& frustum, ViewDistanceInfo& viewDistance, const Matrix& parentTransform ) const
{
	for( auto it = m_spotlightItems.begin(); it != m_spotlightItems.end(); it++ )
	{
		// We only consider the cones. The base size is a quad who's origin is at 0, 0, 0
		// size 2x1 and lies in the positive z direction(see SpotlightCone.fx if you need to
		// figure this out).
		Vector4 bs( 0.f, 0.0f, 0.5f, 1.1191f );
		BoundingSphereTransform( (*it)->m_transform, bs );
		BoundingSphereTransform( parentTransform, bs );
		viewDistance.UpdateClipPlanes( bs, frustum );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Get shader for the spotlight's cone
// Return value:
//   The cone effect.
// --------------------------------------------------------------------------------
Tr2EffectPtr EveSpotlightSet::GetConeEffect() const
{
	return m_coneEffect;
}

// --------------------------------------------------------------------------------
// Description:
//   Set this spotlight's cone shader
// Arguments:
//   the new cone effect
// --------------------------------------------------------------------------------
void EveSpotlightSet::SetConeEffect( Tr2EffectPtr effect )
{
	m_coneEffect = effect;
}

// --------------------------------------------------------------------------------
// Description:
//   Get shader for the spotlight's lensflare glow
// Return value:
//   The glow effect.
// --------------------------------------------------------------------------------
Tr2EffectPtr EveSpotlightSet::GetGlowEffect() const
{
	return m_glowEffect;
}

// --------------------------------------------------------------------------------
// Description:
//   Set this spotlight's lensflare glow shader
// Arguments:
//   the new glow effect
// --------------------------------------------------------------------------------
void EveSpotlightSet::SetGlowEffect( Tr2EffectPtr effect )
{
	m_glowEffect = effect;
}

// --------------------------------------------------------------------------------
// Description:
//   Get name of this thing
// Return value:
//   The name.
// --------------------------------------------------------------------------------
const char* EveSpotlightSet::GetName() const
{
	return m_name.c_str();
}

// --------------------------------------------------------------------------------
// Description:
//   Set this things's name
// Arguments:
//   the name
// --------------------------------------------------------------------------------
void EveSpotlightSet::SetName( const char* name )
{
	m_name = name;
}

// --------------------------------------------------------------------------------
// Description:
//   Return a pointer to the complete list of items
// Arguments:
//   the list
// --------------------------------------------------------------------------------
const EveSpotlightSetItemVector* EveSpotlightSet::GetSpotlightItems() const
{
	return &m_spotlightItems;
}

// --------------------------------------------------------------------------------
// Description:
//   Add a new spotlight (aka item) to this set
// Arguments:
//   the new spotlight
// --------------------------------------------------------------------------------
void EveSpotlightSet::AddSpotlightItem( EveSpotlightSetItemPtr item )
{
	m_spotlightItems.Insert( -1, item );
}












