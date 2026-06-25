// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dLine.h"
#include "Tr2Sprite2dTexture.h"
#include "Tr2Sprite2dScene.h"

static unsigned short s_spriteIndices[6] = {
	0,
	1,
	3,
	3,
	1,
	2
};

Tr2Sprite2dLine::Tr2Sprite2dLine( IRoot* lockobj /*= NULL */ ) :
	m_translationFrom( 0.0f, 0.0f ),
	m_translationTo( 0.0f, 0.0f ),
	m_widthFrom( 0.0f ),
	m_widthTo( 0.0f ),
	m_colorFrom( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_colorTo( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_textureWidth( 1.0 ),
	m_textureOffsetBase( 0.0 ),
	m_textureOffset( 0.0 )
{
	std::fill( std::begin( m_vertices ), std::end( m_vertices ), Tr2Sprite2dD3DVertex() );
}

void Tr2Sprite2dLine::GatherSprites( Tr2Sprite2dScene* renderer )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_display || ( m_spriteEffect == TR2_SFX_NONE ) )
	{
		return;
	}

	if( m_isDirty )
	{
		if( !ValidateAndSetTextures( renderer ) )
		{
			return;
		}

		renderer->SetSpriteEffect( m_spriteEffect );
		renderer->SetTileMode( S2D_TS_TILE_X | S2D_TS_TILE_Y );

		SetRegularRenderState( renderer );

		Vector2 from = m_translationFrom + m_translation;
		Vector2 to = m_translationTo + m_translation;

		Vector2 d = m_translationTo - m_translationFrom;
		float segmentLength = Length( d );
		d = Normalize( d );

		// Rotate 90 degrees
		Vector2 normal;
		normal.x = d.y;
		normal.y = -d.x;

		float halfWidthFrom = 0.5f * m_widthFrom;
		float halfWidthTo = 0.5f * m_widthTo;

		// Anti-aliased lines are rendered with a quad that is larger. This is then
		// compensated for in the pixel shader, using the extra pixels to fill in
		// alpha values to do the anti-aliasing

		bool isAA = m_spriteEffect == TR2_SFX_FILL_AA;
		float pixelWidthFromInTexels;
		float pixelWidthToInTexels;
		if( isAA )
		{
			halfWidthFrom += 2.0f;
			halfWidthTo += 2.0f;

			pixelWidthFromInTexels = 1.0f / ( m_widthFrom + 4.0f );
			pixelWidthToInTexels = 1.0f / ( m_widthTo + 4.0f );
		}

		float texOffset1 = m_textureOffsetBase - m_textureOffset;
		float texOffset2 = m_textureOffsetBase + segmentLength / m_textureWidth - m_textureOffset;

		Tr2Sprite2dVertexBase verts[4];

		Tr2Sprite2dVertexBase& v0 = verts[0];
		v0.position.x = from.x - normal.x * halfWidthFrom;
		v0.position.y = from.y - normal.y * halfWidthFrom;
		v0.position.z = m_depth;
		v0.color = m_colorFrom;
		v0.texCoord[0] = Vector2( texOffset1, 0.0f );

		if( isAA )
		{
			v0.texCoord[1] = Vector2( -pixelWidthFromInTexels, m_widthFrom );
		}
		else
		{
			v0.texCoord[1] = Vector2( texOffset1, 0.0f );
		}

		Tr2Sprite2dVertexBase& v1 = verts[1];
		v1.position.x = from.x + normal.x * halfWidthFrom;
		v1.position.y = from.y + normal.y * halfWidthFrom;
		v1.position.z = m_depth;
		v1.color = m_colorFrom;
		v1.texCoord[0] = Vector2( texOffset1, 1.0f );
		if( isAA )
		{
			v1.texCoord[1] = Vector2( 1.0f + pixelWidthFromInTexels, m_widthFrom );
		}
		else
		{
			v1.texCoord[1] = Vector2( texOffset1, 1.0f );
		}

		Tr2Sprite2dVertexBase& v2 = verts[2];
		v2.position.x = to.x + normal.x * halfWidthTo;
		v2.position.y = to.y + normal.y * halfWidthTo;
		v2.position.z = m_depth;
		v2.color = m_colorTo;
		v2.texCoord[0] = Vector2( texOffset2, 1.0f );
		if( isAA )
		{
			v2.texCoord[1] = Vector2( 1.0f + pixelWidthToInTexels, m_widthTo );
		}
		else
		{
			v2.texCoord[1] = Vector2( texOffset2, 1.0f );
		}

		Tr2Sprite2dVertexBase& v3 = verts[3];
		v3.position.x = to.x - normal.x * halfWidthTo;
		v3.position.y = to.y - normal.y * halfWidthTo;
		v3.position.z = m_depth;
		v3.color = m_colorTo;
		v3.texCoord[0] = Vector2( texOffset2, 0.0f );
		if( isAA )
		{
			v3.texCoord[1] = Vector2( -pixelWidthToInTexels, m_widthTo );
		}
		else
		{
			v3.texCoord[1] = Vector2( texOffset2, 0.0f );
		}

		renderer->PrepareTriangleVerts( &m_vertices[0], verts, sizeof( Tr2Sprite2dVertexBase ), 4 );

		m_isDirty = false;
	}

	SetValidatedTextures( renderer );

	renderer->RenderTriangleVerts( &m_vertices[0], 4, s_spriteIndices, 6 );
}

ITr2SpriteObject* Tr2Sprite2dLine::PickPoint( float x, float y, Tr2Sprite2dScene* renderer )
{
	// TODO: Implement
	return NULL;
}

unsigned int Tr2Sprite2dLine::GetVertexCount()
{
	return 4;
}
