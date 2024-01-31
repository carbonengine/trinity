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
 
    // The ray to cast.
	ray shadowRay;

	
	// map pixel coordinates to -1..1
	float2 pixel = (float2)tid;
    
     // maybe it needs to be (uniforms.resolution.x + 0.5f
	float2 uv = float2(pixel) / float2(uniforms.resolution.x, uniforms.resolution.y);
	float aspectRatio = (uniforms.resolution.x / uniforms.resolution.y);
    
	uv = uv * 2.0f - 1.0f;

    shadowRay.origin = uniforms.viewOriginAndTanHalfFovY.xyz;
    shadowRay.direction = normalize((uv.x * uniforms.ViewMat[0].xyz * uniforms.viewOriginAndTanHalfFovY.w * aspectRatio) - (uv.y * uniforms.ViewMat[1].xyz * uniforms.viewOriginAndTanHalfFovY.w) + uniforms.ViewMat[2].xyz);
        
    // Use the MPSRayIntersection intersectionDataType property to return the
    // intersection distance for this kernel only. You don't need the other fields, so
    // you'll save memory bandwidth.
    
    // Create an intersector to test for intersection between the ray and the geometry in the scene.
    intersector<triangle_data, instancing> i;
    
    // If the sample isn't using intersection functions, provide some hints to Metal for
    // better performance.
    i.assume_geometry_type(geometry_type::triangle);
    i.force_opacity(forced_opacity::opaque);
   
	typename intersector<triangle_data, instancing>::result_type intersection;
 
    i.accept_any_intersection(true);
    
	intersection = i.intersect(shadowRay, accelerationStructure.accelerationStructure[0], GEOMETRY_MASK_GEOMETRY);
    
    // If there was no intersection, then the light source is visible from the original
    // intersection point.
    if (intersection.type == intersection_type::none)
    {
        float3 color = float3(1.0, 0.0, 0.0);
        RTOutput.write(float4(color, 1.0f), tid);
    }
    else
    {
        float3 color = float3(0.0, 1.0, 0.0);
        RTOutput.write(float4(color, 1.0f), tid);
    }
}
