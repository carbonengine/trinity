// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVECHILDCLOUD2_FXH
#define CARBON_EVECHILDCLOUD2_FXH


// Data passed from the engine to the vertex shader stage for EveChildCloud2 draw calls.
struct EveChildCloud2VertexInput
{
	// Vertex position in object space
    float3 position : POSITION;
};


struct EveChildCloud2LightInfo
{
    float4 position; // xyz - position, w - radius
    float4 color; // xyz - color, w - normalized inner radius
};


// Constant buffer format for per-object data for EveChildCloud2. This data is provided by the engine to all shader stages.
struct EveChildCloud2PerObjectData
{
    float4x4 WorldMat;
    float4x4 ProjectionInvMat;
    float4x4 WorldViewInvMat;
    uint4 LightmapDimensions; // xyz - lightmap width/height/depth, w - current generation offset
    uint4 NoiseConfig; // xy - random offset, zw - noise texture width/height
    float4 SunDirection; // xyz - sun direction in model space, w - slice 0 w
    float4 ViewPosition; // xyz - view position in model space, w - slice 1 w
    float4 ViewDirection; // xyz - view direction in model space, w - slice 2 w
    float4 RelativeScaling; // xyz - renormalizing factors for non-uniform scaling, w - lod factor
    float4 TargetInvSize; // xy - render target inv size x2
    EveChildCloud2LightInfo Lights[4];
    float4 MapOffsets[3];
    float4x4 LightViewProj;
};


// The expected pixel shader output from EveChildCloud2 materials. 
// Each slice of the cloud is written to a separate render target.
// Slice depth is taken from EveChildCloud2PerObjectData
struct EveChildCloud2PSOutput
{
    float4 slice0 : SV_Target0;
    float4 slice1 : SV_Target1;
    float4 slice2 : SV_Target2;
    float4 slice3 : SV_Target3;
};


#endif