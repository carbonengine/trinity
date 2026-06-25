// Copyright © 2023 CCP ehf.

#pragma once

#ifndef ITr2Sprite2dRenderer_h
#define ITr2Sprite2dRenderer_h

enum Tr2SpriteObjectPickState
{
	TR2_SPS_OFF,
	TR2_SPS_ON,
	TR2_SPS_CHILDREN
};

enum Tr2SpriteObjectBlendMode
{
	TR2_SBM_NONE,
	TR2_SBM_BLEND,
	TR2_SBM_ADD,
	TR2_SBM_ADDX2
};

enum class Tr2SpriteTarget
{
	COLOR = 1,
	GLOW = 2,
	COLOR_AND_GLOW = 3,
};

// Note that this enum must match constants in PixelShader.fxh
enum Tr2SpriteObjectEffect
{
	TR2_SFX_NO_TEXTURE,
	TR2_SFX_FILL = TR2_SFX_NO_TEXTURE, // No texture - only vertex colors

	TR2_SFX_FILL_AA, // Anti-aliased lines

	TR2_SFX_ONE_TEXTURE = 32,
	TR2_SFX_COPY = TR2_SFX_ONE_TEXTURE, // Primary texture modulated with vertex colors
	TR2_SFX_DOT, // Primary texture used in dot product against light vector
	TR2_SFX_NOALPHA, // Primary texture as a Bink video texture
	TR2_SFX_DROPSHADOW, // Primary texture modulated with vertex colors plus a drop shadow
	TR2_SFX_OUTLINE, // Primary texture modulated with vertex colors plus an outline
	TR2_SFX_COLOROVERLAY, // Primary texture colorized with overlay effect (http://en.wikipedia.org/wiki/Blend_modes#Overlay)
	TR2_SFX_SOFTLIGHT, // Primary texture colorized with a soft light effect (http://en.wikipedia.org/wiki/Blend_modes#Soft_Light)
	TR2_SFX_BLUR, // Primary texture is blurred
	TR2_SFX_BLURBACKGROUNDCOLORED, // Primary texture is blurred, uvs are calculated from screen coords
	TR2_SFX_BLURBACKGROUND, // Primary texture is blurred, uvs are calculated from screen coords
	TR2_SFX_GLOW, // Primary texture is blurred, to be used for glow effects
	TR2_SFX_FONT, // Primary texture is a glyph rendered by FreeType

	TR2_SFX_TWO_TEXTURES = 64,
	TR2_SFX_MODULATE = TR2_SFX_TWO_TEXTURES, // Primary and secondary textures modulated, then again with vertex colors
	TR2_SFX_MASK, // Primary texture with secondary texture alpha as mask
	TR2_SFX_COPY_DOT,

	TR2_SFX_COUNT,

	TR2_SFX_NONE = 0xffffffff
};

struct Tr2Sprite2dClipRect
{
	Tr2Sprite2dClipRect()
	{
	}
	Tr2Sprite2dClipRect( float l, float t, float r, float b ) :
		left( l ), top( t ), right( r ), bottom( b )
	{
	}

	float left;
	float top;
	float right;
	float bottom;
};

struct Tr2Sprite2dVertexBase
{
	Vector3 position;
	Color color;
	Vector2 texCoord[2];
};

struct Tr2Sprite2dD3DVertex : public Tr2Sprite2dVertexBase
{
	Tr2Sprite2dClipRect clipRect;
	float glowBrightness;
	uint8_t transformIndex;
	uint8_t blendMode;
	uint8_t spriteEffect;
	uint8_t tileMode;
	Color outlineColor;
	float outlineThreshold;
};

BLUE_DECLARE( Tr2AtlasTexture );

enum Tr2Sprite2dRenderEffect
{
	S2D_RFX_COPY,
	S2D_RFX_BLUR,
	S2D_RFX_MODULATE
};

enum Tr2Sprite2dTextureSettings
{
	S2D_TS_NONE = 0x00,
	S2D_TS_FORCEOPAQUE = 0x01,
	S2D_TS_REPEAT_MIRROR = 0x02,
	S2D_TS_REPEAT_CLAMP = 0x04,
	S2D_TS_TILE_X = 0x08,
	S2D_TS_TILE_Y = 0x10,
};

extern Be::VarChooser Tr2Sprite2dRenderEffectChooser[];

class TriRenderJob;

inline uint8_t PackBlendMode( Tr2SpriteObjectBlendMode blendMode, Tr2SpriteTarget target )
{
	return blendMode | ( uint8_t( target ) << 3 );
}

#endif
