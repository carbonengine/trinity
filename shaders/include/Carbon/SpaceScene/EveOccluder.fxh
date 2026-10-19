// Copyright © 2026 CCP ehf.

#ifndef CARBON_EVEOCCLUDER_FXH
#define CARBON_EVEOCCLUDER_FXH

#include "DepthMap.fxh"
#include "Volumetrics.fxh"

// Offset into the occlusion buffer where the occlusion results for this occluder are stored. 
// Even though the variable is declared as a float, it is used as an integer value in the shader code (use asuint).
float OcclusionBufferOffset <bool AutoRegister = true;>;
// Scaling factor for fog occlusion to apply to the resulting occlusion value.
float OcclusionFogWeight <bool AutoRegister = true;>;
// Direction towards the lens flare that owns the occluder; W - flare size scale
float4 LensflareFxDirectionScale <bool AutoRegister = true;>;


// Destination buffer for writing occlusion results. Each occluder occupies two consecutive uint values in the buffer.
// The first value is the total occluder's shaded pixel count, and the second value is the unoccluded pixel count. 
RWBuffer<uint> FlareOcclusionBuffer <bool AutoRegister = true;>;

SamplerState _EveOccluderDepthMapSampler
{
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = None;
    AddressU = Mirror;
    AddressV = Mirror;
};

// Records occlusion for a pixel with normalized screen space coordinates.
void EveOccluderRecordOcclusion( float2 pos )
{
    uint bufferOffset = asuint( OcclusionBufferOffset );
    float depth = DepthMap.SampleLevel( _EveOccluderDepthMapSampler, pos, 0.0 ).r;
    if( depth == 0 )
    {
        float4 fog = EveSceneFogVolumeMap.SampleLevel( EveSceneFogVolumeMapSampler, float3( pos, 3 ), 0 );
        InterlockedAdd( FlareOcclusionBuffer[1 + bufferOffset], uint( 100 * ( 1 - min( fog.a * OcclusionFogWeight, 0.95 ) ) ) );
    }
    InterlockedAdd( FlareOcclusionBuffer[bufferOffset], 1 );
}

#endif