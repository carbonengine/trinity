// Copyright © 2026 CCP ehf.

#include "../../../../../include/Carbon/SpaceScene/GpuParticles.fxh"

// Code based on https://github.com/GPUOpen-LibrariesAndSDKs/GPUParticles11

#define SORT_SIZE 512

#define HALF_SIZE		256 // (SORT_SIZE/2)
#define ITERATIONS		1 // (HALF_SIZE > 1024 ? HALF_SIZE/1024 : 1)
#define NUM_THREADS		256 // (HALF_SIZE/ITERATIONS)
#define INVERSION		(16*2 + 8*3)


RWStructuredBuffer<GpuParticleVisibleData> VisibleBuffer;
Buffer<uint> SortParameters;


groupshared GpuParticleVisibleData g_LDS[SORT_SIZE];


[numthreads( NUM_THREADS, 1, 1 )]
void BitonicSortLDS( uint3 Gid : SV_GroupID,
					 uint3 DTid : SV_DispatchThreadID,
					 uint3 GTid : SV_GroupThreadID,
					 uint GI : SV_GroupIndex )
{
    int GlobalBaseIndex = ( Gid.x * SORT_SIZE ) + GTid.x;
    int LocalBaseIndex = GI;

    int g_NumElements = int( SortParameters[3] );
    if( g_NumElements == 0 )
    {
        return;
    }

    uint numElementsInThreadGroup = max( 0, min( SORT_SIZE, g_NumElements - int( Gid.x * SORT_SIZE ) ) );
	
    // Load shared data
    int i;
	[unroll]
    for( i = 0; i < 2 * ITERATIONS; ++i )
    {
        if( GI + i * NUM_THREADS < numElementsInThreadGroup )
            g_LDS[LocalBaseIndex + i * NUM_THREADS] = VisibleBuffer[GlobalBaseIndex + i * NUM_THREADS];
    }
    GroupMemoryBarrierWithGroupSync();
    
	// Bitonic sort
    for( uint nMergeSize = 2; nMergeSize <= SORT_SIZE; nMergeSize = nMergeSize * 2 )
    {
        for( int nMergeSubSize = nMergeSize >> 1; nMergeSubSize > 0; nMergeSubSize = nMergeSubSize >> 1 )
        {
			[unroll]
            for( i = 0; i < ITERATIONS; ++i )
            {
                int tmp_index = GI + NUM_THREADS * i;
                int index_low = tmp_index & ( nMergeSubSize - 1 );
                int index_high = 2 * ( tmp_index - index_low );
                int index = index_high + index_low;

                uint nSwapElem = uint( nMergeSubSize ) == nMergeSize >> 1 ? index_high + ( 2 * nMergeSubSize - 1 ) - index_low : index_high + nMergeSubSize + index_low;
                if( nSwapElem < numElementsInThreadGroup )
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
        }
    }
    
    // Store shared data
	[unroll]
    for( i = 0; i < 2 * ITERATIONS; ++i )
    {
        if( GI + i * NUM_THREADS < numElementsInThreadGroup )
            VisibleBuffer[GlobalBaseIndex + i * NUM_THREADS] = g_LDS[LocalBaseIndex + i * NUM_THREADS];
    }
}

technique Main
{
    pass p0
    {
        ComputeShader = compile cs_5_0 BitonicSortLDS();
    }
}
