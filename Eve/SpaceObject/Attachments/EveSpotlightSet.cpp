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
#include "Utilities/MatrixUtils.h"
#include "Tr2QuadRenderer.h"
#include "Tr2PickingHelperBatch.h"

using namespace Tr2RenderContextEnum;

const Tr2VertexDefinition& EveSpotlightSet::GlowPoolVertex::GetDefinition()
{
	static Tr2VertexDefinition s_definition;
	if( s_definition.empty() )
	{
		Tr2VertexDefinition& vd = s_definition;
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 4 );

		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 0, 1, 1 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 1, 1, 1 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 2, 1, 1 );
		vd.Add( vd.FLOAT16_4 , vd.COLOR, 0, 1, 1 );
		vd.Add( vd.FLOAT16_4 , vd.COLOR, 1, 1, 1 );
		vd.Add( vd.FLOAT16_4, vd.TEXCOORD, 3, 1, 1 );
	}
	return s_definition;
}

const Tr2VertexDefinition& EveSpotlightSet::ConePoolVertex::GetDefinition()
{
	static Tr2VertexDefinition s_definition;
	if( s_definition.empty() )
	{
		Tr2VertexDefinition& vd = s_definition;
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 4 );

		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 0, 1, 1 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 1, 1, 1 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 2, 1, 1 );
		vd.Add( vd.FLOAT16_4 , vd.COLOR, 0, 1, 1 );
		vd.Add( vd.FLOAT16_2, vd.TEXCOORD, 3, 1, 1 );
	}
	return s_definition;
}

