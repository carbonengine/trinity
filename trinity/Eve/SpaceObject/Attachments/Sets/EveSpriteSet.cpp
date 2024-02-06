#include "StdAfx.h"
#include "EveSpriteSet.h"
#include "Shader/Tr2Effect.h"
#include "TriRenderBatch.h"
#include "Utilities/MatrixUtils.h"
#include "Tr2QuadRenderer.h"
#include "Tr2DebugRenderer.h"
#include <Tr2Renderer.h>
#include "Resources/Tr2LightProfileRes.h"

CCP_STATS_DECLARED_ELSEWHERE( primitiveCount );

using namespace Tr2RenderContextEnum;

const Tr2VertexDefinition& EveSpriteSet::PoolVertex::GetDefinition()
{
	static Tr2VertexDefinition s_spriteVertexDecl;
	if( s_spriteVertexDecl.m_items.empty() )
	{
		Tr2VertexDefinition& vd = s_spriteVertexDecl;
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 5 );

		vd.Add( vd.FLOAT32_3, vd.POSITION, 0, 1, 1 );
		vd.Add( vd.FLOAT16_4, vd.TEXCOORD, 0, 1, 1 );
		vd.Add( vd.FLOAT16_2, vd.TEXCOORD, 1, 1, 1 );
		vd.Add( vd.UBYTE_4_NORM , vd.COLOR, 0, 1, 1 );
		vd.Add( vd.UBYTE_4_NORM , vd.COLOR, 1, 1, 1 );
	}
	return s_spriteVertexDecl;
}

EveSpriteLight::EveSpriteLight() :
	lightData( LightData() ),
	index( 0 ),
	blinkRate( 0 ),
	blinkPhase( 0 ),
	minScale( 0 ),
	maxScale( 0 ),
	boneMatrix( IdentityMatrix() )
{

}

EveSpriteLight::EveSpriteLight( const LightData& lightData, float blinkPhase, float blinkRate, float minScale, float maxScale, uint32_t index, const std::wstring profilePath ) :
	lightData( lightData ),
	index( index ),
	blinkRate( blinkRate ),
	blinkPhase( blinkPhase ),
	minScale( minScale ),
	maxScale( maxScale ),
	boneMatrix( IdentityMatrix() )
{
	if( !profilePath.empty() )
	{
		BeResMan->GetResource( profilePath, L"lp", lightProfile );
	}
}

namespace EveSpriteLightUtils {
	float Blink( float blinkRate, float blinkPhase, float minScale, float maxScale )
	{
		const float FLASH_PEAK_TIME = 0.05f;
		float intPart;
		float f = modf( Tr2Renderer::GetAnimationTime() * blinkRate + blinkPhase, &intPart );

		float peak = FLASH_PEAK_TIME * blinkRate;
		float result = 0.0f;
		float end = peak * 4.0f;

		auto lerp = [] ( float a, float b, float f ) {
			return a + f * ( b - a );
		};

		if( peak < 0.0001f )
		{
			peak = 1.0f;
		}
		if( f < peak )
		{
			result = lerp( 0.0f, 1.0f, f / peak );
		}
		else if( f < end )
		{
			result = lerp( 1.0f, 0.0f, ( f - peak ) / ( end - peak ) );
		}
		return ( maxScale - minScale ) * result + minScale;
	}
}


EveSpriteSet::EveSpriteSet( IRoot* lockobj ) :
	PARENTLOCK( m_sprites ),
	m_display( true ),
	m_skinned( false ),
	m_effectHash( 0 ),
	m_intensity( 1.f ),
	m_activationStrength( 0 ),
	m_buffer( "EveSpriteSet::m_buffer" ),
	m_spriteData( "EveSpriteSet::m_spriteData" )
{
}

EveSpriteSet::~EveSpriteSet()
{
}

