#pragma once

#ifndef Tr2Sprite2dStretch_h
#define Tr2Sprite2dStretch_h


#include "Tr2SpriteObject.h"

BLUE_DECLARE( Tr2Sprite2dStretchVertical );
BLUE_DECLARE( Tr2Sprite2dTexture );

//////////////////////////////////////////////////////////////////////////
// Tr2Sprite2dFrame implements the ITr2SpriteObject interface and is used with the 
// Tr2Sprite2dScene to render 2D scenes.
//
// It renders a sprite by cutting a texture into three pieces and stretching the 
// center piece appropriately according to the dimensions of the sprite.
//////////////////////////////////////////////////////////////////////////
class Tr2Sprite2dStretchVertical:
	public Tr2TexturedSpriteObject
{
public:
	EXPOSE_TO_BLUE();
	Tr2Sprite2dStretchVertical( IRoot* lockobj = NULL );

	//////////////////////////////////////////////////////////////////////////
	// ITr2SpriteObject
	unsigned int GetVertexCount();
	void GatherSprites( Tr2Sprite2dScene* renderer );
	ITr2SpriteObject* PickPoint( float x, float y, Tr2Sprite2dScene* renderer );

private:
	void PrepareVertices( Tr2Sprite2dVertexBase* v, float sourceHeight, const Vector2& vertOffset, const Vector2& scale ) const;

	// Resource path used to load a Tr2Sprite2dTexture stored in m_texture
	std::string m_resPath;

	// Number of pixels on the top edge of the texture
	unsigned int m_topEdgeSize;

	// Number of pixels on the bottom edge of the texture
	unsigned int m_bottomEdgeSize;

	// Scaling for edges when rendering (for UI scaling)
	float m_edgeScale;

	// Saturation factor, used when TR2_SFX_COLOROVERLAY or TR2_SFX_SOFTLIGHT is used
	float m_saturation;

	// Opacity of effect - used when TR2_SFX_COLOROVERLAY or TR2_SFX_SOFTLIGHT is used
	float m_effectOpacity;

	// Offset the sprite. Positive values will make it smaller vertically,
	// and negative bigger. The sprite is shifted horizontally by this offset.
	int m_offset;

	// If true, the center piece of the texture is stretched to fill
	// the display width of the stretch sprite.
	bool m_fillCenter;

	// The buffer of vertices used to render the sprite
	Tr2Sprite2dD3DVertex m_vertices[8 * 3];
};

TYPEDEF_BLUECLASS( Tr2Sprite2dStretchVertical );

#endif //Tr2Sprite2dFrame_h
