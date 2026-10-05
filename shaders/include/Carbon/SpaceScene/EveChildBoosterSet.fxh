// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVECHILDBOOSTERSET_FXH
#define CARBON_EVECHILDBOOSTERSET_FXH


// Data passed from the engine to the vertex shader stage for EveChildBoosterSet draw calls.
struct EveChildBoosterSetVertexInput
{
    // Vertex position in local instance space
    float3 position : POSITION;
};


// Data passed from the engine to the vertex shader stage for EveChildBoosterSet draw calls.
struct EveChildBoosterSetInstanceData
{
    // 4x3 matrix that transforms the booster from local instance space to object space
    float4x3 transform;
    // Booster intensity
    float intensity;
    // Per booster instance random number from 0 to 1 used for offsetting booster animations
    float wavePhase;
    // Texture atlas indices, passed from SOF
    uint atlasIndex0;
    uint atlasIndex1;
};

StructuredBuffer<EveChildBoosterSetInstanceData> ChildBoosterSetInstances <bool AutoRegister = true;>;


// Constant buffer format for per-object data for EveChildBoosterSet. This data is provided by the engine to the vertex shader stage.
struct EveChildBoosterSetDataVS
{
	// World matrix of the booster's owner ship
    float4x4 WorldMat;
    float _padding0;
    float _padding1;
    // Booster size at maximum intensity (used for scaling the booster in the shader)
    float maxBoosterSize;
    // Offset into the ChildBoosterSetInstances structured buffer for this booster set's instances
    uint instanceOffset;
};


// Constant buffer format for per-object data for EveChildBoosterSet. This data is provided by the engine to the pixel shader stage.
struct EveChildBoosterSetDataPS
{
    // Additional ship-data ( components: .x = unused, .y = unused, .z = warpStrength, .w = unused )
    float4 Shipdata;
};


#endif