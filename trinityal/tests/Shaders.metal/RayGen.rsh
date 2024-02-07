 #include "MetalDefines.h"

#include <metal_stdlib>

#define GEOMETRY_MASK_TRIANGLE 1
#define GEOMETRY_MASK_SPHERE   2
#define GEOMETRY_MASK_GEOMETRY (GEOMETRY_MASK_TRIANGLE | GEOMETRY_MASK_SPHERE)


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

// Checks if a shadow ray hit something on the way to the light source. If not, the point the
// shadow ray started from was not in shadow so it's color should be added to the output image.
kernel void mainCS(           uint2 tid                                                         [[thread_position_in_grid]],
                              constant Uniforms &uniforms                                       [[CBUFFER(0)]],
                              device const TestStruct &accelerationStructure                    [[buffer(5)]],
                              texture2d<float, access::write> RTOutput                          [[UAVT(0)]] )
{

    if((int)tid.x < uniforms.resolution.x && (int)tid.y < uniforms.resolution.y)
    {
        // The ray to cast.
        ray shadowRay;
        
        uint2 LaunchIndex = tid;
        
        float2 d = (((float2(LaunchIndex.xy) + 0.5f) / uniforms.resolution.xy) * 2.f - 1.f);
        
        float aspectRatio = (uniforms.resolution.x / uniforms.resolution.y);

        shadowRay.origin = uniforms.viewOriginAndTanHalfFovY.xyz;
        //shadowRay.direction = normalize((uv.x * uniforms.ViewMat[0].xyz * uniforms.viewOriginAndTanHalfFovY.w * aspectRatio) - (uv.y * uniforms.ViewMat[1].xyz * uniforms.viewOriginAndTanHalfFovY.w) + uniforms.ViewMat[2].xyz);
        shadowRay.direction = normalize((d.x * uniforms.ViewMat[0].xyz * uniforms.viewOriginAndTanHalfFovY.w * aspectRatio) - (d.y * uniforms.ViewMat[1].xyz * uniforms.viewOriginAndTanHalfFovY.w) + uniforms.ViewMat[2].xyz);
        // Use the MPSRayIntersection intersectionDataType property to return the
        // intersection distance for this kernel only. You don't need the other fields, so
        // you'll save memory bandwidth.
        shadowRay.min_distance = 0.f;
        shadowRay.max_distance = 10000.f;
        

        // INTERSECTORS

        // Create an intersector to test for intersection between the ray and the geometry in the scene.
        intersector<triangle_data, instancing> i;
        
        // If the sample isn't using intersection functions, provide some hints to Metal for
        // better performance.
        i.assume_geometry_type(geometry_type::triangle);
        i.force_opacity(forced_opacity::opaque);
        
        typename intersector<triangle_data, instancing>::result_type intersection;
        i.accept_any_intersection(false);
        
        intersection = i.intersect(shadowRay, accelerationStructure.accelerationStructure[0]);

        float3 color = float3(1.0f,1.0f,1.0f);

        // If there was no intersection, then the light source is visible from the original
        // intersection point.
        if (intersection.type == intersection_type::none)
        {
            color = float3(1.0, 0.0, 0.0);
        }
        else
        {
            color = float3(0.0, 0.0, 1.0);
        }

        RTOutput.write(float4(color, 1.0f), tid);
    }
}
