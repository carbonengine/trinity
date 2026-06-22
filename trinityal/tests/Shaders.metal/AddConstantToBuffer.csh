// Copyright © 2023 CCP ehf.

#include "MetalDefines.h"
#include <metal_stdlib>
using namespace metal;

struct Constants
{
    float4 arg1;
};

// [numthreads(1, 1, 1)]
kernel void mainCS(constant Constants& c [[ CBUFFER(1) ]], const device float4* arg2 [[ SRV(0) ]], device float4* output [[ UAV(0) ]])
{
    output[0] = c.arg1 + arg2[0];
}
