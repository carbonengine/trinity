// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2d.h"
#include "Tr2Sprite2dTexture.h"
#include "Tr2Sprite2dScene.h"
#include "Tr2Sprite2dPickingMask.h"


static unsigned short s_spriteIndices[18] = {
	0,
	1,
	3,
	3,
	1,
	2,
	4,
	5,
	7,
	7,
	5,
	6,
	8,
	9,
	11,
	11,
	9,
	10
};

Tr2Sprite2d::Tr2Sprite2d( IRoot* lockobj ) :
	m_pickRadius( 0.0f ),
	m_useSizeFromTexture( false ),
	m_firstVertex( 0 ),
	m_vertexCount( 0 ),
	m_saturation( 1.0f ),
	m_effectOpacity( 1.0f )
{
	std::fill( std::begin( m_vertices ), std::end( m_vertices ), Tr2Sprite2dD3DVertex() );
}

Tr2Sprite2d::~Tr2Sprite2d()
{
	if( m_texturePrimary )
	{
		m_texturePrimary->UnregisterForChangeNotification( this );
	}
	if( m_textureSecondary )
	{
		m_textureSecondary->UnregisterForChangeNotification( this );
	}
}

void Tr2Sprite2d::GatherSprites( Tr2Sprite2dScene* renderer )
{
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

		// Assume we're only rendering the regular sprite - these will be adjusted
		// if there is a shadow or a glow
		m_firstVertex = 4;
		m_vertexCount = 4;

		renderer->SetSpriteEffect( m_spriteEffect );

		float displayWidth = m_displayWidth;
		float displayHeight = m_displayHeight;
		if( m_useSizeFromTexture )
		{
			displayWidth = (float)m_texturePrimary->GetSrcWidth();
			if( displayWidth == 0.0f )
			{
				displayWidth = (float)m_texturePrimary->GetWidth();
			}
			displayHeight = (float)m_texturePrimary->GetSrcHeight();
			if( displayHeight == 0.0f )
			{
				displayHeight = (float)m_texturePrimary->GetHeight();
			}
		}

		if( m_shadowOffset.x != 0.0f || m_shadowOffset.y != 0.0f )
		{
			m_firstVertex = 0;
			m_vertexCount += 4;
			SetShadowRenderState( renderer );
			Vector2 offsetTranslation = m_translation + m_shadowOffset;
			renderer->PrepareSpriteVerts( &m_vertices[0], offsetTranslation, displayWidth, displayHeight, m_spriteEffect );
		}

		SetRegularRenderState( renderer );

		if( m_spriteEffect == TR2_SFX_OUTLINE )
		{
			SetOutlineRenderState( renderer );
		}

		renderer->PrepareSpriteVerts( &m_vertices[4], m_translation, displayWidth, displayHeight, m_spriteEffect );

		if( m_spriteEffect < TR2_SFX_TWO_TEXTURES && ( m_spriteEffect != TR2_SFX_BLUR ) && ( m_spriteEffect != TR2_SFX_GLOW ) && ( m_spriteEffect != TR2_SFX_OUTLINE ) )
		{
			for( int i = 4; i < 8; ++i )
			{
				m_vertices[i].texCoord[1].x = m_saturation;
				m_vertices[i].texCoord[1].y = m_effectOpacity;
			}
		}

		if( ( m_glowFactor != 0.0f ) || ( m_glowExpand != 0.0f ) )
		{
			m_vertexCount += 4;
			SetGlowRenderState( renderer );

			Vector2 pos = m_translation;
			float offset = m_glowExpand;
			float offset2 = offset + offset;
			pos.x -= offset;
			pos.y -= offset;

			float width = displayWidth + offset2;
			float height = displayHeight + offset2;

			renderer->PrepareSpriteVerts( &m_vertices[8], pos, width, height, m_spriteEffect );
		}

		m_isDirty = false;
	}

	SetValidatedTextures( renderer );

	int indexCount = ( m_vertexCount / 4 ) * 6;
	renderer->RenderTriangleVerts( &m_vertices[m_firstVertex], m_vertexCount, s_spriteIndices, indexCount );
}

ITr2SpriteObject* Tr2Sprite2d::PickPoint( float x, float y, Tr2Sprite2dScene* renderer )
{
	if( !m_display )
	{
		return NULL;
	}

	if( m_pickState == TR2_SPS_ON )
	{
		if( renderer->IsInside( Vector2( x, y ), m_translation, m_displayWidth, m_displayHeight, m_pickRadius ) )
		{
			if( !m_pickingMask || m_pickingMask->SampleMask( renderer->InverseTransformPoint( Vector2( x, y ) ), m_translation, m_displayWidth, m_displayHeight ) )
			{
				return this;
			}
		}
	}

	return NULL;
}

unsigned int Tr2Sprite2d::GetVertexCount()
{
	if( !m_display )
	{
		return 0;
	}

	if( m_isDirty )
	{
		unsigned int vertexCount = 4;
		if( m_shadowOffset.x != 0.0f || m_shadowOffset.y != 0.0f )
		{
			vertexCount += 4;
		}
		if( ( m_glowFactor != 0.0f ) || ( m_glowExpand != 0.0f ) )
		{
			vertexCount += 4;
		}

		return vertexCount;
	}
	else
	{
		return m_vertexCount;
	}
}
