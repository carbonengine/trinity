// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVESPOTLIGHTSET_FXH
#define CARBON_EVESPOTLIGHTSET_FXH

// Data passed from the engine to the vertex shader stage for EveSpotlightSet cone geometry draw calls.
struct EveSpotlightSetConeVertexInput
{
    // Spotlight color and activation state (w - ship activation)
    float4 color : COLOR0;
    // 3x4 matrix that transforms the spotlight from local instance space to world space
    float4 transform1 : TEXCOORD0;
    float4 transform2 : TEXCOORD1;
    float4 transform3 : TEXCOORD2;
    // X - booster gain, Y - unused
    float2 boosterGain : TEXCOORD3;
    // Vertex index of the spotlight cone (0-3)
    float index : TEXCOORD4;
};


// Data passed from the engine to the vertex shader stage for EveSpotlightSet glow sprite draw calls.
struct EveSpotlightSetGlowVertexInput
{
    // 3x4 matrix that transforms the spotlight from local instance space to world space
    float4 transform1 : TEXCOORD0;
    float4 transform2 : TEXCOORD1;
    float4 transform3 : TEXCOORD2;
    // Spotlight sprite color and activation state (w - ship activation)
    float4 spriteColor : COLOR0;
    // Spotlight flare color (w - unused)
    float4 flareColor : COLOR1;
    // Sprite scale (XYZ) and boosterGain influence (W)
    float4 scale : TEXCOORD3;
    // Vertex index of the spotlight cone (0-3)
    float index : TEXCOORD4;
};


#endif