// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dStretchVertical.h"
#include "Tr2AtlasTexture.h"
#include "Tr2Sprite2dTexture.h"
#include "Tr2Sprite2dScene.h"
#include "Tr2Sprite2dPickingMask.h"

// All frames can use the same indices - the layout of the sprite is always the same.
static unsigned short s_frameIndices[] = {
	0,
	2,
	3,
	0,
	3,
	1,
	4,
	6,
	7,
	4,
	7,
	5,

	// The two triangles in the center are kept last - then we can skip the
	// center by sending fewer triangles.
	2,
	4,
	5,
	2,
	5,
	3,
};

static const Color WHITE( 1.0f, 1.0f, 1.0f, 1.0f );

Tr2Sprite2dStretchVertical::Tr2Sprite2dStretchVertical( IRoot* lockobj ) :
	m_topEdgeSize( 0 ),
	m_bottomEdgeSize( 0 ),
	m_edgeScale( 1.f ),
	m_fillCenter( true ),
	m_offset( 0 ),
	m_saturation( 1.0f ),
	m_effectOpacity( 1.0f )
{
	std::fill( std::begin( m_vertices ), std::end( m_vertices ), Tr2Sprite2dD3DVertex() );
}

void Tr2Sprite2dStretchVertical::GatherSprites( Tr2Sprite2dScene* renderer )
{
	if( !m_display )
	{
		return;
	}

	if( m_isDirty )
	{
		if( !ValidateAndSetTextures( renderer ) )
		{
			return;
		}

		if( m_spriteEffect == TR2_SFX_BLUR || m_spriteEffect == TR2_SFX_GLOW )
		{
			// don't support these atm
			return;
		}

		renderer->SetSpriteEffect( m_spriteEffect );
		renderer->SetTileMode( 0 );

		float srcHeight = m_texturePrimary->GetSrcHeight();
		if( srcHeight == 0.0f )
		{
			srcHeight = float( m_texturePrimary->GetHeight() );
		}

		Tr2Sprite2dVertexBase vertices[8 * 3];

		if( m_shadowOffset.x != 0.0f || m_shadowOffset.y != 0.0f )
		{
			SetShadowRenderState( renderer );
			PrepareVertices( &vertices[0], srcHeight, m_shadowOffset, Vector2( 1, 1 ) );
			renderer->PrepareTriangleVerts( &m_vertices[0], &vertices[0], sizeof( Tr2Sprite2dVertexBase ), 8 );
		}

		SetRegularRenderState( renderer );
		PrepareVertices( &vertices[8], srcHeight, Vector2( 0, 0 ), Vector2( 1, 1 ) );
		renderer->PrepareTriangleVerts( &m_vertices[8], &vertices[8], sizeof( Tr2Sprite2dVertexBase ), 8 );

		if( ( m_glowFactor != 0.0f ) || ( m_glowExpand != 0.0f ) )
		{
			float offset2 = m_glowExpand + m_glowExpand;

			SetGlowRenderState( renderer );
			PrepareVertices( &vertices[16], srcHeight, Vector2( -m_glowExpand, -m_glowExpand ), Vector2( 1 + offset2 / m_displayWidth, 1 + offset2 / m_displayHeight ) );
			renderer->PrepareTriangleVerts( &m_vertices[16], &vertices[16], sizeof( Tr2Sprite2dVertexBase ), 8 );
		}

		if( m_spriteEffect < TR2_SFX_TWO_TEXTURES && ( m_spriteEffect != TR2_SFX_BLUR ) && ( m_spriteEffect != TR2_SFX_GLOW ) )
		{
			for( int i = 0; i < 8 * 3; ++i )
			{
				m_vertices[i].texCoord[1].x = m_saturation;
				m_vertices[i].texCoord[1].y = m_effectOpacity;
			}
		}

		m_isDirty = false;
	}

	SetValidatedTextures( renderer );

	renderer->PushTranslation( m_translation );

	unsigned int triangleCount = m_fillCenter ? 6 : 4;
	unsigned int indexCount = triangleCount * 3;
	if( m_shadowOffset.x != 0.0f || m_shadowOffset.y != 0.0f )
	{
		renderer->RenderTriangleVerts( &m_vertices[0], 8, &s_frameIndices[0], indexCount );
	}
	renderer->RenderTriangleVerts( &m_vertices[8], 8, &s_frameIndices[0], indexCount );
	if( ( m_glowFactor != 0.0f ) || ( m_glowExpand != 0.0f ) )
	{
		renderer->RenderTriangleVerts( &m_vertices[16], 8, &s_frameIndices[0], indexCount );
	}

	renderer->PopTranslation();
}

