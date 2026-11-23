// Copyright © 2026 CCP ehf.

#ifndef CARBON_DYNAMICLIGHTING_FXH
#define CARBON_DYNAMICLIGHTING_FXH

#include "../System.fxh"
#include "PerSceneData.fxh"
#include "../Math.fxh"

// Packed light data for a single light source. See LightBuffer variable.
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

// Light flags stored in PerLightData.color.w

// Light affects surfaces (opaque and transparent)
static const uint LIGHT_FLAG_AFFECTS_SURFACE = 1 << 16;
// Light affects particles (opaque and transparent)
static const uint LIGHT_FLAG_AFFECTS_PARTICLES = 1 << 17;
// Light casts shadows
static const uint LIGHT_FLAG_CASTS_SHADOWS = 1 << 18;
// Light affects volumetric fog (if enabled)
static const uint LIGHT_FLAG_IS_VOLUMETRIC = 1 << 19;

// Stores linked lists of indices into LightBuffer for each tile in the screen. The buffer is partitioned into two sections:
// 3 arrays of per-tile list heads (opaque, transparent and particle arrays) and per-light next indices. The first section contains one uint per tile, 
// which is the index (into LightIndexBuffer) of the first light in the linked list for that tile. The second section contains one uint per light, which is the index of the 
// next light in the linked list for that tile. A value of for the index 0 indicates the end of the list.
StructuredBuffer<uint> LightIndexBuffer <bool AutoRegister = true; >;
// Stores the actual light data for each potentially visible light in the scene. The index into this buffer is stored in LightIndexBuffer.
StructuredBuffer<PerLightData> LightBuffer <bool AutoRegister = true; >;

// Global array of light profiles (IES profiles) for lights that use them. The index into this array is stored in PerLightData.color.w.
Texture2DArray<float> LightProfileArray <bool AutoRegister = true; >;
SamplerState LightProfileSampler
{
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = None;
    AddressU = Clamp;
    AddressV = Clamp;
};

// Global shadow map atlas for all lights that cast shadows. The window of this atlas is stored in PerLightData.direction.w.
// This atlas is used when rendering rastrized shadows. Raytraced shadows are stored in EveSpaceSceneDynamicShadowMap.
DepthTexture2D<float> ShadowMapAtlas < bool AutoRegister = true; >;

SamplerComparisonState ShadowMapAtlasSampler
{
    AddressU = Clamp;
    AddressV = Clamp;
    ComparisonFunc = Greater;
    Filter = COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
};

Texture2D<uint> EveSpaceSceneDynamicShadowMap <bool AutoRegister = true; >;

// The size of a tile in pixels. This is used to determine which tile a pixel belongs to when looking up the light list for that tile.
#define TILE_SIZE 16


// ------------------------------------------------------------ PCF -------------------------------------------------------------

float2x2 _PCFTransform( float2 vpos, bool hasJitter, int frameIndex, float inverseShadowMapAtlasSize )
{
    float angle = 0.0;
	
	//Randomize the angle if jittering is enabled (= we have some form of temporal filtering)
	//Otherwise, we do no rotating at all to make sure the image isn't noisy or unstable when no AA or upscaling is enabled.
	
    if( hasJitter )
    {
        int2 pixelPos = int2( vpos );
		//We use 9 evenly distributed rotations, which change both per pixel and over time
		//This gives us effectively NUM_SAMPLES * 9 samples over time.
        uint dither = ( pixelPos.x * 3u + pixelPos.y * 4u + frameIndex * 5u ) % 9u;
        angle += float( dither ) * ( PI * 2.0 / 9.0 );
    }
	
	//Build a rotation matrix so that we can apply the quickly to each sample.
    float s;
    float c;
    sincos( angle, s, c );
    float2x2 rotation = float2x2(
		c, -s,
		s, c
	);

    return rotation * inverseShadowMapAtlasSize;
}

