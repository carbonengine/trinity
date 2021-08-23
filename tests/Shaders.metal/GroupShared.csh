#include "MetalDefines.h"
#include <metal_stdlib>
using namespace metal;

// [numthreads(10, 10, 2)]
kernel void mainCS(device uint* output [[ UAV(0) ]], uint3 globalIdx [[ thread_position_in_grid ]])
{
    threadgroup atomic_uint sum;

    if( all( globalIdx == 0 ) )
    {
        atomic_store_explicit( &sum, 0, memory_order_relaxed );
    }
    threadgroup_barrier( mem_flags::mem_threadgroup );

    atomic_fetch_add_explicit( &sum, globalIdx.x + globalIdx.y + globalIdx.z, memory_order_relaxed);

    threadgroup_barrier( mem_flags::mem_threadgroup );
    if( all( globalIdx == 0 ) )
    {
        output[0] = atomic_load_explicit( &sum, memory_order_relaxed );
    }
}