bool EveSpriteSet::Initialize()
{
	Rebuild();
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Adds sprites to render as booster glow with quad renderer if quad rendering 
//   was enabled with UseQuadRenderer call.
// Arguments:
//   quadRenderer - quad renderer
//   world - parent local to world transform
//   boosterGain - parent booster intensity
// --------------------------------------------------------------------------------
void EveSpriteSet::AddBoosterGlowToQuadRenderer( Tr2QuadRenderer& quadRenderer, const Matrix& world, float boosterGain, float warpIntensity )
{
	if( !m_display || m_spriteData.empty() )
	{
		return;
	}
	Matrix m = IdentityMatrix();
	auto n = m_spriteData.size();

	XMVector3TransformCoordStream( reinterpret_cast
		<XMFLOAT3*>( &m_buffer[0].position ), 
		sizeof( PoolVertex ), 
		reinterpret_cast<XMFLOAT3*>( &m_spriteData[0].position ), 
		sizeof( SpriteData ), 
		uint32_t( n ), 
		world );

	Float_16 zDirX( XMConvertFloatToHalf( world.GetZ().x ) );
	Float_16 zDirY( XMConvertFloatToHalf( world.GetZ().y ) );
	Float_16 zDirZ( XMConvertFloatToHalf( world.GetZ().z ) );
	uint32_t gain = std::min( uint32_t( boosterGain * 255.f ), 255u ) << 24;
	uint32_t warp = std::min( uint32_t( warpIntensity * 255.f ), 255u ) << 24;

	for( size_t i = 0; i < n; ++i )
	{
		auto& vert = m_buffer[i];
		vert.activation = zDirX;
		vert.blinkRate = zDirY;
		vert.falloff = zDirZ;
		vert.color = ( vert.color & 0xffffff ) | gain;
		vert.warpColor = ( vert.warpColor & 0xffffff ) | warp;
	}
	quadRenderer.AddQuads( m_effectHash, &m_buffer[0], m_sprites.GetSize() );
}

// --------------------------------------------------------------------------------------
// Description:
//   Go through list of sprites, update visibility based on if a sprite is visible or not.
// --------------------------------------------------------------------------------------
bool EveSpriteSet::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount )
{
	auto aabb = GetAabb( bones, boneCount );
	if( !aabb.IsInitialized() )
	{
		return false;
	}
	aabb.Transform( parentTransform );

	return frustum.IsBoxVisible( aabb.m_min, aabb.m_max );
}

void EveSpriteSet::UpdateLights( const granny_matrix_3x4* bones, size_t boneCount, float activationStrength, float boosterGain )
{
	for( auto& light : m_lights )
	{
		if( light.lightData.boneIndex > 0 && light.lightData.boneIndex < boneCount )
		{
			TriMatrixCopyFrom3x4( &( light.boneMatrix ), &bones[light.lightData.boneIndex] );
		}
	}
	m_activationStrength = activationStrength;
}

AxisAlignedBoundingBox EveSpriteSet::GetAabb( const granny_matrix_3x4* bones, size_t boneCount ) const
{
	return GetItemSetAabb( m_aabb, m_boundingBoxes, bones, m_skinned ? boneCount : 0 );
}

void EveSpriteSet::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	quadRenderer.RegisterEffect( m_effectHash, TRIBATCHTYPE_ADDITIVE, sizeof( PoolVertex ), 1, PoolVertex::GetDefinition(), m_effect );
}

