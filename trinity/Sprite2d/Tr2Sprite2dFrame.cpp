// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dFrame.h"
#include "Tr2AtlasTexture.h"
#include "Tr2Sprite2dTexture.h"
#include "Tr2Sprite2dScene.h"
#include "Tr2Sprite2dPickingMask.h"

// All frames can use the same indices - the layout of the frame is always the same.
static unsigned short s_frameIndices[] = {
	0,
	4,
	5,
	0,
	5,
	1,
	1,
	5,
	6,
	1,
	6,
	2,
	2,
	6,
	7,
	2,
	7,
	3,
	4,
	8,
	9,
	4,
	9,
	5,
	6,
	10,
	11,
	6,
	11,
	7,
	8,
	12,
	13,
	8,
	13,
	9,
	9,
	13,
	14,
	9,
	14,
	10,
	10,
	14,
	15,
	10,
	15,
	11,

	// The two triangles in the center are kept last - then we can skip the
	// center by sending fewer triangles.
	5,
	9,
	10,
	5,
	10,
	6,
};

static const Color WHITE( 1.0f, 1.0f, 1.0f, 1.0f );

Tr2Sprite2dFrame::Tr2Sprite2dFrame( IRoot* lockobj ) :
	m_cornerSize( 0 ),
	m_cornerScale( 1.f ),
	m_offset( 0 ),
	m_fillCenter( true ),
	m_saturation( 1.0f ),
	m_effectOpacity( 1.0f )
{
	std::fill( std::begin( m_vertices ), std::end( m_vertices ), Tr2Sprite2dD3DVertex() );
}

void Tr2Sprite2dFrame::GatherSprites( Tr2Sprite2dScene* renderer )
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

		float srcWidth = m_texturePrimary->GetSrcWidth();
		float srcHeight = m_texturePrimary->GetSrcHeight();

		if( srcWidth == 0.0f )
		{
			srcWidth = float( m_texturePrimary->GetWidth() );
		}
		if( srcHeight == 0.0f )
		{
			srcHeight = float( m_texturePrimary->GetHeight() );
		}

		if( srcWidth < m_cornerSize * 2 )
		{
			return;
		}

		if( srcHeight < m_cornerSize * 2 )
		{
			return;
		}

		Tr2Sprite2dVertexBase srcVertices[16 * 3];

		if( m_shadowOffset.x != 0.0f || m_shadowOffset.y != 0.0f )
		{
			SetShadowRenderState( renderer );
			PrepareVertices( &srcVertices[0], srcWidth, srcHeight, m_shadowOffset, Vector2( 1, 1 ) );
			renderer->PrepareTriangleVerts( &m_vertices[0], &srcVertices[0], sizeof( Tr2Sprite2dVertexBase ), 16 );
		}


		SetRegularRenderState( renderer );
		renderer->SetDepth( m_depth );
		PrepareVertices( &srcVertices[16], srcWidth, srcHeight, Vector2( 0, 0 ), Vector2( 1, 1 ) );
		renderer->PrepareTriangleVerts( &m_vertices[16], &srcVertices[16], sizeof( Tr2Sprite2dVertexBase ), 16 );

		if( ( m_glowFactor != 0.0f ) || ( m_glowExpand != 0.0f ) )
		{
			float offset2 = m_glowExpand + m_glowExpand;

			SetGlowRenderState( renderer );
			PrepareVertices( &srcVertices[32], srcWidth, srcHeight, Vector2( -m_glowExpand, -m_glowExpand ), Vector2( 1 + offset2 / m_displayWidth, 1 + offset2 / m_displayHeight ) );
			renderer->PrepareTriangleVerts( &m_vertices[32], &srcVertices[32], sizeof( Tr2Sprite2dVertexBase ), 16 );
		}

		if( m_spriteEffect < TR2_SFX_TWO_TEXTURES && ( m_spriteEffect != TR2_SFX_BLUR ) && ( m_spriteEffect != TR2_SFX_GLOW ) )
		{
			for( int i = 0; i < 16 * 3; ++i )
			{
				m_vertices[i].texCoord[1].x = m_saturation;
				m_vertices[i].texCoord[1].y = m_effectOpacity;
			}
		}

		m_isDirty = false;
	}

	SetValidatedTextures( renderer );

	unsigned int triangleCount = m_fillCenter ? 18 : 16;
	unsigned int indexCount = triangleCount * 3;

	if( m_shadowOffset.x != 0.0f || m_shadowOffset.y != 0.0f )
	{
		renderer->RenderTriangleVerts( &m_vertices[0], 16, &s_frameIndices[0], indexCount );
	}
	renderer->RenderTriangleVerts( &m_vertices[16], 16, &s_frameIndices[0], indexCount );
	if( ( m_glowFactor != 0.0f ) || ( m_glowExpand != 0.0f ) )
	{
		renderer->RenderTriangleVerts( &m_vertices[32], 16, &s_frameIndices[0], indexCount );
	}
}

