// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVESTRETCH2_FXH
#define CARBON_EVESTRETCH2_FXH


// Data passed from the engine to the all shader stage for EveStretch2 draw calls.
struct EveStretch2PerObjectData
{
    // XYZ - stretch source position in world space, W - scale factor for the destination point or 0 if the destination effect is hidden
    float4 SourcePosition;
    // XYZ - stretch destination position in world space, W - scale factor for the destination point
    float4 DestPosition;
    // X - time from the beginning of the "start" phase, Y - time from the beginning of the firing loop, Z - time from the beginning of the "end" phase, W - random value from 0 to 1
    float4 Timing;
    // X - effect intensity, YZW - unused
    float4 Misc;
};


// Data passed from the engine to the vertex shader stage for EveStretch2 draw calls.
struct EveStretch2Vertex
{
    // X - index of the quad, Y - index of the vertex in the quad
    float2 indices : POSITION;
};


#endif