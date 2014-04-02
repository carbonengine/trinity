#include "StdAfx.h"
#include "Tr2Sprite2dStretchVertical.h"
#include "Tr2AtlasTexture.h"
#include "Tr2Sprite2dTexture.h"
#include "Tr2Sprite2dScene.h"

// All frames can use the same indices - the layout of the sprite is always the same.
static unsigned short s_frameIndices[] = {
	0, 2, 3,
	0, 3, 1,
	4, 6, 7,
	4, 7, 5,

	// The two triangles in the center are kept last - then we can skip the
	// center by sending fewer triangles.
	2, 4, 5,
	2, 5, 3,
};

static const Color WHITE( 1.0f, 1.0f, 1.0f, 1.0f );

Tr2Sprite2dStretchVertical::Tr2Sprite2dStretchVertical( IRoot* lockobj ) : 
	m_topEdgeSize( 0 ),
	m_bottomEdgeSize( 0 ),
	m_fillCenter( true )
{
}

void Tr2Sprite2dStretchVertical::GatherSprites( Tr2Sprite2dScene* renderer )
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

		float srcHeight = m_texture->GetSrcHeight();

		unsigned int textureHeight = m_texture->GetHeight();

		if( srcHeight == 0.0f )
		{
			srcHeight = (float)textureHeight;
		}

		float topEdgeSize = (float)m_topEdgeSize;
		float bottomEdgeSize = (float)m_bottomEdgeSize;

		SetRegularRenderState( renderer );
		renderer->SetDepth( m_depth );

		const Color white( WHITE );

		float scaledWidth = m_displayWidth;
		float scaledHeight = m_displayHeight;

		Tr2Sprite2dVertexBase vertices[8];
		Tr2Sprite2dVertexBase* v = &vertices[0];
		//
		// First line
		//
		// Vertex 0
		v->position.x = 0.0f;
		v->position.y = 0.0f;
		v->position.z = 0.0f;
		v->texCoord[0].x = 0.0f;
		v->texCoord[0].y = 0.0f;
		v->color = white;
		++v;

		// Vertex 1
		v->position.x = scaledWidth;
		v->position.y = 0.0f;
		v->position.z = 0.0f;
		v->texCoord[0].x = 1.0f;
		v->texCoord[0].y = 0.0f;
		v->color = white;
		++v;

		//
		// Second line
		//
		// Vertex 2
		v->position.x = 0.0f;
		v->position.y = topEdgeSize;
		v->position.z = 0.0f;
		v->texCoord[0].x = 0.0f;
		v->texCoord[0].y = topEdgeSize / srcHeight;
		v->color = white;
		++v;

		// Vertex 3
		v->position.x = scaledWidth;
		v->position.y = topEdgeSize;
		v->position.z = 0.0f;
		v->texCoord[0].x = 1.0f;
		v->texCoord[0].y = topEdgeSize / srcHeight;
		v->color = white;
		++v;

		//
		// Third line
		//
		// Vertex 4
		v->position.x = 0.0f;
		v->position.y = scaledHeight - bottomEdgeSize;
		v->position.z = 0.0f;
		v->texCoord[0].x = 0.0f;
		v->texCoord[0].y = 1.0f - topEdgeSize / srcHeight;
		v->color = white;
		++v;

		// Vertex 5
		v->position.x = scaledWidth;
		v->position.y = scaledHeight - bottomEdgeSize;
		v->position.z = 0.0f;
		v->texCoord[0].x = 1.0f;
		v->texCoord[0].y = 1.0f - topEdgeSize / srcHeight;
		v->color = white;
		++v;

		//
		// Fourth line
		// 
		// Vertex 6
		v->position.x = 0.0f;
		v->position.y = scaledHeight;
		v->position.z = 0.0f;
		v->texCoord[0].x = 0.0f;
		v->texCoord[0].y = 1.0f;
		v->color = white;
		++v;

		// Vertex 7
		v->position.x = scaledWidth;
		v->position.y = scaledHeight;
		v->position.z = 0.0f;
		v->texCoord[0].x = 1.0f;
		v->texCoord[0].y = 1.0f;
		v->color = white;
		++v;

		renderer->PrepareTriangleVerts( &m_vertices[0], &vertices[0], sizeof( Tr2Sprite2dVertexBase ), 8 );

	}

	m_texture->Apply( renderer, 0 );
	
	renderer->PushTranslation( m_translation );
	
	unsigned int triangleCount = m_fillCenter ? 6 : 4;
	unsigned int indexCount = triangleCount * 3;
	renderer->RenderTriangleVerts( &m_vertices[0], 8, &s_frameIndices[0], indexCount );
	
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
		if( renderer->IsInside( Vector2( x, y ), Vector2( m_translation.x, m_translation.y ), m_displayWidth, m_displayHeight, -1.0f ) )
		{
			return this;
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

	return 8;
}
