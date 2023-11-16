////////////////////////////////////////////////////////////
//
//    Created:   July 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "EveSpotlightSet.h"
#include "TriRenderBatch.h"
#include "Shader/Tr2Effect.h"
#include "Utilities/BoundingSphere.h"
#include "Utilities/MatrixUtils.h"
#include "Tr2QuadRenderer.h"
#include "Tr2PickingHelperBatch.h"
#include "Tr2DebugRenderer.h"

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

}

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members, set everything to invalid/empty
// --------------------------------------------------------------------------------
EveSpotlightSet::EveSpotlightSet( IRoot* lockobj ) :
	m_display( true ),
	m_skinned( false ),
	m_intensity( 1.0f ),
	PARENTLOCK( m_spotlightItems ),
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
//   Initializes the device resources.
// --------------------------------------------------------------------------------
bool EveSpotlightSet::Initialize()
{
	if( m_coneEffect )
	{
		m_coneEffectHash = m_coneEffect->GetHashValue();
	}
	if( m_glowEffect )
	{
		m_glowEffectHash = m_glowEffect->GetHashValue();
	}
	Rebuild();
	return true;
}

inline void EveSpotlightSet::RegisterQuadRendererCone( Tr2QuadRenderer& quadRenderer )
{
	quadRenderer.RegisterEffect( m_coneEffectHash, TRIBATCHTYPE_ADDITIVE, sizeof( ConePoolVertex ), CONE_QUAD_COUNT, ConePoolVertex::GetDefinition(), m_coneEffect );
}

inline void EveSpotlightSet::RegisterQuadRendererGlow( Tr2QuadRenderer& quadRenderer )
{
	quadRenderer.RegisterEffect( m_glowEffectHash, TRIBATCHTYPE_ADDITIVE, sizeof( GlowPoolVertex ), SPRITE_QUAD_COUNT, GlowPoolVertex::GetDefinition(), m_glowEffect );
}

// --------------------------------------------------------------------------------------
// Description:
//   Get bounding box around spot lights, update visibility based on if box is visible or not
// --------------------------------------------------------------------------------------
bool EveSpotlightSet::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount )
{
	auto aabb = GetItemSetAabb( m_aabb, m_boundingBoxes, bones, m_skinned ? boneCount : 0 );
	if( !aabb.IsInitialized() )
	{
		return false;
	}
	aabb.Transform( parentTransform );

	return frustum.IsBoxVisible( aabb.m_min, aabb.m_max );
}

