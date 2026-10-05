// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVEBANNERSET_FXH
#define CARBON_EVEBANNERSET_FXH


// Data passed from the engine to the vertex shader stage for EveBannerSet draw calls.
struct EveBannerSetVertexInput
{
    // Vertex position in local space
    float3 position : POSITION;
    // Vertex normal in local space
    float3 normal : NORMAL;
    // UV coordinates
    float2 texCoord : TEXCOORD;
    // W component contains bone index for skinning, XYZ components are unused
    int4 indices : BLENDINDICES;
};

// EveBannerSet uses the same per-object data layout as other space objects. See PerObjectData.fxh

#endif