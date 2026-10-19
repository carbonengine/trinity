// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVESPRITESET_FXH
#define CARBON_EVESPRITESET_FXH

// Data passed from the engine to the vertex shader stage for EveSpriteSet draw calls.
struct EveSpriteSetVertexInput
{
    // Sprite center position in world space
    float3 position : POSITION;
    // Sprite color
    float4 color : COLOR;
    // X - owner ship's activation strength, Y - sprite blink phase, Z - sprite blink rate, W - sprite minimum size
    float4 texcoord0 : TEXCOORD0;
    // X - sprite maximum size, Y - sprite falloff, Z, W - unused
    float2 texcoord1 : TEXCOORD1;
    // Warp mode color
    float4 warpColor : COLOR1;
    // Vertex index for sprite quad (0-3)
    float index : TEXCOORD5;
};

// EveSpriteSet uses the quad renderer for rendering sprites, so shaders do not have access to per-object data.


#endif