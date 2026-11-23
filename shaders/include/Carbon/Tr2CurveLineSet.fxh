// Copyright © 2026 CCP ehf.

#ifndef CARBON_TR2CURVELINESET_FXH
#define CARBON_TR2CURVELINESET_FXH

// Data passed from the engine to the vertex shader stage for Tr2CurveLineSet draw calls.
struct Tr2CurveLineSetVertexInput
{
    // Vertex position in local space
    float3 position : POSITION;
    // XYZ - offset to next/previous line segment, W - width (incl. -1.f to indicate direction)
    float4 lineDir : TEXCOORD0; 
    // X - 0 or 1 for begin/end, Y - uniform scalar from 0 to 1 across length of line segment, Z - override color border, W - length of vert in uniform scalar
    float4 beginEnd : TEXCOORD1; 
    // X - animation speed, Y - animation scale, Z - lineID
    float3 animationData : TEXCOORD2; 
    // Offset to one after next/previous line segment
    float3 nextLineDir : TEXCOORD3; 
    // Line color
    float4 color : COLOR0; 
    // Override color
    float4 overrideColor : COLOR1;
    // Overlay color
    float4 overlayColor : COLOR2;
};

// Data passed from the engine to any shader stage for Tr2CurveLineSet draw calls.
struct Tr2CurveLineSetData
{
    float4x4 WorldMat;
};

#endif