ITr2SpriteObject* Tr2Sprite2dFrame::PickPoint( float x, float y, Tr2Sprite2dScene* renderer )
{
	if( !m_display )
	{
		return NULL;
	}

	if( m_pickState == TR2_SPS_ON )
	{
		if( renderer->IsInside( Vector2( x, y ), Vector2( m_translation.x, m_translation.y ), m_displayWidth, m_displayHeight, 0.0f ) )
		{
			if( !m_pickingMask || m_pickingMask->SampleMask( renderer->InverseTransformPoint( Vector2( x, y ) ), m_translation, m_displayWidth, m_displayHeight ) )
			{
				return this;
			}
		}
	}

	return NULL;
}

unsigned int Tr2Sprite2dFrame::GetVertexCount()
{
	return 16 * 3;
}

void Tr2Sprite2dFrame::PrepareVertices( Tr2Sprite2dVertexBase* v, float srcWidth, float srcHeight, const Vector2& vertOffset, const Vector2& scale ) const
{
	float cs = floor( 0.5f + m_cornerScale * static_cast<float>( m_cornerSize ) );

	float texHorizontalCs = static_cast<float>( m_cornerSize ) / srcWidth;
	float texVerticalCs = static_cast<float>( m_cornerSize ) / srcHeight;

	const Color white( WHITE );

	float offset = (float)m_offset;
	float offset_2 = offset * 2.0f;

	float offsetWidth = m_displayWidth * scale.x - offset_2 * scale.x;
	float offsetHeight = m_displayHeight * scale.y - offset_2 * scale.y;

	float offsetX = offset * scale.x + m_translation.x;
	float offsetY = offset * scale.y + m_translation.y;

	//
	// First line
	//
	// Vertex 0
	v->position.x = offsetX + vertOffset.x;
	v->position.y = offsetY + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 0.0f;
	v->texCoord[0].y = 0.0f;
	v->texCoord[1].x = 0.0f;
	v->texCoord[1].y = 0.0f;
	v->color = white;
	++v;

	// Vertex 1
	v->position.x = cs * scale.x + offsetX + vertOffset.x;
	v->position.y = offsetY + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = texHorizontalCs;
	v->texCoord[0].y = 0.0f;
	v->texCoord[1].x = texHorizontalCs;
	v->texCoord[1].y = 0.0f;
	v->color = white;
	++v;

	// Vertex 2
	v->position.x = offsetWidth - cs * scale.x + offsetX + vertOffset.x;
	v->position.y = offsetY + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 1.0f - texHorizontalCs;
	v->texCoord[0].y = 0.0f;
	v->texCoord[1].x = 1.0f - texHorizontalCs;
	v->texCoord[1].y = 0.0f;
	v->color = white;
	++v;

	// Vertex 3
	v->position.x = offsetWidth + offsetX + vertOffset.x;
	v->position.y = offsetY + vertOffset.y;
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
	// Vertex 4
	v->position.x = offsetX + vertOffset.x;
	v->position.y = cs * scale.y + offsetY + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 0.0f;
	v->texCoord[0].y = texVerticalCs;
	v->texCoord[1].x = 0.0f;
	v->texCoord[1].y = texVerticalCs;
	v->color = white;
	++v;

	// Vertex 5
	v->position.x = cs * scale.x + offsetX + vertOffset.x;
	v->position.y = cs * scale.y + offsetY + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = texHorizontalCs;
	v->texCoord[0].y = texVerticalCs;
	v->texCoord[1].x = texHorizontalCs;
	v->texCoord[1].y = texVerticalCs;
	v->color = white;
	++v;

	// Vertex 6
	v->position.x = offsetWidth - cs * scale.x + offsetX + vertOffset.x;
	v->position.y = cs * scale.y + offsetY + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 1.0f - texHorizontalCs;
	v->texCoord[0].y = texVerticalCs;
	v->texCoord[1].x = 1.0f - texHorizontalCs;
	v->texCoord[1].y = texVerticalCs;
	v->color = white;
	++v;

	// Vertex 7
	v->position.x = offsetWidth + offsetX + vertOffset.x;
	v->position.y = cs * scale.y + offsetY + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 1.0f;
	v->texCoord[0].y = texVerticalCs;
	v->texCoord[1].x = 1.0f;
	v->texCoord[1].y = texVerticalCs;
	v->color = white;
	++v;

	//
	// Third line
	//
	// Vertex 8
	v->position.x = offsetX + vertOffset.x;
	v->position.y = offsetHeight - cs * scale.y + offsetY + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 0.0f;
	v->texCoord[0].y = 1.0f - texVerticalCs;
	v->texCoord[1].x = 0.0f;
	v->texCoord[1].y = 1.0f - texVerticalCs;
	v->color = white;
	++v;

	// Vertex 9
	v->position.x = cs * scale.x + offsetX + vertOffset.x;
	v->position.y = offsetHeight - cs * scale.y + offsetY + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = texHorizontalCs;
	v->texCoord[0].y = 1.0f - texVerticalCs;
	v->texCoord[1].x = texHorizontalCs;
	v->texCoord[1].y = 1.0f - texVerticalCs;
	v->color = white;
	++v;

	// Vertex 10
	v->position.x = offsetWidth - cs * scale.x + offsetX + vertOffset.x;
	v->position.y = offsetHeight - cs * scale.y + offsetY + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 1.0f - texHorizontalCs;
	v->texCoord[0].y = 1.0f - texVerticalCs;
	v->texCoord[1].x = 1.0f - texHorizontalCs;
	v->texCoord[1].y = 1.0f - texVerticalCs;
	v->color = white;
	++v;

	// Vertex 11
	v->position.x = offsetWidth + offsetX + vertOffset.x;
	v->position.y = offsetHeight - cs * scale.y + offsetY + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 1.0f;
	v->texCoord[0].y = 1.0f - texVerticalCs;
	v->texCoord[1].x = 1.0f;
	v->texCoord[1].y = 1.0f - texVerticalCs;
	v->color = white;
	++v;

	//
	// Fourth line
	//
	// Vertex 12
	v->position.x = 0.0f + offsetX + vertOffset.x;
	v->position.y = offsetHeight + offsetY + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 0.0f;
	v->texCoord[0].y = 1.0f;
	v->texCoord[1].x = 0.0f;
	v->texCoord[1].y = 1.0f;
	v->color = white;
	++v;

	// Vertex 13
	v->position.x = cs * scale.x + offsetX + vertOffset.x;
	v->position.y = offsetHeight + offsetY + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = texHorizontalCs;
	v->texCoord[0].y = 1.0f;
	v->texCoord[1].x = texHorizontalCs;
	v->texCoord[1].y = 1.0f;
	v->color = white;
	++v;

	// Vertex 14
	v->position.x = offsetWidth - cs * scale.x + offsetX + vertOffset.x;
	v->position.y = offsetHeight + offsetY + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 1.0f - texHorizontalCs;
	v->texCoord[0].y = 1.0f;
	v->texCoord[1].x = 1.0f - texHorizontalCs;
	v->texCoord[1].y = 1.0f;
	v->color = white;
	++v;

	// Vertex 15
	v->position.x = offsetWidth + offsetX + vertOffset.x;
	v->position.y = offsetHeight + offsetY + vertOffset.y;
	v->position.z = 0.0f;
	v->texCoord[0].x = 1.0f;
	v->texCoord[0].y = 1.0f;
	v->texCoord[1].x = 1.0f;
	v->texCoord[1].y = 1.0f;
	v->color = white;
}