// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVECHILDCLOUD_FXH
#define CARBON_EVECHILDCLOUD_FXH


// Data passed from the engine to the vertex shader stage for EveChildCloud draw calls.
struct EveChildCloudVertexInput
{
    float2 position : POSITION;
};


// Data passed from the engine to the vertex shader stage for EveChildCloud draw calls.
struct EveChildCloudPerObjectDataVS
{
    float4x4 WorldMat;
    float4x4 WorldViewMat;
    float4x4 WorldViewInv;
    float4x4 ProjectionInv;
    // Near plane in object space
    float4 NearPlaneLocal;
    // XYZ - camera position in object space, W - screen depth of the object (Z in clip space)
    float4 EyePosLocal;
    // Bounds of the cloud in projection space (XY - min, ZW - max)
    float4 ScreenSize;
};


#endif