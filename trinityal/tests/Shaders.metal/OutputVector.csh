#include "MetalDefines.h"

// [numthreads(1, 1, 1)]
kernel void mainCS(device float4* output [[ UAV(0) ]])
{
    output[0] = float4( 1, 2, 3, 4 );
}
