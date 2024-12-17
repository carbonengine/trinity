//Any changes made to the ray payload take effect regardless of how the intersection function returns: Rejected primitives can have side effects to memory that are
//observed by future intersection shader threads.

#include <metal_stdlib>
#include "MetalDefines.h"
using namespace metal;
using namespace raytracing;

constant bool useIntersectionFunctions [[function_constant(0)]];

struct Attributes 
{
	float2 uv;
};

struct ReturnTypeIntersection {
    bool accept    [[accept_intersection]]; // Whether to accept or reject the intersection.
};

[[visible]]
void ClosestHit(	thread float4 & color, device void * )
{
    color = float4(0.f, 1.f, 0.f, 1.0f);
}