float2x2 _PCFTransform( float2 vpos, EveSpaceSceneDataPS perFramePS )
{
    bool hasJitter = perFramePS.MiscData2.y != 0;
    int frameIndex = perFramePS.MiscData2.x;
    float inverseShadowMapAtlasSize = asfloat( perFramePS.MiscData2.z );
    return _PCFTransform( vpos, hasJitter, frameIndex, inverseShadowMapAtlasSize );
}

// based on: https://www.shadertoy.com/view/4djSRW
float3 _Random( float2 p, uint timer )
{
    timer &= 2047;
    float3 p3 = float3( p, float( timer ) );
    p3 = frac( p3 * float3( .1031, .1030, .0973 ) );
    p3 += dot( p3, p3.yxz + 33.33 );
    return frac( ( p3.xxy + p3.yxx ) * p3.zyx );
}

float3 _PCFOffset( float2 vpos, EveSpaceSceneDataPS perFramePS )
{
    bool hasJitter = perFramePS.MiscData2.y != 0;
    return hasJitter ? _Random( vpos, perFramePS.MiscData2.x ).xyz * .004 : 0.0.xxx;
}

struct _PCFJitter
{
    float2x2 transform;
    float3 offset;
};

_PCFJitter _GetPCFJitter( float2 vpos, EveSpaceSceneDataPS perFramePS )
{
    _PCFJitter jitter;
    jitter.transform = _PCFTransform( vpos, perFramePS );
    jitter.offset = _PCFOffset( vpos, perFramePS );
    return jitter;
}

float _PCF( _PCFJitter pcfJitter, DepthTexture2D <float>ShadowMapAtlas, SamplerComparisonState ShadowMapAtlasSampler, float centerDepth, float2 uv )
{
    const int NUM_SAMPLES = 16; //higher count reduces flickering, but is slower
    const float RADIUS = 4.5; //higher increases filter radius, making the transition softer and reducing flickering

	//Here we do a golden angle spiral sample pattern ("optimal" even distribution over a circle) over a larger area
    float pcf = 0.0;
	[unroll]
    for( int i = 0; i < NUM_SAMPLES; i++ )
    {
		//The following values are all constant in an unrolled loop
		//That's why we don't include the extra "random" rotation here!
        const float GOLDEN_ANGLE = PI * ( 3.0 - sqrt( 5.0 ) );
        float angle = float( i ) * GOLDEN_ANGLE;
        float f = ( float( i ) + 0.5 ) / float( NUM_SAMPLES );
        float2 sampleOffset = float2( cos( angle ), sin( angle ) ) * ( sqrt( f ) * RADIUS );
        float weight = 1.0 / float( NUM_SAMPLES );

		//These are actually calculated per sample, so we apply the per-pixel rotation here
        float2 sampleUV = uv + mul( pcfJitter.transform, sampleOffset );

        float comparisonResult = ShadowMapAtlas.SampleCmpLevelZero( ShadowMapAtlasSampler, sampleUV, centerDepth );
        pcf += comparisonResult * weight;
    }

    float visibility = pcf;
    return visibility;
}

// ------------------------------------------------------------------------------------------------------------------------------

struct ShadowData
{
    _PCFJitter pcfJitter;
    uint raytracedMask;
};

ShadowData GetShadowData( float2 vpos, EveSpaceSceneDataPS perFramePS )
{
    ShadowData shadowData;
    shadowData.pcfJitter.transform = float2x2( 0., 0., 0., 0. );
    shadowData.pcfJitter.offset = 0.0.xxx;
    shadowData.raytracedMask = 0;

    uint shadowQuality = asuint( perFramePS.ShadowMapSettings2.w );
    if( ( shadowQuality & ( SHADOW_QUALITY_FLAG_LOW | SHADOW_QUALITY_FLAG_HIGH ) ) != 0 )
    {
        shadowData.pcfJitter = _GetPCFJitter( vpos, perFramePS );
    }
    if( ( shadowQuality & SHADOW_QUALITY_FLAG_RAYTRACED ) != 0 )
    {
        shadowData.raytracedMask = EveSpaceSceneDynamicShadowMap.Load( int3( int2( vpos.xy ), 0 ) ).x;
    }
    return shadowData;
}