namespace
{

const int CONE_QUAD_COUNT = 4;
const int SPRITE_QUAD_COUNT = 2;

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

}

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
	m_useQuadRenderer( false ),
	m_skinned( false ),
	PARENTLOCK( m_spotlightItems ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_coneEffectHash( 0 ),
	m_glowEffectHash( 0 ),
	m_coneBuffer( "EveSpotlightSet::m_coneBuffer" ),
	m_glowBuffer( "EveSpotlightSet::m_glowBuffer" ),
	m_spotlightData( "EveSpotlightSet::m_spotlightData" )
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
//   Switches spotlight set to use shared quad renderer.
// Arguments:
//   useQuadRenderer - if the quad renderer should be used
//   skinned - if the spotlight set should use skinning (only matters for quad rendering)
// --------------------------------------------------------------------------------
void EveSpotlightSet::UseQuadRenderer( bool useQuadRenderer, bool skinned )
{
	if( useQuadRenderer == m_useQuadRenderer )
	{
		return;
	}
	if( useQuadRenderer && ( !m_coneEffect || !m_glowEffect ) )
	{
		CCP_ASSERT_M( false, "effect must be initialized before using quad renderer" );
		useQuadRenderer = false;
	}
	m_useQuadRenderer = useQuadRenderer;
	m_skinned = skinned;
	if( m_useQuadRenderer )
	{
		m_coneEffectHash = m_coneEffect->GetHashValue();
		m_glowEffectHash = m_glowEffect->GetHashValue();
		ReleaseResources( TRISTORAGE_ALL );
	}
	PrepareResources();
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
	if( m_useQuadRenderer )
	{
		int n = (unsigned int)m_spotlightItems.GetSize();
		m_coneBuffer.resize( n );
		m_glowBuffer.resize( n );
		m_spotlightData.resize( n );

		for( int i = 0; i < n; ++i )
		{
			m_spotlightData[i].transform = m_spotlightItems[i]->m_transform;
			m_spotlightData[i].boneIndex = m_spotlightItems[i]->m_boneIndex;
			m_spotlightData[i].boosterGainInfluence = m_spotlightItems[i]->m_boosterGainInfluence;
		}

		for( int i = 0; i < n; ++i )
		{
			auto& vertex = m_coneBuffer[i];
			vertex.m_color[0] = m_spotlightItems[i]->m_coneColor.r;
			vertex.m_color[1] = m_spotlightItems[i]->m_coneColor.g;
			vertex.m_color[2] = m_spotlightItems[i]->m_coneColor.b;
		}


		for( int i = 0; i < n; ++i )
		{
			auto& vertex = m_glowBuffer[i];
			vertex.m_spriteColor[0] = m_spotlightItems[i]->m_spriteColor.r;
			vertex.m_spriteColor[1] = m_spotlightItems[i]->m_spriteColor.g;
			vertex.m_spriteColor[2] = m_spotlightItems[i]->m_spriteColor.b;
			vertex.m_flareColor[0] = m_spotlightItems[i]->m_flareColor.r;
			vertex.m_flareColor[1] = m_spotlightItems[i]->m_flareColor.g;
			vertex.m_flareColor[2] = m_spotlightItems[i]->m_flareColor.b;
			vertex.m_scale[0] = m_spotlightItems[i]->m_spriteScale.x;
			vertex.m_scale[1] = m_spotlightItems[i]->m_spriteScale.y;
			vertex.m_scale[2] = m_spotlightItems[i]->m_spriteScale.z;
		}
		return true;
	}

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
	m_coneVertexCount = itemCount * CONE_QUAD_COUNT * vertCount;
	m_spriteVertexCount =  itemCount * SPRITE_QUAD_COUNT * vertCount;
	// Both vertex buffers use the same vertex declaration
	unsigned int bytesPerVertex = sizeof( SpotVertex );

	SpotVertex* coneVerts = reinterpret_cast<SpotVertex*>( CCP_ALIGNED_MALLOC( "coneVerts", m_coneVertexCount * bytesPerVertex, 16 ) );
	ON_BLOCK_EXIT( [&] { CCP_ALIGNED_FREE( coneVerts ); } );

	SpotVertex* spriteVerts = reinterpret_cast<SpotVertex*>( CCP_ALIGNED_MALLOC( "spriteVerts", m_spriteVertexCount * bytesPerVertex, 16 ) );
	ON_BLOCK_EXIT( [&] { CCP_ALIGNED_FREE( spriteVerts ); } );

	int n = (unsigned int)m_spotlightItems.GetSize();
	for( int i = 0; i < n; ++i )
	{
		for( int q = 0; q < CONE_QUAD_COUNT; ++q )
		{
			for ( int v = 0; v < vertCount; ++v )
			{
				SpotVertex& vertex = coneVerts[i * CONE_QUAD_COUNT * vertCount + vertCount * q + v];
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
			coneVerts[i * CONE_QUAD_COUNT * vertCount + vertCount * q + 0].m_index = (uint8_t)( q * vertCount + 1 );
			coneVerts[i * CONE_QUAD_COUNT * vertCount + vertCount * q + 1].m_index = (uint8_t)( q * vertCount + 0 );
			coneVerts[i * CONE_QUAD_COUNT * vertCount + vertCount * q + 2].m_index = (uint8_t)( q * vertCount + 2 );
			coneVerts[i * CONE_QUAD_COUNT * vertCount + vertCount * q + 3].m_index = (uint8_t)( q * vertCount + 3 );
		}

		for( int k = 0; k < SPRITE_QUAD_COUNT; ++k )
		{
			for( int v = 0; v < vertCount; ++v )
			{
				SpotVertex& spriteVertex = spriteVerts[i * SPRITE_QUAD_COUNT * vertCount + vertCount * k + v];
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
			spriteVerts[i * SPRITE_QUAD_COUNT * vertCount + vertCount * k + 0].m_index = (uint8_t)( k * vertCount + 1 );
			spriteVerts[i * SPRITE_QUAD_COUNT * vertCount + vertCount * k + 1].m_index = (uint8_t)( k * vertCount + 0 );
			spriteVerts[i * SPRITE_QUAD_COUNT * vertCount + vertCount * k + 2].m_index = (uint8_t)( k * vertCount + 2 );
			spriteVerts[i * SPRITE_QUAD_COUNT * vertCount + vertCount * k + 3].m_index = (uint8_t)( k * vertCount + 3 );
		}
	}


	// create cone VB
	CR_RETURN_VAL( m_coneVertexBuffer.Create( m_coneVertexCount * bytesPerVertex, USAGE_IMMUTABLE, coneVerts, renderContext ), false );
	CR_RETURN_VAL( m_spriteVertexBuffer.Create( m_spriteVertexCount * bytesPerVertex, USAGE_IMMUTABLE, spriteVerts, renderContext ), false );

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Registers set effects with quad renderer if quad rendering was enabled with 
//   UseQuadRenderer call.
// Arguments:
//   quadRenderer - quad renderer
// --------------------------------------------------------------------------------
void EveSpotlightSet::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( m_useQuadRenderer )
	{
		quadRenderer.RegisterEffect( m_coneEffectHash, sizeof( ConePoolVertex ), CONE_QUAD_COUNT, ConePoolVertex::GetDefinition(), m_coneEffect );
		quadRenderer.RegisterEffect( m_glowEffectHash, sizeof( GlowPoolVertex ), SPRITE_QUAD_COUNT, GlowPoolVertex::GetDefinition(), m_glowEffect );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Adds sprites to render with quad renderer if quad rendering was enabled with 
//   UseQuadRenderer call.
// Arguments:
//   quadRenderer - quad renderer
//   world - parent local to world transform
//   activation - parent "activation" state
//   boosterGain - parent booster intensity
//   bones - array of bone transforms
//   boneCount - number of bones
// --------------------------------------------------------------------------------
void EveSpotlightSet::AddToQuadRenderer( Tr2QuadRenderer& quadRenderer, const Matrix& world, float activation, float boosterGain, const granny_matrix_3x4* bones, size_t boneCount )
{
	if( !m_useQuadRenderer || !m_display || m_glowBuffer.empty() )
	{
		return;
	}
	Matrix m = Tr2Renderer::GetIdentityTransform();
	if( !m_skinned )
	{
		for( size_t i = 0; i < m_spotlightData.size(); ++i )
		{
			m = XMMatrixMultiply( m_spotlightData[i].transform, world );
			auto& glow = m_glowBuffer[i];
			glow.m_transform1 = Vector4( m._11, m._21, m._31, m._41 );
			glow.m_transform2 = Vector4( m._12, m._22, m._32, m._42 );
			glow.m_transform3 = Vector4( m._13, m._23, m._33, m._43 );
			glow.m_activation = activation;
			glow.m_boosterGainInfluence = 1 + ( boosterGain - 1 ) * m_spotlightData[i].boosterGainInfluence;

			auto& cone = m_coneBuffer[i];
			cone.m_transform1 = Vector4( m._11, m._21, m._31, m._41 );
			cone.m_transform2 = Vector4( m._12, m._22, m._32, m._42 );
			cone.m_transform3 = Vector4( m._13, m._23, m._33, m._43 );
			cone.m_activation = activation;
			cone.m_boosterGainInfluence = glow.m_boosterGainInfluence;
		}
	}
	else
	{
		for( size_t i = 0; i < m_spotlightData.size(); ++i )
		{
			auto& data = m_spotlightData[i];
			if( data.boneIndex < boneCount )
			{
				TriMatrixCopyFrom3x4( &m, &bones[data.boneIndex] );
				m = XMMatrixMultiply( XMMatrixMultiply( data.transform, m ), world );
			}
			else
			{
				m = XMMatrixMultiply( data.transform, world );
			}

			auto& glow = m_glowBuffer[i];
			glow.m_transform1 = Vector4( m._11, m._21, m._31, m._41 );
			glow.m_transform2 = Vector4( m._12, m._22, m._32, m._42 );
			glow.m_transform3 = Vector4( m._13, m._23, m._33, m._43 );
			glow.m_activation = activation;
			glow.m_boosterGainInfluence = 1 + ( boosterGain - 1 ) * data.boosterGainInfluence;

			auto& cone = m_coneBuffer[i];
			cone.m_transform1 = Vector4( m._11, m._21, m._31, m._41 );
			cone.m_transform2 = Vector4( m._12, m._22, m._32, m._42 );
			cone.m_transform3 = Vector4( m._13, m._23, m._33, m._43 );
			cone.m_activation = activation;
			cone.m_boosterGainInfluence = glow.m_boosterGainInfluence;
		}
	}
	quadRenderer.AddQuads( m_glowEffectHash, &m_glowBuffer[0], m_glowBuffer.size() );
	quadRenderer.AddQuads( m_coneEffectHash, &m_coneBuffer[0], m_coneBuffer.size() );
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

void EveSpotlightSet::GetPickingBatches( ITriRenderBatchAccumulator* batches, uint16_t& areaIDOffset, const Tr2PerObjectData* perObjectData )
{
	for( auto it = m_spotlightItems.begin(); it != m_spotlightItems.end(); ++it )
	{
		if( auto batch = batches->Allocate<Tr2PickingHelperBatch>() )
		{
			batch->SetPerObjectData( perObjectData );
			batch->AddSphere(  
				( *it )->m_transform.GetTranslation(),
				std::max( float( ( *it )->m_spriteScale.x ), std::max( float( ( *it )->m_spriteScale.y ), float( ( *it )->m_spriteScale.z ) ) ) * 0.5f );
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