// Copyright © 2026 CCP ehf.

#ifndef CARBON_RAYTRACING_SHADOWDATA_FXH
#define CARBON_RAYTRACING_SHADOWDATA_FXH


// Data passed to the shadow ray tracing shaders when rendering main sun light shadows in a "b2" constant buffer using Tr2RaytracingManager.
// Can be used by declaring a constant buffer as follows:
// cbuffer RtShadowsPerFrameBuffer: register( b2 ) { RtShadowsSunData RtShadowsPerFrame; }
struct RtShadowsSunData
{
    // The inverse projection transform of the camera for the current frame.
    float4x4 ProjectionInv;
    // The inverse view transform of the camera for the current frame.
    float4x4 ViewInv;
    // XYZ - sun direction in world space, W - sun angular size in radians.
    float4 SunDirection;

    // Planet positions and radii in world space. X, Y, Z - position, W - radius.
    float4 Planets[2];

    // The resolution of the render target in pixels.
    float2 Resolution;
    // The current frame index, used for temporal accumulation of shadows.
    uint FrameIndex;
};

struct RtShadowsPerLightData
{
    float4 position; // xyz - world position, w - radius
    float4 color; // xyz - color, w -  bits 0..15 - innerRadius (float 16), bits 16..31 - flags
    float4 direction; // see next few lines:
    // x bits 0..15 - direction.x (float 16), x bits 16..31 - direction.y (float 16)
    // y bits 0..15 - direction.z (float 16), y bits 16..31 - projectionPlaneDistance (float 16)
    // z bits 0..15 - outerAngle (float 16), z bits 16..31 - innerAngle (float 16)
    // w bits 0..1 - padding, w bits 2..11 - shadowMapScale, w bits 12..21 - shadowMapOffsetX, w bits 22..31 - shadowMapOffsetY
};

// Data passed to the shadow ray tracing shaders when rendering main sun light shadows in a "b7" constant buffer using Tr2LightManager.
// Can be used by declaring a constant buffer as follows:
// cbuffer RtShadowsPerFrameBuffer: register( b7 ) { RtShadowsPointLightsData RtShadowsPerFrame; }
struct RtShadowsPointLightsData
{
    // The inverse projection transform of the camera for the current frame.
    float4x4 ProjectionInv;
    // The inverse view transform of the camera for the current frame.
    float4x4 ViewInv;

    // Planet positions and radii in world space. X, Y, Z - position, W - radius.
    float4 Planets[2];

    // The resolution of the render target in pixels.
    float2 Resolution;
    // Number of dynamic lights in DynamicLights array.
    uint NumDynamicLights;
    float Padding;

    // Per-light data for dynamic lights in the scene. Up to 16 shadow casting lights are supported.
    RtShadowsPerLightData DynamicLights[16];
};



#endif