// Light list types for GetDynamicLightingListHead function
static const uint LIGHT_LIST_TYPE_OPAQUE = 0;
static const uint LIGHT_LIST_TYPE_TRANSPARENT = 1;
static const uint LIGHT_LIST_TYPE_PARTICLES = 2;

// Returns the index of the first light record in the linked list for the tile that contains the pixel at vpos. 
// The lightListType parameter specifies which list to return (opaque, transparent or particle).
uint GetDynamicLightingListHead( uint2 vpos, uint targetWidth, uint lightListType )
{
    uint2 pixelPos = vpos.xy / TILE_SIZE;
    uint tilesPerRow = ( targetWidth + TILE_SIZE - 1 ) / TILE_SIZE;
    uint index = pixelPos.x + pixelPos.y * tilesPerRow;
    return LightIndexBuffer[index * 3 + lightListType];
}


struct ComputedDynamicLight
{
    // Direction from the lit position to the light source, normalized.
    float3 lightDirection;
    // Distance from the lit position to the light source.
    float distance;
    // Color of the light source, after shadowing. Does not take distance-based attenuation into account.
    float3 lightColor;
    // Increase in roughness due to the light.
    float roughnessIncrease;
    // Light radius
    float radius;
    // Light inner radius
    float innerRadius;
};