// --------------------------------------------------------------------------------------
// Description:
//   Get bounding box surrounding spot lights
// --------------------------------------------------------------------------------------
AxisAlignedBoundingBox EveSpotlightSet::GetAabb( const granny_matrix_3x4* bones, size_t boneCount ) const
{
	return GetItemSetAabb( m_aabb, m_boundingBoxes, bones, m_skinned ? boneCount : 0 );
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
	RegisterQuadRendererCone( quadRenderer );
	RegisterQuadRendererGlow( quadRenderer );
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
	if( !m_display || m_glowBuffer.empty() )
	{
		return;
	}

	activation *= m_intensity;

	Matrix m = IdentityMatrix();
	if( !m_skinned )
	{
		for( size_t i = 0; i < m_spotlightData.size(); ++i )
		{
			m = XMMatrixMultiply( m_spotlightData[i].transform, world );
			auto& glow = m_glowBuffer[i];
			glow.m_transform1 = Vector4( m._11, m._21, m._31, m._41 );
			glow.m_transform2 = Vector4( m._12, m._22, m._32, m._42 );
			glow.m_transform3 = Vector4( m._13, m._23, m._33, m._43 );
			glow.m_activation = Float_16( XMConvertFloatToHalf( activation ) );
			glow.m_boosterGainInfluence = Float_16( XMConvertFloatToHalf( 1 + ( boosterGain - 1 ) * m_spotlightData[i].boosterGainInfluence ) );

			auto& cone = m_coneBuffer[i];
			cone.m_transform1 = Vector4( m._11, m._21, m._31, m._41 );
			cone.m_transform2 = Vector4( m._12, m._22, m._32, m._42 );
			cone.m_transform3 = Vector4( m._13, m._23, m._33, m._43 );
			cone.m_activation = Float_16( XMConvertFloatToHalf( activation ) );
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
			glow.m_activation = Float_16( XMConvertFloatToHalf( activation ) );
			glow.m_boosterGainInfluence = Float_16( XMConvertFloatToHalf( 1 + ( boosterGain - 1 ) * data.boosterGainInfluence ) );

			auto& cone = m_coneBuffer[i];
			cone.m_transform1 = Vector4( m._11, m._21, m._31, m._41 );
			cone.m_transform2 = Vector4( m._12, m._22, m._32, m._42 );
			cone.m_transform3 = Vector4( m._13, m._23, m._33, m._43 );
			cone.m_activation = Float_16( XMConvertFloatToHalf( activation ) );
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
		vertex.m_color[0] = Float_16( m_spotlightItems[i]->m_coneColor.r );
		vertex.m_color[1] = Float_16( m_spotlightItems[i]->m_coneColor.g );
		vertex.m_color[2] = Float_16( m_spotlightItems[i]->m_coneColor.b );
	}


	for( int i = 0; i < n; ++i )
	{
		auto& vertex = m_glowBuffer[i];
		vertex.m_spriteColor[0] = Float_16( m_spotlightItems[i]->m_spriteColor.r );
		vertex.m_spriteColor[1] = Float_16( m_spotlightItems[i]->m_spriteColor.g );
		vertex.m_spriteColor[2] = Float_16( m_spotlightItems[i]->m_spriteColor.b );
		vertex.m_flareColor[0] = Float_16( m_spotlightItems[i]->m_flareColor.r );
		vertex.m_flareColor[1] = Float_16( m_spotlightItems[i]->m_flareColor.g );
		vertex.m_flareColor[2] = Float_16( m_spotlightItems[i]->m_flareColor.b );
		vertex.m_scale[0] = Float_16( m_spotlightItems[i]->m_spriteScale.x );
		vertex.m_scale[1] = Float_16( m_spotlightItems[i]->m_spriteScale.y );
		vertex.m_scale[2] = Float_16( m_spotlightItems[i]->m_spriteScale.z );
	}

	CreateItemSetBoundingBoxes( m_aabb, m_boundingBoxes, m_skinned, begin( m_spotlightItems ), end( m_spotlightItems ) );
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
	if( m_coneEffect )
	{
		m_coneEffectHash = m_coneEffect->GetHashValue();
	}
	else
	{
		m_coneEffectHash = 0;
	}
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
	if( m_glowEffect )
	{
		m_glowEffectHash = m_glowEffect->GetHashValue();
	}
	else
	{
		m_glowEffectHash = 0;
	}
}

void EveSpotlightSet::SetSkinned( bool skinned )
{
	m_skinned = skinned;
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

void EveSpotlightSet::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	if( nullptr != m_coneEffect )
	{
		m_coneEffect->SetOption( name, value );
		m_coneEffectHash = m_coneEffect->GetHashValue();
		RegisterQuadRendererCone( *Tr2QuadRenderer::Instance() );
	}

	if( nullptr != m_glowEffect )
	{
		m_glowEffect->SetOption( name, value );
		m_glowEffectHash = m_glowEffect->GetHashValue();
		RegisterQuadRendererGlow( *Tr2QuadRenderer::Instance() );
	}
}

void EveSpotlightSet::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Spotlight Sets" );
	options.insert( "Spotlight Sets Bounds" );
}

void EveSpotlightSet::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount )
{
	if( renderer.HasOption( GetRawRoot(), "Spotlight Sets" ) )
	{
		for( auto it = m_spotlightItems.begin(); it != m_spotlightItems.end(); ++it )
		{
			auto& transform = ( *it )->m_transform;

			renderer.DrawCone(
				*it,
				Matrix( XMMatrixRotationX( -XM_PI / 2 ) ) * Matrix( XMMatrixTranslation( 0, 0, 0.5f ) ) * transform * parentTransform,
				0.5f,
				1.f,
				10,
				Tr2DebugRenderer::Wireframe,
				Tr2DebugColor( 0xffaaff00, 0x22aaff00 ) );

			renderer.DrawCone(
				*it,
				Matrix( XMMatrixRotationX( -XM_PI / 2 ) ) * Matrix( XMMatrixTranslation( 0, 0, 0.5f ) ) * transform * parentTransform,
				0.5f,
				1.f,
				10,
				Tr2DebugRenderer::Solid,
				0 );
		}
	}

	if( renderer.HasOption( GetRawRoot(), "Spotlight Sets Bounds" ) )
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