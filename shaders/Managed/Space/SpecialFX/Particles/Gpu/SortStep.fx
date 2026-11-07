// Copyright © 2026 CCP ehf.

#include "../../../../../include/Carbon/SpaceScene/GpuParticles.fxh"

// Code based on https://github.com/GPUOpen-LibrariesAndSDKs/GPUParticles11

RWStructuredBuffer<GpuParticleVisibleData> VisibleBuffer;
Buffer<uint> SortParameters;

struct PerObjectVSData
{
    int4 JobParams;
} PerObjectVS;

[numthreads( 256, 1, 1 )]
void BitonicSortStep( uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID )
{
    int4 tgp;

    tgp.x = Gid.x * 256;
    tgp.y = 0;
    tgp.z = SortParameters[3];
    tgp.w = min( 512, max( 0, tgp.z - (int)Gid.x * 512 ) );

    uint localID = tgp.x + GTid.x; // calculate threadID within this sortable-array

    uint index_low = localID & ( PerObjectVS.JobParams.x - 1 );
    uint index_high = 2 * ( localID - index_low );

    uint index = tgp.y + index_high + index_low;
    uint nSwapElem = tgp.y + index_high + PerObjectVS.JobParams.y + PerObjectVS.JobParams.z * index_low;

    if( nSwapElem < uint( tgp.y + tgp.z ) )
    {
        GpuParticleVisibleData a = VisibleBuffer[index];
        GpuParticleVisibleData b = VisibleBuffer[nSwapElem];

        if( a.depth > b.depth )
        {
            VisibleBuffer[index] = b;
            VisibleBuffer[nSwapElem] = a;
        }
    }
}

technique Main
{
    pass p0
    {
        ComputeShader = compile cs_5_0 BitonicSortStep();
    }
}