// Computes the lighting contribution of a single light source at a given position in world space.
// Returns true if the light contributes to the lighting at the given position (based on distance and filters), false otherwise.
// The lighting data is returned in the computedDynamicLight parameter.
// Parameters:
// shadowData - Shadow data for the current pixel, including PCF jitter and raytraced mask. Normally GetDynamicLight is called in a loop over all lights, 
//              and this data is computed once per pixel and passed to each call to GetDynamicLight.
// inverseShadowMapAtlasSize - Inverse size of the shadow map atlas (from EveSpaceSceneDataPS).
// shadowMapAtlasEntryMinSizeLog2 - Minimum size of a shadow map atlas entry, in log2 (from EveSpaceSceneDataPS).
// shadowQuality - Current shadow quality setting in the application (from EveSpaceSceneDataPS, see SHADOW_QUALITY_FLAG_*).
// lightData - Data for the light source, obtained from LightBuffer.
// litPosition - Position in world space being lit.
// flags - Flags for filtering which lights to consider (LIGHT_FLAG_*).
bool GetDynamicLight( 
    ShadowData shadowData, 
    float inverseShadowMapAtlasSize, 
    uint shadowMapAtlasEntryMinSizeLog2, 
    uint shadowQuality,
    PerLightData lightData, 
    float3 litPosition,
    uint flags,
    out ComputedDynamicLight computedDynamicLight )
{
    computedDynamicLight.lightDirection = 0;
    computedDynamicLight.distance = 0;
    computedDynamicLight.lightColor = 0;
    computedDynamicLight.roughnessIncrease = 0;
    computedDynamicLight.radius = 0;
    computedDynamicLight.innerRadius = 0;

    computedDynamicLight.lightDirection = lightData.position.xyz - litPosition;
    float3 unnormalizedLightDirection = computedDynamicLight.lightDirection;
    computedDynamicLight.distance = length( computedDynamicLight.lightDirection );
    computedDynamicLight.lightDirection /= computedDynamicLight.distance;

    float radius = lightData.position.w;
    computedDynamicLight.radius = radius;
    float innerRadius = f16tof32( asuint( lightData.color.w ) & 0xffff );
    computedDynamicLight.innerRadius = innerRadius;
    uint lightFlags = asuint( lightData.color.w );
    uint profile = lightFlags >> 20;
    bool gotShadowMap = ( lightFlags & LIGHT_FLAG_CASTS_SHADOWS ) != 0;
    
    float3 lightDir = float3(
        f16tof32( asuint( lightData.direction.x ) & 0xffff ),
        f16tof32( asuint( lightData.direction.x ) >> 16 ),
        f16tof32( asuint( lightData.direction.y ) & 0xffff )
    );
    float outerCosAngle = f16tof32( asuint( lightData.direction.z ) & 0xffff );
    float innerCosAngle = f16tof32( asuint( lightData.direction.z ) >> 16 );
    float cosAngle = dot( lightDir, computedDynamicLight.lightDirection );
 
    bool result = false;
    [branch]
    if( computedDynamicLight.distance < radius && ( lightFlags & flags ) != 0 )
    {
        float pointLightFuzzyness = clamp( innerRadius / computedDynamicLight.distance, 0.0, 0.8 );

        // fuzzFactor needs to be in the 4th power (or 2 powers stronger than roughness) 
        // so the fuzz doesn't overshadow the roughness of the material (if that makes sense)
        float fuzzFactor = pow( pointLightFuzzyness, 4 );

        float attenuation = 1;
        
        if( outerCosAngle > 0.0f )
        {
            attenuation *= pow( LinStep( outerCosAngle, innerCosAngle, cosAngle ), 2 );
        }
        if( profile > 0 )
        {
            attenuation *= LightProfileArray.SampleLevel( LightProfileSampler, float3( cosAngle * -0.5 + 0.5, 0, profile - 1 ), 0 );
        }

        if( ( shadowQuality & ( SHADOW_QUALITY_FLAG_LOW | SHADOW_QUALITY_FLAG_HIGH ) ) != 0 && inverseShadowMapAtlasSize > 0. && gotShadowMap && attenuation > 0. )
        {
            uint shadowMapScale = ( ( asuint( lightData.direction.w ) >> 2 ) & ( ( 1 << 10 ) - 1 ) ) << shadowMapAtlasEntryMinSizeLog2;
            uint shadowMapOffsetX = ( ( asuint( lightData.direction.w ) >> 12 ) & ( ( 1 << 10 ) - 1 ) ) << shadowMapAtlasEntryMinSizeLog2;
            uint shadowMapOffsetY = ( ( asuint( lightData.direction.w ) >> 22 ) & ( ( 1 << 10 ) - 1 ) ) << shadowMapAtlasEntryMinSizeLog2;
            float projectionPlaneDistance = f16tof32( asuint( lightData.direction.y ) >> 16 );

            float depth = 0.;
            if( outerCosAngle > 0.0f )
            {
                // spotlight
                lightDir = -lightDir;
                float3 up = abs( lightDir.y ) < .7 ? float3( 0., 1., 0. ) : float3( 1., 0., 0. );
                float3 side = normalize( cross( lightDir, up ) );
                up = cross( side, lightDir );
                float3x3 lightRotation = float3x3( side, up, lightDir );
                unnormalizedLightDirection = mul( lightRotation, -unnormalizedLightDirection );
                depth = unnormalizedLightDirection.z;
            }
            else
            {
                // pointlight
                float3 absLightDir = abs( computedDynamicLight.lightDirection );
                absLightDir += shadowData.pcfJitter.offset;
                float maxDir = max( max( absLightDir.x, absLightDir.y ), absLightDir.z );
                if( absLightDir.x == maxDir )
                {
                    depth = unnormalizedLightDirection.x;
                    unnormalizedLightDirection.xy = unnormalizedLightDirection.zy;
                    shadowMapOffsetX += 0;
                }
                else if( absLightDir.y == maxDir )
                {
                    depth = unnormalizedLightDirection.y;
                    unnormalizedLightDirection.xy = unnormalizedLightDirection.xz;
                    shadowMapOffsetX += shadowMapScale;
                }
                else
                {
                    depth = unnormalizedLightDirection.z;
                    unnormalizedLightDirection.xy = unnormalizedLightDirection.yx;
                    shadowMapOffsetX += 2 * shadowMapScale;
                }
                unnormalizedLightDirection.y *= depth > 0. ? -1. : 1.;
                shadowMapOffsetY += depth < 0. ? shadowMapScale : 0;
                depth = abs( depth );
            }

            // perspective projection of z, assuming reverse z and far clip being the same as near clip divided by 1000
            const float b = 1. / 999.;
            float m = radius * b;
            unnormalizedLightDirection.z = -b + m / depth;
            
            // perspective projection of xy and flipping image
            unnormalizedLightDirection.xy *= ( projectionPlaneDistance / depth );
            unnormalizedLightDirection.y *= -1.;

            // margin scale and changing value range from [-1, 1] to [0, 1]
            const float margin = 8.;
            float marginScale = .5 - ( margin / shadowMapScale );
            unnormalizedLightDirection.xy = unnormalizedLightDirection.xy * marginScale + .5.xx;

            // coordinate on atlas
            unnormalizedLightDirection.xy = unnormalizedLightDirection.xy * float( shadowMapScale ) + float2( shadowMapOffsetX, shadowMapOffsetY );
            unnormalizedLightDirection.xy *= inverseShadowMapAtlasSize;

            float visibility = _PCF( shadowData.pcfJitter, ShadowMapAtlas, ShadowMapAtlasSampler, unnormalizedLightDirection.z, unnormalizedLightDirection.xy );
            
            attenuation *= visibility;
        }

        if( ( shadowQuality & SHADOW_QUALITY_FLAG_RAYTRACED ) != 0 && gotShadowMap && attenuation > 0. )
        {
            uint shadowMask = ( ( asuint( lightData.direction.w ) ) & ( ( 1 << 16 ) - 1 ) );
            int shadow = shadowData.raytracedMask & shadowMask;
            attenuation *= shadow != 0 ? 0. : 1.;
        }

        computedDynamicLight.lightColor = lightData.color.rgb * attenuation; // innerRadius now softens the core in GetBoundedAttenuation; (1-fuzz) dimming dropped to avoid double-counting source size (fuzz still drives roughnessIncrease)
        computedDynamicLight.roughnessIncrease = fuzzFactor;
        result = true;
    }
    return result;
}

