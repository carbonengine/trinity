// Copyright © 2026 CCP ehf.

#ifndef CARBON_SPACESCENE_PERSCENEDATA_FXH
#define CARBON_SPACESCENE_PERSCENEDATA_FXH

#include "../System.fxh"


struct FroxelPerFrameData 
{
	float3 FogColor;
	float BackgroundVisibility;

	float BaseDensity;
	float MaxDistance;
	float MaxDistanceVisibility;
	float EnvironmentIntensity;

	float EnvironmentG;
	float _pad0;
	float _pad1;
	float _pad2;
	
    float4 Planets[2];
};

// Per-scene (per-frame) constant buffer data for the vertex shader.
struct EveSpaceSceneDataVS
{
	// matrices
    float4x4 ViewInverseTransposeMat;
    float4x4 ViewProjectionMat;  
    float4x4 ViewMat;
    float4x4 ProjectionMat;
    float4x4 ShadowViewMat;
    float4x4 ShadowViewProjectionMat;
    float4x4 EnvMapRotationMat;
    float4x4 ViewProjectionLast;
    float4x4 ViewLast;
    float4x4 ProjLast;
    // lighting info
    float4 SunDirection;
    float4 SunDiffuseColor;
	float3 FogFactors; // Depth multiplier in x, constant in y and max fog in z
	// rendertarget resolution and fovXY (rt pixelsize in .xy, fovXY in .zw)
	float4 TargetResolution;
	float4 ViewportAdjustment;
	float4 MiscParams; // x - time, y - upscaling amount, zw - viewport size
};


// Per-scene (per-frame) constant buffer data for the pixel shader.
struct EveSpaceSceneDataPS
{
    // some pixel-shaders do need these...
    float4x4 ViewInverseTransposeMat;
    float4x4 ViewMat;
    float4x4 EnvMapRotationMat;
    
	// scene info
    float4 SunDirection;
    float4 SunDiffuseColor; // .rgb - sun diffuse color, .a - environment map diffuse "roughness"
    float4 AmbientColor; // .rgb - ambient color, .a - nebula intensity
    float4 FogColor;

	// various info
	float4 ViewportOffsetSize; // .xy - viewport offset, .zw - viewport size
	float4 RenderTargetData; // .xy - RT size in pixel, z - MSAA sample count (legacy, always 1 nowadays), w - debug
	float4 ShadowMapSettings; // .xyzw - various shadow filter parameters
	float4 ShadowMapSettings2; // .xy - shadow camera range, .z - shadow lightness, .w - shadow quality
	float4 ProjectionData; // x - projection[3][2], y - projection[2][2], .zw = FoV
	float4 MiscData; // x - time, y - scenemiplodbias, z - upscaling amount, w - gamma brightness
	uint4 MiscData2; // x - frame counter, y - Jittering (bool), z - InverseShadowMapAtlasSize (float), w - ShadowMapAtlasEntryMinSizeLog2 (uint)
	float4 VolumetricSlices;
	float4 ShadowMapValues[4]; // x = zFar value[0], y = zFar value[1], z = zFar value[2], w = zFar value[3]..etc
    float4x4 ShadowMatrix[16]; // Matrix that takes a coordinate from view space all the way to the packed cascades
    float4 SplitInfo; // x = NrOfSplits, y = <unused>, z = <unused>, w = <unused>
    float4x4 ProjectionInverseMat; // 
	float4 CascadeRanges[16];

	//Froxel fog related global variables
	FroxelPerFrameData FroxelFogData;
};


#endif // CARBON_SPACESCENE_PERSCENEDATA_FXH