ITr2SpriteObject* Tr2Sprite2dStretchVertical::PickPoint( float x, float y, Tr2Sprite2dScene* renderer )
{
	if( !m_display )
	{
		return NULL;
	}

	if( m_pickState == TR2_SPS_ON )
	{
		if( renderer->IsInside( Vector2( x, y ), Vector2( m_translation.x, m_translation.y ), m_displayWidth, m_displayHeight, m_pickingMask ? 0.f : -1.0f ) )
		{
			if( !m_pickingMask || m_pickingMask->SampleMask( renderer->InverseTransformPoint( Vector2( x, y ) ), m_translation, m_displayWidth, m_displayHeight ) )
			{
				return this;
			}
		}
	}

	return NULL;
}

unsigned int Tr2Sprite2dStretchVertical::GetVertexCount()
{
	if( !m_display )
	{
		return 0;
	}

	return 8 * 3;
}

void Tr2Sprite2dStretchVertical::PrepareVertices( Tr2Sprite2dVertexBase* v, float srcHeight, const Vector2& vertOffset, const Vector2& scale ) const
{
	float topEdgeSize = (float)m_topEdgeSize;
	float bottomEdgeSize = (float)m_bottomEdgeSize;

	float offset = float( m_offset );

	float scaledWidth = m_displayWidth * scale.x - offset * scale.x;
	float scaledHeight = m_displayHeight * scale.y - offset * 2.0f * scale.y;
	float offsetY = offset * scale.y;

	const Color white( 1, 1, 1, 1 );
	//
	// First line
	//
	// Vertex 0
	v->position.x = 0.0f + vertOffset.x;
	v->position.y = offsetY + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 0.0f;
	v->texCoord[0].y = 0.0f;
	v->texCoord[1].x = 0.0f;
	v->texCoord[1].y = 0.0f;
	v->color = white;
	++v;

	// Vertex 1
	v->position.x = scaledWidth + vertOffset.x;
	v->position.y = 0.0f + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 1.0f;
	v->texCoord[0].y = 0.0f;
	v->texCoord[1].x = 1.0f;
	v->texCoord[1].y = 0.0f;
	v->color = white;
	++v;

	//
	// Second line
	//
	// Vertex 2
	v->position.x = 0.0f + vertOffset.x;
	v->position.y = offsetY + topEdgeSize * m_edgeScale * scale.y + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 0.0f;
	v->texCoord[0].y = topEdgeSize / srcHeight;
	v->texCoord[1].x = 0.0f;
	v->texCoord[1].y = topEdgeSize / srcHeight;
	v->color = white;
	++v;

	// Vertex 3
	v->position.x = scaledWidth + vertOffset.x;
	v->position.y = offsetY + topEdgeSize * m_edgeScale * scale.y + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 1.0f;
	v->texCoord[0].y = topEdgeSize / srcHeight;
	v->texCoord[1].x = 1.0f;
	v->texCoord[1].y = topEdgeSize / srcHeight;
	v->color = white;
	++v;

	//
	// Third line
	//
	// Vertex 4
	v->position.x = 0.0f + vertOffset.x;
	v->position.y = offsetY + scaledHeight - bottomEdgeSize * m_edgeScale * scale.y + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 0.0f;
	v->texCoord[0].y = 1.0f - bottomEdgeSize / srcHeight;
	v->texCoord[1].x = 0.0f;
	v->texCoord[1].y = 1.0f - bottomEdgeSize / srcHeight;
	v->color = white;
	++v;

	// Vertex 5
	v->position.x = scaledWidth + vertOffset.x;
	v->position.y = offsetY + scaledHeight - bottomEdgeSize * m_edgeScale * scale.y + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 1.0f;
	v->texCoord[0].y = 1.0f - bottomEdgeSize / srcHeight;
	v->texCoord[1].x = 1.0f;
	v->texCoord[1].y = 1.0f - bottomEdgeSize / srcHeight;
	v->color = white;
	++v;

	//
	// Fourth line
	//
	// Vertex 6
	v->position.x = 0.0f + vertOffset.x;
	v->position.y = scaledHeight + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 0.0f;
	v->texCoord[0].y = 1.0f;
	v->texCoord[1].x = 0.0f;
	v->texCoord[1].y = 1.0f;
	v->color = white;
	++v;

	// Vertex 7
	v->position.x = scaledWidth + vertOffset.x;
	v->position.y = scaledHeight + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 1.0f;
	v->texCoord[0].y = 1.0f;
	v->texCoord[1].x = 1.0f;
	v->texCoord[1].y = 1.0f;
	v->color = white;
}