// Computes the lighting contribution of a single light source at a given position in world space.
// Returns true if the light contributes to the lighting at the given position (based on distance and filters), false otherwise.
// The lighting data is returned in the computedDynamicLight parameter.
// Parameters:
// shadowData - Shadow data for the current pixel, including PCF jitter and raytraced mask. Normally GetDynamicLight is called in a loop over all lights, 
//              and this data is computed once per pixel and passed to each call to GetDynamicLight.
// index - Index of the light in LightBuffer to compute lighting for.
// litPosition - Position in world space being lit.
// perFramePS - Per-scene pixel shader data (from EveSpaceSceneDataPS).
// flags - Flags for filtering which lights to consider (LIGHT_FLAG_*).
bool GetDynamicLight( ShadowData shadowData, uint index, float3 litPosition, EveSpaceSceneDataPS perFramePS, uint flags, out ComputedDynamicLight computedDynamicLight )
{
    PerLightData lightData = LightBuffer[index];
    float inverseShadowMapAtlasSize = asfloat( perFramePS.MiscData2.z );
    uint shadowMapAtlasEntryMinSizeLog2 = asuint( perFramePS.MiscData2.w );
    uint shadowQuality = asuint( perFramePS.ShadowMapSettings2.w );
    return GetDynamicLight( shadowData, inverseShadowMapAtlasSize, shadowMapAtlasEntryMinSizeLog2, shadowQuality, lightData, litPosition, flags, computedDynamicLight );
}


#endif