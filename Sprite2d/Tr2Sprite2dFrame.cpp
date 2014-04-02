#include "StdAfx.h"
#include "Tr2Sprite2dFrame.h"
#include "Tr2AtlasTexture.h"
#include "Tr2Sprite2dTexture.h"
#include "Tr2Sprite2dScene.h"

// All frames can use the same indices - the layout of the frame is always the same.
static unsigned short s_frameIndices[] = {
	0, 4, 5,
	0, 5, 1,
	1, 5, 6,
	1, 6, 2,
	2, 6, 7,
	2, 7, 3,
	4, 8, 9,
	4, 9, 5,
	6, 10, 11,
	6, 11, 7,
	8, 12, 13,
	8, 13, 9,
	9, 13, 14,
	9, 14, 10,
	10, 14, 15,
	10, 15, 11,

	// The two triangles in the center are kept last - then we can skip the
	// center by sending fewer triangles.
	5, 9, 10,
	5, 10, 6,
};

static const Color WHITE( 1.0f, 1.0f, 1.0f, 1.0f );

Tr2Sprite2dFrame::Tr2Sprite2dFrame( IRoot* lockobj ) : 
	m_cornerSize( 0 ),
	m_offset( 0 ),
	m_cachedWidth( 0.0f ), 
	m_cachedHeight( 0.0f ), 
	m_cachedCornerSize( 0 ),
	m_fillCenter( true )
{
}

Tr2Sprite2dFrame::~Tr2Sprite2dFrame()
{
	if( m_texture )
	{
		m_texture->UnregisterForChangeNotification( this );
	}
}

