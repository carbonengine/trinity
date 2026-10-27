// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dEffect.h"
#include "Tr2Sprite2dScene.h"
#include "Tr2Sprite2dPickingMask.h"
#include "Shader/Tr2Effect.h"

static unsigned short s_quadIndices[6] = { 0, 1, 3, 3, 1, 2 };

Tr2Sprite2dEffect::Tr2Sprite2dEffect( IRoot* lockobj /*= NULL */ ) :
	m_color( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_glowBrightness( 1.0f ),
	m_target( Tr2SpriteTarget::COLOR )
{
}

Tr2Sprite2dEffect::~Tr2Sprite2dEffect()
{
}

void Tr2Sprite2dEffect::GatherSprites( Tr2Sprite2dScene* renderer )
{
	TRINITY_STATS_ZONE( __FUNCTION__ );

	if( !m_display || !m_effect )
	{
		return;
	}

	static const Vector2 s_corners[4] = {
		Vector2( 0.0f, 0.0f ),
		Vector2( 1.0f, 0.0f ),
		Vector2( 1.0f, 1.0f ),
		Vector2( 0.0f, 1.0f )
	};

	// Every field is set here, from the sprite or as a default Tr2SpriteObject
	// would set it, so the effect gets the same vertices whatever the sprites
	// before it left
	renderer->SetDepth( 0.0f );

	Tr2Sprite2dD3DVertex vertices[4];
	for( int i = 0; i < 4; ++i )
	{
		Tr2Sprite2dD3DVertex& vertex = vertices[i];
		vertex.position = Vector3( m_translation.x + s_corners[i].x * m_displayWidth, m_translation.y + s_corners[i].y * m_displayHeight, 0.0f );
		vertex.color = m_color;
		vertex.texCoord[0] = s_corners[i];
		vertex.texCoord[1] = Vector2( m_displayWidth, m_displayHeight );
		vertex.glowBrightness = m_glowBrightness;
		vertex.transformIndex = 0;
		vertex.blendMode = PackBlendMode( TR2_SBM_NONE, m_target );
		vertex.spriteEffect = TR2_SFX_FILL;
		vertex.tileMode = S2D_TS_NONE;
		vertex.outlineColor = Color( 0.0f, 0.0f, 0.0f, 1.0f );
		vertex.outlineThreshold = 0.0f;
	}

	renderer->RenderTriangleVerts( *m_effect, vertices, 4, s_quadIndices, 6 );
}

ITr2SpriteObject* Tr2Sprite2dEffect::PickPoint( float x, float y, Tr2Sprite2dScene* renderer )
{
	if( !m_display )
	{
		return NULL;
	}

	if( m_pickState == TR2_SPS_ON )
	{
		if( renderer->IsInside( Vector2( x, y ), m_translation, m_displayWidth, m_displayHeight, 0.0f ) )
		{
			if( !m_pickingMask || m_pickingMask->SampleMask( renderer->InverseTransformPoint( Vector2( x, y ) ), m_translation, m_displayWidth, m_displayHeight ) )
			{
				return this;
			}
		}
	}

	return NULL;
}

unsigned int Tr2Sprite2dEffect::GetVertexCount()
{
	return ( m_display && m_effect ) ? 4 : 0;
}
