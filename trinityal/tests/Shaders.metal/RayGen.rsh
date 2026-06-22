// Copyright © 2024 CCP ehf.

 #include "MetalDefines.h"

#include <metal_stdlib>

using namespace metal;
using namespace raytracing;



struct Uniforms{
    float4x4 ViewMat;
    float4 viewOriginAndTanHalfFovY;
    float2 resolution;
};

struct TestStruct
{
     instance_acceleration_structure accelerationStructure[1];
};

struct Material
{
    device void* buffers;
};

constant bool useIntersectionFunctions [[function_constant(0)]];

struct GlobalInput
{
    constant const Uniforms* uniforms;
    texture2d<float, access::write> RTOutput;
    device RaytracingAccelerationStructure accelerationStructure;
};


kernel void RayGen(
    device __RtLocalMaterial* __rtMaterials [[SRV(0)]],
    constant GlobalInput& __rtGlobals [[SRV(1)]],
    constant ShaderTableT<float4, GlobalInput>& __rtShaderTable[[SRV(2)]],
    uint2 tid [[thread_position_in_grid]] )
{
    auto uniforms = __rtGlobals.uniforms[0];
    auto RTOutput = __rtGlobals.RTOutput;
    auto accelerationStructure = __rtGlobals.accelerationStructure;
    
    if((int)tid.x < uniforms.resolution.x && (int)tid.y < uniforms.resolution.y)
    {
        // The ray to cast.
        RayDesc ray;
        
        uint2 LaunchIndex = tid;
        
        float2 d = (((float2(LaunchIndex.xy) + 0.5f) / uniforms.resolution.xy) * 2.f - 1.f);
        
        float aspectRatio = (uniforms.resolution.x / uniforms.resolution.y);
        float4x4 viewMat = transpose( uniforms.ViewMat );

        ray.Origin = uniforms.viewOriginAndTanHalfFovY.xyz;
        ray.Direction = normalize((d.x * viewMat[0].xyz * uniforms.viewOriginAndTanHalfFovY.w * aspectRatio) - (d.y * viewMat[1].xyz * uniforms.viewOriginAndTanHalfFovY.w) + viewMat[2].xyz);
        // Use the MPSRayIntersection intersectionDataType property to return the
        // intersection distance for this kernel only. You don't need the other fields, so
        // you'll save memory bandwidth.
        ray.TMin = 0.f;
        ray.TMax = 100000000.f;

        
        float4 colorPayload = float4( 0, 1, 0, 0 );

        TraceRay(
            accelerationStructure,
            0,
            0xff,
            0,0,0,
            ray,
            colorPayload );

        RTOutput.write(colorPayload, tid);
    }
}
