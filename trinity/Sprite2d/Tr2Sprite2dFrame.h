// Copyright © 2023 CCP ehf.

#pragma once

#ifndef Tr2Sprite2dFrame_h
#define Tr2Sprite2dFrame_h


#include "Tr2SpriteObject.h"
#include "ITr2Sprite2dTexture.h"

BLUE_DECLARE( Tr2Sprite2dFrame );
BLUE_DECLARE( Tr2Sprite2dTexture );

//////////////////////////////////////////////////////////////////////////
// Tr2Sprite2dFrame implements the ITr2SpriteObject interface and is used with the
// Tr2Sprite2dScene to render 2D scenes.
//
// It renders a frame by cutting a texture in nine pieces and stretching them
// appropriately according to the dimensions of the frame.
//
// See http://carbon/wiki/Tr2Sprite2dFrame for more details.
//////////////////////////////////////////////////////////////////////////
class Tr2Sprite2dFrame : public Tr2TexturedSpriteObject
{
public:
	EXPOSE_TO_BLUE();
	Tr2Sprite2dFrame( IRoot* lockobj = NULL );

	//////////////////////////////////////////////////////////////////////////
	// ITr2SpriteObject
	unsigned int GetVertexCount();
	void GatherSprites( Tr2Sprite2dScene* renderer );
	ITr2SpriteObject* PickPoint( float x, float y, Tr2Sprite2dScene* renderer );

private:
	void PrepareVertices( Tr2Sprite2dVertexBase* v, float sourceWidth, float sourceHeight, const Vector2& vertOffset, const Vector2& scale ) const;

	// Resource path used to load a Tr2Sprite2dTexture stored in m_texture
	std::string m_resPath;

	unsigned int m_cornerSize;

	float m_cornerScale;

	// Offset the frame. Positive values will make it smaller, and negative bigger.
	int m_offset;

	// Saturation factor, used when TR2_SFX_COLOROVERLAY or TR2_SFX_SOFTLIGHT is used
	float m_saturation;

	// Opacity of effect - used when TR2_SFX_COLOROVERLAY or TR2_SFX_SOFTLIGHT is used
	float m_effectOpacity;

	// If true, the center piece of the texture is stretched to fill
	// the display extents of the frame. If false, the center is left
	// empty.
	bool m_fillCenter;

	// The buffer of vertices used to render the frame
	Tr2Sprite2dD3DVertex m_vertices[16 * 3];
};

TYPEDEF_BLUECLASS( Tr2Sprite2dFrame );

#endif //Tr2Sprite2dFrame_h
