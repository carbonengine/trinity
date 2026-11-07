// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVETACTICALOVERLAY_FXH
#define CARBON_EVETACTICALOVERLAY_FXH


// Data passed from the engine to the vertex shader stage for EveTacticalOverlay sphere connector draw calls.
struct EveTacticalOverlaySphereConnectorVertexInput
{
    // XYZ - connector position in world space, W - step count and step index
    float4 instanceData : TEXCOORD0;
    // Object radius in integer part, intensity in fractional part
    float instanceData2 : TEXCOORD1;
    // Quad vertex index (0-3)
    float index : TEXCOORD5;
};


// Data passed from the engine to the vertex shader stage for EveTacticalOverlay anchor draw calls.
struct EveTacticalOverlayAnchorVertexInput
{
    // XYZ - anchor position in world space, W - effect intensity
    float4 instanceData : TEXCOORD0;
    // Quad vertex index (0-3)
    float index : TEXCOORD5;
};


// Data passed from the engine to the vertex shader stage for EveTacticalOverlay velocity connector draw calls.
struct EveTacticalOverlayVelocityConnectorVertexInput
{
    // XYZ - connector position in world space, W - quad index (0 - velocity, 1 - approach amount, 2 - relative approach amount)
    float4 instanceData : TEXCOORD0;
    // XYZ - velocity in world space, W - radius in integer part, blink phase in fractional part
    float4 instanceData2 : TEXCOORD1;
    // Quad vertex index (0-3)
    float index : TEXCOORD5;
};

// Tactical overlay plane position in world space, passed via a variable store
float3 PlanePosition = float3( 0, 0, 0 );
// Player ship velocity in world space, passed via a variable store
float3 RootVelocity = float3( 0, 0, 0 );
// X - active range; Y - fadeout length; Z - multiplier for range and fadeout; W - base/source/ship radius
float4 Fadeout = float4( 200000.0, 50000.0, 1.0, 50.0 );

#endif