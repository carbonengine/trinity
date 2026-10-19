// Copyright © 2026 CCP ehf.

#include "../../../../../include/Carbon/SpaceScene/GpuParticles.fxh"

struct PerObjectVSData
{
    uint bufferSize;
} PerObjectVS;

RWStructuredBuffer<uint> DeadBuffer;
RWBuffer<int> ParticleCounters;
RWStructuredBuffer<GpuParticleData> ParticleBuffer;


[numthreads( 1, 1, 1 )]
void ClearCounter()
{
    ParticleCounters[0] = 0;
}

#define POPULATE_DEAD_DIM 16

[numthreads( POPULATE_DEAD_DIM, POPULATE_DEAD_DIM, 1 )]
void PopulateDead( uint groupIndex : SV_GroupIndex )
{
    uint iterations = PerObjectVS.bufferSize / POPULATE_DEAD_DIM / POPULATE_DEAD_DIM;
    for( uint i = 0; i < iterations; ++i )
    {
        GpuParticleData particle = (GpuParticleData)0;
        particle.age = -1;
        ParticleBuffer[groupIndex + POPULATE_DEAD_DIM * POPULATE_DEAD_DIM * i] = particle;
        int index;
        InterlockedAdd( ParticleCounters[0], 1, index );
        DeadBuffer[index] = groupIndex + POPULATE_DEAD_DIM * POPULATE_DEAD_DIM * i;
    }

    GroupMemoryBarrierWithGroupSync();

    if( groupIndex == 0 )
    {
        for( uint i = iterations * POPULATE_DEAD_DIM * POPULATE_DEAD_DIM; i < PerObjectVS.bufferSize; ++i )
        {
            GpuParticleData particle = (GpuParticleData)0;
            particle.age = -1;
            ParticleBuffer[i] = particle;
            int index;
            InterlockedAdd( ParticleCounters[0], 1, index );
            DeadBuffer[index] = i;
        }
    }
}


technique Main
{
    pass p0
    {
        ComputeShader = compile cs_5_0 ClearCounter();
    }
    pass p1
    {
        ComputeShader = compile cs_5_0 PopulateDead();
    }
}
