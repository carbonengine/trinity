// Copyright © 2026 CCP ehf.

#include "../../../../../include/Carbon/Math.fxh"
#include "../../../../../include/Carbon/SpaceScene/GpuParticles.fxh"
#include "../../../../../include/Carbon/SpaceScene/PerSceneData.fxh"


#define GROUP_X 16
#define GROUP_Y 16


Texture3D NoiseMap
<
    bool SasUiVisible = true;
>;

SamplerState NoiseMapSampler
{
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
    AddressU = Wrap;
    AddressV = Wrap;
    AddressW = Wrap;
};

float3 GetTurbulence( float3 position, float frequency, float3 offset, float3 animation )
{
    position += offset;
    position *= frequency;
    position += NoiseMap.SampleLevel( NoiseMapSampler, animation / 32, 0.0 ).xyz;
    float3 result = 0;
    float amplitude = 1;
	
	[unroll]
    for( int i = 0; i < 3; ++i )
    {
        result += ( NoiseMap.SampleLevel( NoiseMapSampler, position, 0.0 ).xyz - 0.5 ) * amplitude;
        amplitude *= 0.5;
        position = 2.0 * position.zyx;
    }
    return result;
}

bool UpdateParticle( inout GpuParticleData particle, GpuParticleEmitterParams emitter, float dt, float seedf, float3 originShift, float3 turbulenceOffset, float3 turbulenceAnimation )
{
    if( particle.age >= 0 )
    {
        particle.age += dt;

        if( particle.age >= lerp( emitter.minLifeTime, emitter.maxLifeTime, seedf ) )
        {
            particle.age = -1;
            return false;
        }
        else
        {
			// forces
            particle.velocity += float3( 0, emitter.gravity * dt, 0 );
            particle.velocity += GetTurbulence( particle.position, emitter.turbulenceFrequency, turbulenceOffset, turbulenceAnimation ) * emitter.turbulenceAmplitude * dt;
            if( emitter.attractor.w != 0 )
            {
                float3 dir = normalize( emitter.attractor.xyz - particle.position );
                particle.velocity += dir * emitter.attractor.w * dt;
            }
            if( emitter.drag < 0 )
            {
                float speed = length( particle.velocity );
                if( speed > 0 )
                {
                    particle.velocity += particle.velocity * speed * emitter.drag * dt;
                }
            }
            else
            {
                particle.velocity -= particle.velocity * emitter.drag * dt;
            }

            particle.position += particle.velocity * dt + originShift;
            if( any( isnan( particle.position ) ) )
            {
                return false;
            }
            return true;
        }
    }
    else
    {
        return false;
    }
}


struct PerObjectVSData
{
    uint groupCountX;
    uint groupCountY;
    uint groupCountZ;
    float deltaTime;

    float3 originShift;
    uint bufferSize;
    float4 turbulenceOffset;
    float4 turbulenceAnimation;

    float4 frustumPlanes[6];
} PerObjectVS;

RWStructuredBuffer<GpuParticleData> ParticleBuffer;
RWStructuredBuffer<uint> DeadBuffer;
RWBuffer<int> ParticleCounters;
RWStructuredBuffer<GpuParticleVisibleData> VisibleBuffer;
StructuredBuffer<GpuParticleEmitterParams> Emitters;

EveSpaceSceneDataVS PerFrameVS : register( PERFRAME_VS_STARTREGISTER );

bool SphereInFrustum( float4 xyzr, float4 frustumPlane[6] )
{
    [unroll]
    for( uint i = 0; i != 6; ++i )
    {
        if( dot( float4( xyzr.xyz, 1 ), -frustumPlane[i] ) > xyzr.w )
        {
            return false;
        }
    }
    return true;
}

[numthreads( GROUP_X, GROUP_Y, 1 )]
void Update( uint3 grid : SV_GroupID, uint groupIndex : SV_GroupIndex )
{
    uint offset = ( grid.x + grid.y * PerObjectVS.groupCountX + grid.z * PerObjectVS.groupCountX * PerObjectVS.groupCountY ) * GROUP_X * GROUP_Y + groupIndex;
    float3 eye = ExtractEyePosFromViewMatrix( PerFrameVS.ViewInverseTransposeMat );
    float3 dir = normalize( ExtractEyeDirFromViewMatrix( PerFrameVS.ViewMat ) );
    float widthProjection = PerFrameVS.ProjectionMat[0][0] * PerFrameVS.TargetResolution.x;

    uint bufferSize = PerObjectVS.bufferSize;

    for( uint i = offset; i < bufferSize; i += GROUP_X * GROUP_Y * PerObjectVS.groupCountX * PerObjectVS.groupCountY * PerObjectVS.groupCountZ )
    {
        GpuParticleData particle = ParticleBuffer[i];
        GpuParticleEmitterParams emitter = Emitters[particle.emitterSeed & 0xffff];
        uint seed = particle.emitterSeed >> 16;
        float seedf = float( seed ) / float( 0xffff );

        if( particle.age >= 0 )
        {
            if( UpdateParticle( particle, emitter, PerObjectVS.deltaTime, seedf, PerObjectVS.originShift.xyz, PerObjectVS.turbulenceOffset.xyz, PerObjectVS.turbulenceAnimation.xyz ) )
            {
                float maxSize = max( emitter.sizes.x, max( emitter.sizes.y, emitter.sizes.z ) ) * ( 1 + abs( emitter.sizeVariance ) );
                float4 frustumPlanes[6] =
                {
                    PerObjectVS.frustumPlanes[0],
					PerObjectVS.frustumPlanes[1],
					PerObjectVS.frustumPlanes[2],
					PerObjectVS.frustumPlanes[3],
					PerObjectVS.frustumPlanes[4],
					PerObjectVS.frustumPlanes[5]
                };
                if( SphereInFrustum( float4( particle.position, maxSize ), frustumPlanes ) )
                {
                    float depth = -dot( dir, particle.position - eye );
                    float screenSize = maxSize / max( 1.0, depth ) * widthProjection;
                    if( screenSize > 2 )
                    {
                        int index;
                        InterlockedAdd( ParticleCounters[1], 1, index );

                        GpuParticleVisibleData ad;
                        ad.index = i;
                        ad.depth = -depth;
                        VisibleBuffer[index] = ad;
                    }
                }
            }
            else
            {
                int index;
                InterlockedAdd( ParticleCounters[0], 1, index );

                DeadBuffer[index] = i;
            }
            ParticleBuffer[i] = particle;
        }
        else
        {
            int index;
            InterlockedAdd( ParticleCounters[0], 1, index );
            DeadBuffer[index] = i;
        }
    }
}


[numthreads( 1, 1, 1 )]
void ClearCountersCS()
{
    ParticleCounters[0] = 0;
    ParticleCounters[1] = 0;
}


technique Main
{
    pass p0
    {
        ComputeShader = compile cs_5_0 Update();
    }
}

technique ClearCounters
{
    pass p0
    {
        ComputeShader = compile cs_5_0 ClearCountersCS();
    }
}
