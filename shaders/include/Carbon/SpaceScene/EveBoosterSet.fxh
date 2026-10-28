// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVEBOOSTERSET_FXH
#define CARBON_EVEBOOSTERSET_FXH

// Data passed from the engine to the vertex shader stage for EveBoosterSet draw calls.
struct EveBoosterSetVertexInput
{
    // Vertex position in local instance space
    float3 position : POSITION;
    // UV coordinates; only filled when shader quality is set to low, for "star" shaped boosters
    float2 texCoord : TEXCOORD0;
    // 4x4 matrix that transforms the booster from local instance space to object space
    float4 row1 : TEXCOORD1;
    float4 row2 : TEXCOORD2;
    float4 row3 : TEXCOORD3;
    float4 row4 : TEXCOORD4;
    // Custom values passed from SOF (no particular semantics, but can be used for various purposes in the shader)
    float4 functionality : TEXCOORD5;
    // Per booster instance random number from 0 to 1 used for offsetting booster animations
    float wavePhase : TEXCOORD6;
    // Texture atlas indices as floats, passed from SOF
    float2 atlasIndex : TEXCOORD7;
};


// Constant buffer format for per-object data for EveBoosterSet. This data is provided by the engine to the vertex shader stage.
struct EveBoosterSetDataVS
{
	// World matrix of the booster's owner ship
    float4x4 WorldMat;
    // Additional ship-data ( components: .x = boosterGain, .y = speed, .z = maxSize, .w = unused )
    float4 Shipdata;
    // Trail spline data
    float4 TrailsControlPositions[5];
    float4 TrailsControlNormals[5];
};


// Constant buffer format for per-object data for EveBoosterSet. This data is provided by the engine to the pixel shader stage.
struct EveBoosterSetDataPS
{
    // Additional ship-data ( components: .x = boosterGain, .y = trailIntensity, .z = warpStrength, .w = unused )
    float4 Shipdata;
};



#endif