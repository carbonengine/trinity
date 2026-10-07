// Copyright © 2026 CCP ehf.

#ifndef FOG_FXH
#define FOG_FXH

#include "../../../../../include/Carbon/System.fxh"


static const uint LIGHT_FLAG_AFFECTS_SURFACE = 1 << 16;
static const uint LIGHT_FLAG_AFFECTS_PARTICLES = 1 << 17;
static const uint LIGHT_FLAG_CASTS_SHADOWS = 1 << 18;
static const uint LIGHT_FLAG_IS_VOLUMETRIC = 1 << 19;

struct PerLightData
{
    float4 position; // xyz - world position, w - radius
    float4 color; // xyz - color, w -  bits 0..15 - innerRadius (float 16), bits 16..31 - flags
    float4 direction; // see next few lines:
    // x bits 0..15 - direction.x (float 16), x bits 16..31 - direction.y (float 16)
    // y bits 0..15 - direction.z (float 16), y bits 16..31 - projectionPlaneDistance (float 16)
    // z bits 0..15 - outerAngle (float 16), z bits 16..31 - innerAngle (float 16)
    // w bits 0..1 - padding, w bits 2..11 - shadowMapScale, w bits 12..21 - shadowMapOffsetX, w bits 22..31 - shadowMapOffsetY
};

struct HitInfo
{
	float visibility;
};

#ifndef RAYTRACING
Texture2D<float> ShadowMapAtlas <bool AutoRegister = true; >;
#endif

struct FroxelPerObjectData
{
    uint3 Resolution;
    uint NumDynamicLights;

    float3 Jitter;
    float Far;

    float3 Scattering;
    float BaseDensity;

    float MaxDistanceVisibility;
    float LightG;
    float EnvironmentIntensity;
    float InverseShadowMapAtlasSize;

    float3 Extinction;
    uint ShadowMapAtlasEntryMinSizeLog2;

    float4x4 InverseViewMatrix;


    float GodRayNoiseFrequency;
    float GodRayNoiseLerp;
    float GodRayNoiseAnimation;
    float GodRayNoiseIntensity;

    float4x4 GodRayNoiseMatrix;


	float3 FogNoiseOffset;
    float FogNoiseFrequency;

    float FogNoiseLerp;
    float FogNoiseIntensity;
    float2 LinearizeDepthParams;


    float4 UnprojectParams;
    float4 PreviousProjectParams;
    float4x4 ReprojectionMatrix;

    float3 SunViewDirection;
    float SunAngle;

    float3 SunWorldDirection;
    float pad0;

    float3 SunColor;
    float LightProfileTextureWidth;

    //Directional light shadows
	float4 ShadowMapValues[4]; // x = zFar value[0], y = zFar value[1], z = zFar value[2], w = zFar value[3]..etc
    float4x4 ShadowMatrix[16]; // Matrix that takes a coordinate from view space all the way to the packed cascades
    float4 SplitInfo; // x = NrOfSplits, y = <unused>, z = <unused>, w = <unused>
    
    PerLightData DynamicLights[16];
    
    float4 Planets[2];
};

cbuffer FroxelPerObjectCB : register(b3)
{
    FroxelPerObjectData FroxelPerObject;
}

float Dither(uint2 xy, float jitter)
{
	float g = 1.32471795724474602596;
	return frac(jitter + dot(float2(xy), 1.0 / float2(g, g*g)));
}

float GetLayerDistance(float normalizedZ)
{
    float layerDistance = -log(lerp(1.0, FroxelPerObject.MaxDistanceVisibility, normalizedZ)) / FroxelPerObject.BaseDensity;
    return layerDistance;
}

float GetNormalizedZ(float layerDistance)
{
    float normalizedZ = saturate((exp(-layerDistance * FroxelPerObject.BaseDensity) - 1.0) / (FroxelPerObject.MaxDistanceVisibility - 1.0));
    return normalizedZ;
}

#endif