void Tr2Sprite2dFrame::GatherSprites( Tr2Sprite2dScene* renderer )
{
	if( !m_display )
	{
		return;
	}

	if( (m_spriteEffect == TR2_SFX_FILL) || !m_texture )
	{
		return;
	}
	
	if( m_isDirty )
	{
		renderer->SetSpriteEffect( m_spriteEffect );
		renderer->SetTileMode( 0 );

		if( !m_texture->IsGood() )
		{
			return;
		}

		m_texture->Apply( renderer, 0 );

		float srcWidth = m_texture->GetSrcWidth();
		float srcHeight = m_texture->GetSrcHeight();

		unsigned textureWidth = m_texture->GetWidth();
		unsigned textureHeight = m_texture->GetHeight();

		if( srcWidth == 0.0f )
		{
			srcWidth = (float)textureWidth;
		}
		if( srcHeight == 0.0f )
		{
			srcHeight = (float)textureHeight;
		}

		if( srcWidth < m_cornerSize*2 )
		{
			return;
		}

		if( srcHeight < m_cornerSize*2 )
		{
			return;
		}

		float cs = (float)m_cornerSize;

		float texHorizontalCs = cs / srcWidth;
		float texVerticalCs = cs / srcHeight;

		SetRegularRenderState( renderer );
		renderer->SetDepth( m_depth );

		const Color white( WHITE );

		float offset = (float)m_offset;
		float offset_2 = offset * 2.0f;

		float offsetWidth = m_displayWidth - offset_2;
		float offsetHeight = m_displayHeight - offset_2;

		m_cachedWidth = offsetWidth;
		m_cachedHeight = offsetHeight;
		m_cachedCornerSize = m_cornerSize;
		m_cachedTranslation = m_translation;

		float offsetX = offset + m_translation.x;
		float offsetY = offset + m_translation.y;

		Tr2Sprite2dVertexBase srcVertices[16];
		Tr2Sprite2dVertexBase* v = &srcVertices[0];
		//
		// First line
		//
		// Vertex 0
		v->position.x = offsetX;
		v->position.y = offsetY;
		v->position.z = 0.0f;
		v->texCoord[0].x = 0.0f;
		v->texCoord[0].y = 0.0f;
		v->color = white;
		++v;

		// Vertex 1
		v->position.x = cs + offsetX;
		v->position.y = offsetY;
		v->position.z = 0.0f;
		v->texCoord[0].x = texHorizontalCs;
		v->texCoord[0].y = 0.0f;
		v->color = white;
		++v;

		// Vertex 2
		v->position.x = offsetWidth - cs + offsetX;
		v->position.y = offsetY;
		v->position.z = 0.0f;
		v->texCoord[0].x = 1.0f - texHorizontalCs;
		v->texCoord[0].y = 0.0f;
		v->color = white;
		++v;

		// Vertex 3
		v->position.x = offsetWidth + offsetX;
		v->position.y = offsetY;
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
		v->position.y = cs + offsetY;
		v->position.z = 0.0f;
		v->texCoord[0].x = 0.0f;
		v->texCoord[0].y = texVerticalCs;
		v->color = white;
		++v;

		// Vertex 5
		v->position.x = cs + offsetX;
		v->position.y = cs + offsetY;
		v->position.z = 0.0f;
		v->texCoord[0].x = texHorizontalCs;
		v->texCoord[0].y = texVerticalCs;
		v->color = white;
		++v;

		// Vertex 6
		v->position.x = offsetWidth - cs + offsetX;
		v->position.y = cs + offsetY;
		v->position.z = 0.0f;
		v->texCoord[0].x = 1.0f - texHorizontalCs;
		v->texCoord[0].y = texVerticalCs;
		v->color = white;
		++v;

		// Vertex 7
		v->position.x = offsetWidth + offsetX;
		v->position.y = cs + offsetY;
		v->position.z = 0.0f;
		v->texCoord[0].x = 1.0f;
		v->texCoord[0].y = texVerticalCs;
		v->color = white;
		++v;

		//
		// Third line
		//
		// Vertex 8
		v->position.x = offsetX;
		v->position.y = offsetHeight - cs + offsetY;
		v->position.z = 0.0f;
		v->texCoord[0].x = 0.0f;
		v->texCoord[0].y = 1.0f - texVerticalCs;
		v->color = white;
		++v;

		// Vertex 9
		v->position.x = cs + offsetX;
		v->position.y = offsetHeight - cs + offsetY;
		v->position.z = 0.0f;
		v->texCoord[0].x = texHorizontalCs;
		v->texCoord[0].y = 1.0f - texVerticalCs;
		v->color = white;
		++v;

		// Vertex 10
		v->position.x = offsetWidth - cs + offsetX;
		v->position.y = offsetHeight - cs + offsetY;
		v->position.z = 0.0f;
		v->texCoord[0].x = 1.0f - texHorizontalCs;
		v->texCoord[0].y = 1.0f - texVerticalCs;
		v->color = white;
		++v;

		// Vertex 11
		v->position.x = offsetWidth + offsetX;
		v->position.y = offsetHeight - cs + offsetY;
		v->position.z = 0.0f;
		v->texCoord[0].x = 1.0f;
		v->texCoord[0].y = 1.0f - texVerticalCs;
		v->color = white;
		++v;

		//
		// Fourth line
		//
		// Vertex 12
		v->position.x = 0.0f + offsetX;
		v->position.y = offsetHeight + offsetY;
		v->position.z = 0.0f;
		v->texCoord[0].x = 0.0f;
		v->texCoord[0].y = 1.0f;
		v->color = white;
		++v;

		// Vertex 13
		v->position.x = cs + offsetX;
		v->position.y = offsetHeight + offsetY;
		v->position.z = 0.0f;
		v->texCoord[0].x = texHorizontalCs;
		v->texCoord[0].y = 1.0f;
		v->color = white;
		++v;

		// Vertex 14
		v->position.x = offsetWidth - cs + offsetX;
		v->position.y = offsetHeight + offsetY;
		v->position.z = 0.0f;
		v->texCoord[0].x = 1.0f - texHorizontalCs;
		v->texCoord[0].y = 1.0f;
		v->color = white;
		++v;

		// Vertex 15
		v->position.x = offsetWidth + offsetX;
		v->position.y = offsetHeight + offsetY;
		v->position.z = 0.0f;
		v->texCoord[0].x = 1.0f;
		v->texCoord[0].y = 1.0f;
		v->color = white;
		++v;

		renderer->PrepareTriangleVerts( &m_vertices[0], &srcVertices[0], sizeof( Tr2Sprite2dVertexBase ), 16 );

		m_isDirty = false;
	}

	unsigned int triangleCount = m_fillCenter ? 18 : 16;
	unsigned int indexCount = triangleCount * 3;
	
	m_texture->Apply( renderer, 0 );
	renderer->RenderTriangleVerts( &m_vertices[0], 16, &s_frameIndices[0], indexCount );
}

ITr2SpriteObject* Tr2Sprite2dFrame::PickPoint( float x, float y, Tr2Sprite2dScene* renderer )
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

Tr2Sprite2dTexture* Tr2Sprite2dFrame::GetTexture() const
{
	return m_texture;
}

void Tr2Sprite2dFrame::SetTexture( Tr2Sprite2dTexture* val )
{
	if( m_texture )
	{
		m_texture->UnregisterForChangeNotification( this );
	}
	m_texture = val;
	if( m_texture )
	{
		m_texture->RegisterForChangeNotification( this );
	}
}

void Tr2Sprite2dFrame::Sprite2dTextureChanged( ITr2Sprite2dTexture* p )
{
	SetDirty();
}

unsigned int Tr2Sprite2dFrame::GetVertexCount()
{
	return 16;
}
