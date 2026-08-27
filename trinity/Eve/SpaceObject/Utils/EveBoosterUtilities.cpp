// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveBoosterUtilities.h"
#include "Resources/TriGeometryRes.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteSet.h"
#include "Tr2Renderer.h"

namespace
{

struct EveBoosterVertex
{
	Vector3 position;
	Vector2 texCoord;
};

struct EveChildBoosterVertex
{
	Vector3 position;
};

template <typename Vertex>
ALResult GetBoxVB( Tr2SuballocatedBuffer::Allocation& vb, Tr2PrimaryRenderContext& renderContext )
{
	const uint32_t vertexCount = 4 * 6;
	Vertex vertices[vertexCount];
	auto p = &vertices[0];
	( p++ )->position = Vector3( -1.0f, -1.0f, 0.0f );
	( p++ )->position = Vector3( 1.0f, -1.0f, 0.0f );
	( p++ )->position = Vector3( 1.0f, 1.0f, 0.0f );
	( p++ )->position = Vector3( -1.0f, 1.0f, 0.0f );

	( p++ )->position = Vector3( -1.0f, -1.0f, -1.0f );
	( p++ )->position = Vector3( -1.0f, 1.0f, -1.0f );
	( p++ )->position = Vector3( 1.0f, 1.0f, -1.0f );
	( p++ )->position = Vector3( 1.0f, -1.0f, -1.0f );

	( p++ )->position = Vector3( -1.0f, -1.0f, 0.0f );
	( p++ )->position = Vector3( -1.0f, 1.0f, 0.0f );
	( p++ )->position = Vector3( -1.0f, 1.0f, -1.0f );
	( p++ )->position = Vector3( -1.0f, -1.0f, -1.0f );

	( p++ )->position = Vector3( 1.0f, -1.0f, 0.0f );
	( p++ )->position = Vector3( 1.0f, -1.0f, -1.0f );
	( p++ )->position = Vector3( 1.0f, 1.0f, -1.0f );
	( p++ )->position = Vector3( 1.0f, 1.0f, 0.0f );

	( p++ )->position = Vector3( -1.0f, -1.0f, 0.0f );
	( p++ )->position = Vector3( -1.0f, -1.0f, -1.0f );
	( p++ )->position = Vector3( 1.0f, -1.0f, -1.0f );
	( p++ )->position = Vector3( 1.0f, -1.0f, 0.0f );

	( p++ )->position = Vector3( -1.0f, 1.0f, 0.0f );
	( p++ )->position = Vector3( 1.0f, 1.0f, 0.0f );
	( p++ )->position = Vector3( 1.0f, 1.0f, -1.0f );
	( p++ )->position = Vector3( -1.0f, 1.0f, -1.0f );

	return g_sharedBuffer.Allocate( sizeof( Vertex ), vertexCount, &vertices[0], renderContext, vb );
}

ALResult GetStarVB( Tr2SuballocatedBuffer::Allocation& vb, Tr2PrimaryRenderContext& renderContext )
{
	const uint32_t vertexCount = 4 * 4;
	EveBoosterVertex vertices[vertexCount];
	auto p = &vertices[0];
	for( unsigned int i = 0; i < vertexCount; i += 4 )
	{
		float t = (float)i * XM_PI / 4.f / 4.f;
		float x = cos( t ) * 0.5f;
		float y = sin( t ) * 0.5f;
		p->position = Vector3( -x, -y, 0.f );
		p->texCoord = Vector2( 1.f, 1.f );
		++p;
		p->position = Vector3( -x, -y, -1.f );
		p->texCoord = Vector2( 1.f, 0.f );
		++p;
		p->position = Vector3( x, y, -1.f );
		p->texCoord = Vector2( 0.f, 0.f );
		++p;
		p->position = Vector3( x, y, 0.0f );
		p->texCoord = Vector2( 0.f, 1.f );
		++p;
	}

	return g_sharedBuffer.Allocate( sizeof( EveBoosterVertex ), vertexCount, &vertices[0], renderContext, vb );
}

}

Tr2ProceduralBuffer MakeChildBoosterBoxBuffer()
{
	return Tr2ProceduralBuffer( BlueSharedString( "ChildBoosterBoxVB" ), GetBoxVB<EveChildBoosterVertex> );
}

Tr2ProceduralBuffer MakeBoosterBoxBuffer()
{
	return Tr2ProceduralBuffer( BlueSharedString( "BoosterBoxVB" ), GetBoxVB<EveBoosterVertex> );
}

Tr2ProceduralBuffer MakeBoosterStarBuffer()
{
	return Tr2ProceduralBuffer( BlueSharedString( "BoosterStarVB" ), GetStarVB );
}

void CreateBoosterFlares( EveSpriteSet& glows,
						  const Matrix& transform,
						  const EveBoosterFlareParams& params )
{
	// grab pos/dir/scale from the local transform matrix
	Vector3 pos( transform._41, transform._42, transform._43 );
	Vector3 dir( transform._31, transform._32, transform._33 );
	float scale = std::max( Length( transform.GetX() ), Length( transform.GetY() ) );

	dir = Normalize( dir );
	if( scale < 3.f )
	{
		dir *= scale / 3.f;
	}

	float seed = float( rand() ) / float( RAND_MAX ) * 0.7f;

	Vector3 spritePos = pos - 2.5f * dir;
	glows.Add( spritePos, seed, seed, scale * params.glowScale, scale * params.glowScale, 0.0f, params.glowColor, params.warpGlowColor );

	spritePos = pos - 3.0f * dir;
	glows.Add( spritePos, seed, 1.0f + seed, scale * params.symHaloScale, scale * params.symHaloScale, 0.0f, params.haloColor, params.warpHaloColor );

	spritePos = pos - 3.01f * dir;
	glows.Add( spritePos, seed, 1.0f + seed, scale * params.haloScaleX, scale * params.haloScaleY, 0.0f, params.haloColor, params.warpHaloColor );
}

namespace
{
// constants
const unsigned g_lightNoiseSize = 128;
}

float ComputeBoosterLightFlicker( float phase, float amplitude, float frequency )
{
	static const std::array<float, g_lightNoiseSize> noise = [] {
		std::array<float, g_lightNoiseSize> table;
		for( auto& v : table )
		{
			v = float( rand() ) / float( RAND_MAX );
		}
		return table;
	}();

	phase = ( phase + Tr2Renderer::GetAnimationTime() ) * frequency;
	float p0 = noise[int( phase ) % g_lightNoiseSize];
	float p1 = noise[( int( phase ) + 1 ) % g_lightNoiseSize];
	float t = phase - std::floor( phase );
	float flicker = 1 + amplitude * 2.0f * ( p0 * ( 1.0f - t ) + p1 * t ) - amplitude;

	return flicker;
}

float GenerateBoosterLightPhase()
{
	return float( g_lightNoiseSize ) * float( rand() ) / float( RAND_MAX );
}

Vector4 PadBoosterBoundingSphere( Vector4 boosterBoundingSphere, const Matrix& transform )
{
	Vector4 boundingSphere;
	// move bounding sphere back to catch all the glowy exhaust
	boundingSphere = boosterBoundingSphere + Vector4( 0.f, 0.f, -0.5f * boosterBoundingSphere.w, 0.f );
	// transform center into worldspace
	boundingSphere.GetXYZ() = TransformCoord( boundingSphere.GetXYZ(), transform );
	// blow up radius so we contain all the glowy stuff coming out of a booster
	boundingSphere.w = 2.f * boosterBoundingSphere.w;

	return boundingSphere;
}
