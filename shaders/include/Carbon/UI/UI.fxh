// Copyright © 2026 CCP ehf.

#ifndef CARBON_UI_FXH
#define CARBON_UI_FXH


// The following constants mirror the Tr2SpriteObjectBlendMode enum in C++
static const uint TR2_SBM_NONE = 0;
static const uint TR2_SBM_BLEND = 1;
static const uint TR2_SBM_ADD = 2;
static const uint TR2_SBM_ADDX2 = 3;

static const uint S2D_TS_TILE_X = 8;
static const uint S2D_TS_TILE_Y = 16;

static const uint S2D_RT_COLOR = 1 << 3;
static const uint S2D_RT_GLOW = 1 << 4;


// The following constants mirror the Tr2SpriteObjectEffect enum in C++
static const uint TR2_SFX_FILL = 0;
static const uint TR2_SFX_FILL_AA = 1;

static const uint TR2_SFX_COPY = 32;
static const uint TR2_SFX_DOT = 33;
static const uint TR2_SFX_NOALPHA = 34;
static const uint TR2_SFX_DROPSHADOW = 35;
static const uint TR2_SFX_OUTLINE = 36;
static const uint TR2_SFX_COLOROVERLAY = 37;
static const uint TR2_SFX_SOFTLIGHT = 38;
static const uint TR2_SFX_BLUR = 39;
static const uint TR2_SFX_BLURBACKGROUNDCOLORED = 40;
static const uint TR2_SFX_BLURBACKGROUND = 41;
static const uint TR2_SFX_GLOW = 42;
static const uint TR2_SFX_FONT = 43;
static const uint TR2_SFX_NOISEFADE = 44;

static const uint TR2_SFX_MODULATE = 64;
static const uint TR2_SFX_MASK = 65;

#define TR2_SS_MAX_TRANSFORM_COUNT 32

// x, y = 1/width, 1/height of the containing atlas texture, always
// z, w = 0.5/width, 0.5/height of the atlas texture: the shift to apply to TEX2D to sample the texel center.
float4 g_texelSizeUI0;
float4 g_texelSizeUI1;

// x, y = width, height of the viewport in pixels
float4 UIViewportSize;

// UI transform stack
float4x4 g_uiTransforms[TR2_SS_MAX_TRANSFORM_COUNT] : register( c5 );

// Vertex as it is passed from the application to the vertex shader
struct UIVertexInput
{
    // Vertex position
    float3 pos : POSITION;
    // Sprite color
    float4 color : COLOR0;
    // Outline color
    float4 outlineColor : COLOR1;
    // Texture coordinates for the primary texture
    float2 tex0 : TEXCOORD0;
    // Texture coordinates for the secondary texture
    float2 tex1 : TEXCOORD1;
    // Clipping rectangle for the sprite (left, top, right, bottom)
    float4 clipRect : TEXCOORD2;
    // Glow brightness for the sprite
    float glowBrightness : TEXCOORD3;
    // Threshold value for the opacity of the outline effect
    float outlineThreshold : TEXCOORD4;
    
    // data.x : transform index
    // data.y : blend mode
    // data.z : sprite effect (0-2 bits), glow bit (3 bit)
	// data.w : tile mode
    uint4 data : BLENDINDICES;
};


#endif