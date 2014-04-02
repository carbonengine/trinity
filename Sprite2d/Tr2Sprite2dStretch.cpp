#include "StdAfx.h"
#include "Tr2Sprite2dStretch.h"
#include "Tr2AtlasTexture.h"
#include "Tr2Sprite2dTexture.h"
#include "Tr2Sprite2dScene.h"

// All frames can use the same indices - the layout of the sprite is always the same.
static unsigned short s_frameIndices[] = {
	0, 4, 5,
	0, 5, 1,
	2, 6, 7,
	2, 7, 3,

	// The two triangles in the center are kept last - then we can skip the
	// center by sending fewer triangles.
	1, 5, 6,
	1, 6, 2,
};

static const Color WHITE( 1.0f, 1.0f, 1.0f, 1.0f );

Tr2Sprite2dStretch::Tr2Sprite2dStretch( IRoot* lockobj ) : 
	m_leftEdgeSize( 0 ),
	m_rightEdgeSize( 0 ),
	m_offset( 0 ),
	m_fillCenter( true ),
	m_dpiScaleBehavior( S2D_SSC_ALIGN_BOTTOMRIGHT )
{
}

void Tr2Sprite2dStretch::GatherSprites( Tr2Sprite2dScene* renderer )
{
	if( !m_display )
	{
		return;
	}

	if( m_isDirty )
	{
		if( !m_texture )
		{
			return;
		}

		if( !m_texture->IsGood() )
		{
			return;
		}

		m_texture->Apply( renderer, 0 );

		renderer->SetSpriteEffect( m_spriteEffect );
		renderer->SetTileMode( 0 );

		float srcWidth = m_texture->GetSrcWidth();

		unsigned int textureWidth = m_texture->GetWidth();

		if( srcWidth == 0.0f )
		{
			srcWidth = (float)textureWidth;
		}

		if( srcWidth < m_leftEdgeSize + m_rightEdgeSize )
		{
			return;
		}

		float leftEdgeSize = (float)m_leftEdgeSize;
		float rightEdgeSize = (float)m_rightEdgeSize;

		SetRegularRenderState( renderer );
		renderer->SetDepth( m_depth );

		const Color white( WHITE );

		float offset = (float)m_offset;
		float offset_2 = offset * 2.0f;

		float scaledWidth = m_displayWidth - offset_2;
		float scaledHeight = m_displayHeight - offset;
		float offsetX = offset;
		float offsetY = offset;

		float y0;
		float y1;

		y0 = 0;
		y1 = scaledHeight;

		y0 += offsetY;
		y1 += offsetY;

		Tr2Sprite2dVertexBase vertices[8];
		Tr2Sprite2dVertexBase* v = &vertices[0];

		//
		// First line
		//
		// Vertex 0
		v->position.x = offsetX;
		v->position.y = y0;
		v->position.z = 0.0f;
		v->texCoord[0].x = 0.0f;
		v->texCoord[0].y = 0.0f;
		v->color = white;
		++v;

		// Vertex 1
		v->position.x = offsetX + leftEdgeSize;
		v->position.y = y0;
		v->position.z = 0.0f;
		v->texCoord[0].x = leftEdgeSize / srcWidth;
		v->texCoord[0].y = 0.0f;
		v->color = white;
		++v;

		// Vertex 2
		v->position.x = offsetX + scaledWidth - rightEdgeSize;
		v->position.y = y0;
		v->position.z = 0.0f;
		v->texCoord[0].x = 1.0f - rightEdgeSize / srcWidth;
		v->texCoord[0].y = 0.0f;
		v->color = white;
		++v;

		// Vertex 3
		v->position.x = offsetX + scaledWidth;
		v->position.y = y0;
		v->position.z = 0.0f;
		v->texCoord[0].x = 1.0f;
		v->texCoord[0].y = 0.0f;
		v->color = white;
		++v;

		//
		// Second line
		//
		// Vertex 4
		v->position.x = offsetX;
		v->position.y = y1;
		v->position.z = 0.0f;
		v->texCoord[0].x = 0.0f;
		v->texCoord[0].y = 1.0f;
		v->color = white;
		++v;

		// Vertex 5
		v->position.x = offsetX + leftEdgeSize;
		v->position.y = y1;
		v->position.z = 0.0f;
		v->texCoord[0].x = leftEdgeSize / srcWidth;
		v->texCoord[0].y = 1.0f;
		v->color = white;
		++v;

		// Vertex 6
		v->position.x = offsetX + scaledWidth - rightEdgeSize;
		v->position.y = y1;
		v->position.z = 0.0f;
		v->texCoord[0].x = 1.0f - rightEdgeSize / srcWidth;
		v->texCoord[0].y = 1.0f;
		v->color = white;
		++v;

		// Vertex 7
		v->position.x = offsetX + scaledWidth;
		v->position.y = y1;
		v->position.z = 0.0f;
		v->texCoord[0].x = 1.0f;
		v->texCoord[0].y = 1.0f;
		v->color = white;
		++v;

		renderer->PrepareTriangleVerts( &m_vertices[0], &vertices[0], sizeof( Tr2Sprite2dVertexBase ), 8 );
		m_isDirty = false;
	}

	m_texture->Apply( renderer, 0 );
	renderer->PushTranslation( m_translation );

	unsigned int triangleCount = m_fillCenter ? 6 : 4;
	unsigned int indexCount = triangleCount * 3;
	renderer->RenderTriangleVerts( &m_vertices[0], 8, &s_frameIndices[0], indexCount );

	renderer->PopTranslation();
}

ITr2SpriteObject* Tr2Sprite2dStretch::PickPoint( float x, float y, Tr2Sprite2dScene* renderer )
{
	if( !m_display )
	{
		return NULL;
	}

	if( m_pickState == TR2_SPS_ON )
	{
		if( renderer->IsInside( Vector2( x, y ), Vector2( m_translation.x, m_translation.y ), m_displayWidth, m_displayHeight, -1.0f ) )
		{
			return this;
		}
	}

	return NULL;
}

unsigned int Tr2Sprite2dStretch::GetVertexCount()
{
	if( !m_display )
	{
		return 0;
	}

	return 8;
}
