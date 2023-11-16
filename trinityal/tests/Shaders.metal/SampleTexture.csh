#include "MetalDefines.h"
#include <metal_stdlib>
using namespace metal;

// [numthreads(1, 1, 1)]
kernel void mainCS(texture2d<float> tex [[ texture(0) ]], sampler sampl [[ sampler(0) ]], device float4* output [[ UAV(0) ]])
{
    float avg = tex.read(uint2(0, 0)).x + tex.read(uint2(1, 0)).x + tex.read(uint2(0, 1)).x + tex.read(uint2(1, 1)).x;
    float sampled = tex.sample( sampl, float2( 0.5, 0.5 ), level( 0.0 ) ).x;
    output[0] = float4( avg, sampled, 0, 0 );
}
