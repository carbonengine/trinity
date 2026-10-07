// Copyright © 2026 CCP ehf.

#include "../../../../../include/Carbon/SpaceScene/GpuParticles.fxh"

// Code based on https://github.com/GPUOpen-LibrariesAndSDKs/GPUParticles11

#define SORT_SIZE 512

#define NUM_THREADS		256 // (SORT_SIZE/2)
#define INVERSION		(16*2 + 8*3)

RWStructuredBuffer<GpuParticleVisibleData> VisibleBuffer;
Buffer<uint> SortParameters;

groupshared GpuParticleVisibleData g_LDS[SORT_SIZE];


[numthreads( NUM_THREADS, 1, 1 )]
void BitonicInnerSort( 
    uint3 Gid : SV_GroupID,
    uint3 DTid : SV_DispatchThreadID,
    uint3 GTid : SV_GroupThreadID,
    uint GI : SV_GroupIndex )
{
    int4 tgp;

    tgp.x = Gid.x * 256;
    tgp.y = 0;
    tgp.z = SortParameters[3];
    tgp.w = min( 512, max( 0, tgp.z - int( Gid.x * 512 ) ) );

    int GlobalBaseIndex = tgp.y + tgp.x * 2 + GTid.x;
    int LocalBaseIndex = GI;
    int i;

    // Load shared data
    if( GlobalBaseIndex < tgp.z )
    {
		[unroll]
        for( i = 0; i < 2; ++i )
        {
            if( (int)GI + i * NUM_THREADS < tgp.w )
                g_LDS[LocalBaseIndex + i * NUM_THREADS] = VisibleBuffer[GlobalBaseIndex + i * NUM_THREADS];
        }
    }
    GroupMemoryBarrierWithGroupSync();

	// sort threadgroup shared memory
    for( int nMergeSubSize = SORT_SIZE >> 1; nMergeSubSize > 0; nMergeSubSize = nMergeSubSize >> 1 )
    {
        int tmp_index = GI;
        int index_low = tmp_index & ( nMergeSubSize - 1 );
        int index_high = 2 * ( tmp_index - index_low );
        int index = index_high + index_low;

        uint nSwapElem = index_high + nMergeSubSize + index_low;

        if( (int)nSwapElem < tgp.w )
        {
            GpuParticleVisibleData a = g_LDS[index];
            GpuParticleVisibleData b = g_LDS[nSwapElem];

            if( a.depth > b.depth )
            {
                g_LDS[index] = b;
                g_LDS[nSwapElem] = a;
            }
        }
        GroupMemoryBarrierWithGroupSync();
    }
    
    // Store shared data
    if( GlobalBaseIndex < tgp.z )
    {
		[unroll]
        for( i = 0; i < 2; ++i )
        {
            if( (int)GI + i * NUM_THREADS < tgp.w )
                VisibleBuffer[GlobalBaseIndex + i * NUM_THREADS] = g_LDS[LocalBaseIndex + i * NUM_THREADS];
        }
    }
}

technique Main
{
    pass p0
    {
        ComputeShader = compile cs_5_0 BitonicInnerSort();
    }
}
