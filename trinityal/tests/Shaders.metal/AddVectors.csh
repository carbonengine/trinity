#include "MetalDefines.h"

// [numthreads(1, 1, 1)]
kernel void mainCS(const device float4* arg1 [[ SRV(0) ]], const device float4* arg2 [[ SRV(1) ]], device float4* output [[ UAV(0) ]])
{
    output[0] = arg1[0] + arg2[0];
}