// --------------------------------------------------------------------------------
void EveSpriteSet::AddToQuadRenderer( Tr2QuadRenderer& quadRenderer, const Matrix& parentTransform, float activation, float boosterGain, const granny_matrix_3x4* bones, size_t boneCount )
{
	if( !m_display || m_spriteData.empty() )
	{
		return;
	}
	Matrix m = IdentityMatrix();
	auto n = m_spriteData.size();
	if( !m_skinned )
	{
		XMVector3TransformCoordStream( 
			reinterpret_cast<XMFLOAT3*>( &m_buffer[0].position ), 
			sizeof( PoolVertex ), 
			reinterpret_cast<XMFLOAT3*>( &m_spriteData[0] ), 
			sizeof( SpriteData ), 
			uint32_t( n ), 
			parentTransform );
	}
	else
	{
		for( size_t i = 0; i < n; ++i )
		{
			auto boneIndex = m_spriteData[i].boneIndex;
			if( boneIndex < boneCount )
			{
				TriMatrixCopyFrom3x4( &m, &bones[boneIndex] );
				XMVECTOR position = XMVector3TransformCoord( XMLoadFloat3( reinterpret_cast<XMFLOAT3*>( &m_spriteData[i] ) ), m );
				XMStoreFloat3( 
					reinterpret_cast<XMFLOAT3*>( &m_buffer[i].position ), 
					XMVector3TransformCoord( position, parentTransform ) );
			}
			else
			{
				XMStoreFloat3( 
					reinterpret_cast<XMFLOAT3*>( &m_buffer[i].position ), 
					XMVector3TransformCoord( XMLoadFloat3( reinterpret_cast<XMFLOAT3*>( &m_spriteData[i] ) ), parentTransform ) );
			}
		}
	}
	Float_16 activation16( XMConvertFloatToHalf( activation * m_intensity ) );
	for( size_t i = 0; i < n; ++i )
	{
		m_buffer[i].activation = activation16;
	}
	quadRenderer.AddQuads( m_effectHash, &m_buffer[0], m_sprites.GetSize() );
}

void EveSpriteSet::Clear()
{
	m_sprites.Remove(-1);
}

void EveSpriteSet::Add( 
	const Vector3& pos, 
	float blinkRate, 
	float blinkPhase, 
	float minScale, 
	float maxScale, 
	float falloff, 
	const Color& color, 
	const Color& warpColor )
{
	EveSpriteSetItemPtr item;
	item.CreateInstance();

	item->m_position = pos;
	item->m_blinkRate = blinkRate;
	item->m_blinkPhase = blinkPhase;
	item->m_minScale = minScale;
	item->m_maxScale = maxScale;
	item->m_falloff = falloff;
	item->m_color = color;
	item->m_warpColor = warpColor;
	item->m_boneIndex = 0;

	Add( item );
}

void EveSpriteSet::Add( const Vector3& pos, float scale, const Color& color, const Color& warpColor )
{
	Add( pos, 0.0f, 0.0f, scale, scale, 0.0f, color, warpColor );
}

void EveSpriteSet::Add( EveSpriteSetItemPtr newItem )
{
	m_sprites.Append( newItem );
}

EveSpriteSetItemVector* EveSpriteSet::GetSprites()
{
	return &m_sprites;
}

const char* EveSpriteSet::GetName()
{
	return m_name.c_str();
}

Tr2EffectPtr EveSpriteSet::GetEffect()
{
	return m_effect;
}

void EveSpriteSet::SetEffect( Tr2EffectPtr effect )
{
	m_effect = effect;
	if( m_effect )
	{
		m_effectHash = m_effect->GetHashValue();
	}
	else
	{
		m_effectHash = 0;
	}
}

void EveSpriteSet::SetSkinned( bool skinned )
{
	m_skinned = skinned;
}

void EveSpriteSet::SetName( const char* name )
{
	m_name = name;
}

// --------------------------------------------------------------------------------
// Description:
//   Rebuild resources after adding/removing/changing individual sprites
// --------------------------------------------------------------------------------
void EveSpriteSet::Rebuild()
{
	if( m_effect )
	{
		m_effectHash = m_effect->GetHashValue();
	}

	int n = (unsigned int)m_sprites.GetSize();
	m_buffer.resize( n );
	for( int i = 0; i < n; ++i )
	{
		auto sprite = m_sprites[i];
		auto& vertex = m_buffer[i];
		vertex.position = sprite->m_position;
		uint32_t color = sprite->m_color;
		vertex.color = 
			( ( color & 0xff0000 ) >> 16 ) |
			( color & 0xff00ff00 ) |
			( ( color & 0xff ) << 16 );
		color = sprite->m_warpColor;
		vertex.warpColor = 
			( ( color & 0xff0000 ) >> 16 ) |
			( color & 0xff00ff00 ) |
			( ( color & 0xff ) << 16 );
		vertex.blinkPhase = Float_16( sprite->m_blinkPhase );
		vertex.blinkRate = Float_16( sprite->m_blinkRate );
		vertex.minScale = Float_16( sprite->m_minScale );
		vertex.maxScale = Float_16( sprite->m_maxScale );
		vertex.falloff = Float_16( sprite->m_falloff );
	}

	m_spriteData.resize( n );
	for( int i = 0; i < n; ++i )
	{
		m_spriteData[i].position = m_sprites[i]->m_position;
		m_spriteData[i].boneIndex = m_sprites[i]->m_boneIndex;
	}

	CreateItemSetBoundingBoxes( m_aabb, m_boundingBoxes, m_skinned, begin( m_sprites ), end( m_sprites ) );
}

