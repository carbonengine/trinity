// Copyright © 2023 CCP ehf.

#pragma once

#ifndef Tr2Sprite2dStretch_h
#define Tr2Sprite2dStretch_h


#include "Tr2SpriteObject.h"

BLUE_DECLARE( Tr2Sprite2dStretch );
BLUE_DECLARE( Tr2Sprite2dTexture );

enum Tr2StretchScaleBehavior
{
	S2D_SSC_ALIGN_BOTTOMRIGHT,
	S2D_SSC_ALIGN_TOPLEFT,
	S2D_SSC_SCALE
};

//////////////////////////////////////////////////////////////////////////
// Tr2Sprite2dFrame implements the ITr2SpriteObject interface and is used with the
// Tr2Sprite2dScene to render 2D scenes.
//
// It renders a sprite by cutting a texture into three pieces and stretching the
// center piece appropriately according to the dimensions of the sprite.
//////////////////////////////////////////////////////////////////////////
class Tr2Sprite2dStretch : public Tr2TexturedSpriteObject
{
public:
	EXPOSE_TO_BLUE();
	Tr2Sprite2dStretch( IRoot* lockobj = NULL );

	//////////////////////////////////////////////////////////////////////////
	// ITr2SpriteObject
	unsigned int GetVertexCount();
	void GatherSprites( Tr2Sprite2dScene* renderer );
	ITr2SpriteObject* PickPoint( float x, float y, Tr2Sprite2dScene* renderer );

private:
	void PrepareVertices( Tr2Sprite2dVertexBase* v, float sourceWidth, const Vector2& vertOffset, const Vector2& scale ) const;

	// Resource path used to load a Tr2Sprite2dTexture stored in m_texture
	std::string m_resPath;

	// Number of pixels on the left edge of the texture
	unsigned int m_leftEdgeSize;

	// Number of pixels on the right edge of the texture
	unsigned int m_rightEdgeSize;

	// Scaling for edges when rendering (for UI scaling)
	float m_edgeScale;

	// Saturation factor, used when TR2_SFX_COLOROVERLAY or TR2_SFX_SOFTLIGHT is used
	float m_saturation;

	// Opacity of effect - used when TR2_SFX_COLOROVERLAY or TR2_SFX_SOFTLIGHT is used
	float m_effectOpacity;

	// Offset the sprite. Positive values will make it smaller horizontally,
	// and negative bigger. The sprite is shifted vertically by this offset.
	int m_offset;

	// If true, the center piece of the texture is stretched to fill
	// the display width of the stretch sprite.
	bool m_fillCenter;

	Tr2StretchScaleBehavior m_dpiScaleBehavior;

	// The buffer of vertices used to render the sprite
	Tr2Sprite2dD3DVertex m_vertices[8 * 3];
};

TYPEDEF_BLUECLASS( Tr2Sprite2dStretch );

#endif //Tr2Sprite2dFrame_h
