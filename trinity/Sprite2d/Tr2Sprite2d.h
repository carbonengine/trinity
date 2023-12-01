#pragma once

#ifndef Tr2Sprite2d_h
#define Tr2Sprite2d_h


#include "Tr2SpriteObject.h"
#include "ITr2Sprite2dTexture.h"

BLUE_DECLARE( Tr2Sprite2d );
BLUE_DECLARE( Tr2Sprite2dTexture );
BLUE_DECLARE_INTERFACE( ITr2Sprite2dTexture );

class Tr2Sprite2d:
     public Tr2TexturedSpriteObject
{
public:
    EXPOSE_TO_BLUE();
    Tr2Sprite2d( IRoot* lockobj = NULL );
	~Tr2Sprite2d();

	//////////////////////////////////////////////////////////////////////////
	// ITr2SpriteObject
	unsigned int GetVertexCount();
	void GatherSprites( Tr2Sprite2dScene* renderer );

	ITr2SpriteObject* PickPoint( float x, float y, Tr2Sprite2dScene* renderer );

private:
	float m_pickRadius;

	// If set, then displayWidth/Height is ignored and size of texture
	// is used instead.
	bool m_useSizeFromTexture;

	// One sprite needs 4 vertices, but it can also be drawn with a shadow
	// and a glow, which are essentially sprites in their own right.
	// So we keep space for 12 vertices - shadow gets the first 4, the regular
	// sprite the next 4 and the glow the final 4.
	// The vertex data is rebuilt whenever the sprite is dirty.
	Tr2Sprite2dD3DVertex m_vertices[12];

	// Where to start - if there's a shadow we start at the first vertex (0),
	// otherwise it's where the regular sprite starts (4)
	unsigned int m_firstVertex;

	// How many vertices - at least 4 for the regular sprite, plus 4 if there's
	// a shadow, plus 4 if there's a glow.
	unsigned int m_vertexCount;

	// Saturation factor, used when TR2_SFX_COLOROVERLAY or TR2_SFX_SOFTLIGHT is used
	float m_saturation;

	// Opacity of effect - used when TR2_SFX_COLOROVERLAY or TR2_SFX_SOFTLIGHT is used
	float m_effectOpacity;
};

TYPEDEF_BLUECLASS( Tr2Sprite2d );
#endif //Tr2Sprite2d_h