void EveSpriteSet::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Sprite Sets" );
	options.insert( "Sprite Sets Bounds" );
	options.insert( "Sprite Sets Lights" );
}

void EveSpriteSet::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform, const granny_matrix_3x4* bones, size_t boneCount )
{
	if( renderer.HasOption( GetRawRoot(), "Sprite Sets" ) )
	{
		for( auto it = m_sprites.begin(); it != m_sprites.end(); ++it )
		{
			Matrix transform = parentTransform;
			auto boneIndex = ( *it )->m_boneIndex;
			Tr2DebugColor color( Color( 0.0f, 0.7f, 0.9f, 0.5f ), Color( 0.0f, 0.7f, 0.9f, 0.1f ) );
			if( boneIndex >= 0 )
			{
				if( boneIndex < int( boneCount ) )
				{
					Matrix boneTF = IdentityMatrix();
					TriMatrixCopyFrom3x4( &boneTF, &bones[boneIndex] );
					transform = boneTF * transform;
				}
				else
				{
					color = Tr2DebugColor( Color( 0.9f, 0.2f, 0.2f, 0.5f ), Color( 0.9f, 0.2f, 0.2f, 0.1f ) );
				}
			}
			renderer.DrawSphere(
				*it,
				transform,
				( *it )->m_position,
				( *it )->m_maxScale,
				6,
				Tr2DebugRenderer::Lit,
				color );
		}
	}

	if( renderer.HasOption( GetRawRoot(), "Sprite Sets Bounds" ) )
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

	if( renderer.HasOption( this, "Sprite Sets Lights" ) )
	{
		for( auto& l : m_lights )
		{
			Matrix t = TranslationMatrix( l.lightData.position ) * l.boneMatrix * parentTransform;

			Color c = l.lightData.color;
			float blinkScale = EveSpriteLightUtils::Blink( l.blinkRate, l.blinkPhase, l.minScale, l.maxScale );

			c.a = 0.5;

			auto sprite = l.index > m_sprites.size() ? nullptr : m_sprites[l.index];

			renderer.DrawSphere(
				sprite,
				t,
				l.lightData.innerRadius * blinkScale,
				10,
				Tr2DebugRenderer::Solid,
				Tr2DebugColor( c ) );

			c.a = 0.3;
			renderer.DrawSphere(
				sprite,
				t,
				l.lightData.radius * blinkScale,
				10,
				Tr2DebugRenderer::Solid,
				Tr2DebugColor( c ) );

		}
	}
}

void EveSpriteSet::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	if( nullptr != m_effect )
	{
		m_effect->SetOption( name, value );
		m_effectHash = m_effect->GetHashValue();
		RegisterWithQuadRenderer( *Tr2QuadRenderer::Instance() );		
	}
}

void EveSpriteSet::AddLight( const EveSpriteLight& light )
{
	m_lights.push_back( light );
}

void EveSpriteSet::GetLights( Tr2LightManager& lightManager, const Matrix& parentTransform ) const
{
	LightFeatures features = LightFeatures();
	features.parentBrightness = m_activationStrength;

	for( auto& light : m_lights )
	{
		features.profileIndex = light.lightProfile == nullptr ? 0 : light.lightProfile->GetTextureIndex();

		auto data = light.lightData.AsPerPointLightData( light.boneMatrix * parentTransform, features );
		float blinkScale = EveSpriteLightUtils::Blink( light.blinkRate, light.blinkPhase, light.minScale, light.maxScale );
		data.radius *= blinkScale;
		data.innerRadius = Float_16( float( data.innerRadius ) * blinkScale );
		lightManager.AddLight(data);
	}
}
