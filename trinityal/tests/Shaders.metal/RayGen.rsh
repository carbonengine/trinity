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

// Checks if a shadow ray hit something on the way to the light source. If not, the point the
// shadow ray started from was not in shadow so it's color should be added to the output image.
kernel void mainCS(           uint2 tid                                                                         [[thread_position_in_grid]],
                              constant Uniforms &uniforms                                                       [[CBUFFER(0)]],
                              device const TestStruct *accelerationStructure                                    [[buffer(5)]],
                              texture2d<float, access::write> RTOutput                                          [[UAVT(0)]],
                              intersection_function_table<instancing> HitGroupFunctionTable                     [[buffer(12)]],
                              visible_function_table<void(thread float4&)> MissShaderFunctionTable              [[buffer(13)]],
                              visible_function_table<void(thread float4&, device void *)> ClosestHitShaderFunctionTable              [[buffer(14)]],
                              device Material* hitMaterials [[buffer(15)]],
                              device Material* missMaterials [[buffer(16)]]
                              )
{

    if((int)tid.x < uniforms.resolution.x && (int)tid.y < uniforms.resolution.y)
    {
        // The ray to cast.
        ray ray;
        
        uint2 LaunchIndex = tid;
        
        float2 d = (((float2(LaunchIndex.xy) + 0.5f) / uniforms.resolution.xy) * 2.f - 1.f);
        
        float aspectRatio = (uniforms.resolution.x / uniforms.resolution.y);
        float4x4 viewMat = transpose( uniforms.ViewMat );

        ray.origin = uniforms.viewOriginAndTanHalfFovY.xyz;
        ray.direction = normalize((d.x * viewMat[0].xyz * uniforms.viewOriginAndTanHalfFovY.w * aspectRatio) - (d.y * viewMat[1].xyz * uniforms.viewOriginAndTanHalfFovY.w) + viewMat[2].xyz);
        // Use the MPSRayIntersection intersectionDataType property to return the
        // intersection distance for this kernel only. You don't need the other fields, so
        // you'll save memory bandwidth.
        ray.min_distance = 0.f;
        ray.max_distance = 100000000.f;

        // INTERSECTORS

        // Create an intersector to test for intersection between the ray and the geometry in the scene.
        intersector<instancing> i;
        
        // If the sample isn't using intersection functions, provide some hints to Metal for
        // better performance.
        i.assume_geometry_type(geometry_type::triangle);
        
        typename intersector<instancing>::result_type intersection;
        i.accept_any_intersection(true);
        
        float4 colorPayload = float4( 0, 0, 0, 0 );
        
        // NOTE: payload will be changed even if we accept the intersection or not!!
        intersection = i.intersect(ray, accelerationStructure[0].accelerationStructure[0], HitGroupFunctionTable, colorPayload);

        //float3 color = float3(1.0f,1.0f,1.0f);
    
        // if we don't hit anything then color it red, if we do hit something, color it green
        if (intersection.type == intersection_type::none)
        {
            MissShaderFunctionTable[0](colorPayload);
            //missIntersection = nistanceIntersector.intersect(ray, accelerationStructure[0].accelerationStructure[0], MissShaderFunctionTable, colorPayload);
        }
        else
        {
            uint offset = ((device uint*)accelerationStructure)[2 + intersection.instance_id];
            ClosestHitShaderFunctionTable[0](colorPayload, hitMaterials + 8 * offset);
        }

        RTOutput.write(colorPayload, tid);
    }
}
