// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVEPLANESET_FXH
#define CARBON_EVEPLANESET_FXH


// Data passed from the engine to the vertex shader stage for EvePlaneSet draw calls.
struct EvePlaneSetVertexInput
{
    // 3x4 matrix that transforms the plane from local instance space to world space
    float4 transform1 : TEXCOORD0;
    float4 transform2 : TEXCOORD1;
    float4 transform3 : TEXCOORD2;
    // Color of the plane
    float4 color : COLOR;
    // UV layer 1 transform and scroll data (XY - scale, ZW - offset)
    float4 layer1Transform : TEXCOORD3;
    // UV layer 2 transform and scroll data (XY - scale, ZW - offset)
    float4 layer2Transform : TEXCOORD4;
    // UV layer 1 scroll data (XY - scroll speed, ZW - scroll offset)
    float4 layer1Scroll : TEXCOORD5;
    // UV layer 2 scroll data (XY - scroll speed, ZW - scroll offset)
    float4 layer2Scroll : TEXCOORD6;
    // Z - texture atlas index
    uint4 index : TEXCOORD7;
    // Plane blink data coming from SOF (X - blink rate, Y - blink phase, Z - duty cycle, W - blink mode)
    float4 blinkData : TEXCOORD8;
};

// EvePlaneSet uses the quad renderer for rendering sprites, so shaders do not have access to per-object data.

#endif