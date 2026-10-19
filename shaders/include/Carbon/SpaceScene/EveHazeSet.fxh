// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVEHAZESET_FXH
#define CARBON_EVEHAZESET_FXH


// Data passed from the engine to the vertex shader stage for EveHazeSet draw calls.
struct EveHazeSetVertexInput
{
    // 3x4 matrix that transforms the haze from local instance space to object space
    float4 transform1 : TEXCOORD0;
    float4 transform2 : TEXCOORD1;
    float4 transform3 : TEXCOORD2;
    // 3x4 matrix that transforms the haze from object space to local instance space
    float4 invTransform1 : TEXCOORD3;
    float4 invTransform2 : TEXCOORD4;
    float4 invTransform3 : TEXCOORD5;
    // X - haze falloff, Y - source size, Z - source brightness, W - saturation
    float4 hazeData : TEXCOORD6;
    // Color of the haze
    float4 color : COLOR;
    // X - vertex index (0-7), Y - skeleton bone index if the haze is attached to the bone
    uint4 index : TEXCOORD7;
};

// EveHazeSet uses the same per-object data layout as other space objects. See PerObjectData.fxh

#endif