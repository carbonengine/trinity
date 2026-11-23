// Copyright © 2026 CCP ehf.

#pragma once
#ifndef Tr2Sprite2dEffect_h
#define Tr2Sprite2dEffect_h

#include "ITr2SpriteObject.h"
#include "Tr2SpriteObject.h"

BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( Tr2Sprite2dEffect );

// A leaf sprite that draws one effect over its rect, in order with the other
// sprites, as a draw call of its own. The effect draws the sprite's quad in
// place of the scene's own shaders, so it gets the same vertices and
// constants (UIVertexInput, g_uiTransforms and UIViewportSize in
// Carbon/UI/UI.fxh), clips and transforms them as they do, and should write
// glow to SV_Target1 like them. Texcoord 0 runs 0 to 1 over the quad, texcoord
// 1 is the quad's width and height, and the vertex color is the sprite's color
// with its ancestors' opacity multiplied into alpha. The glow brightness and
// sprite target are the sprite's own, and the other vertex fields hold a
// default sprite's values, whatever was drawn before. The effect gets none of
// the scene's textures.
class Tr2Sprite2dEffect : public Tr2SpriteObjectBase
{
public:
	EXPOSE_TO_BLUE();

	Tr2Sprite2dEffect( IRoot* lockobj = NULL );

	//////////////////////////////////////////////////////////////////////////
	// ITr2SpriteObject
	unsigned int GetVertexCount();
	void GatherSprites( Tr2Sprite2dScene* renderer ) override;
	ITr2SpriteObject* PickPoint( float x, float y, Tr2Sprite2dScene* renderer ) override;

private:
	Tr2EffectPtr m_effect;
	Color m_color;
	float m_glowBrightness;
	Tr2SpriteTarget m_target;
};

TYPEDEF_BLUECLASS( Tr2Sprite2dEffect );

#endif
