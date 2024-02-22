//Any changes made to the ray payload take effect regardless of how the intersection function returns: Rejected primitives can have side effects to memory that are
//observed by future intersection shader threads.

#include <metal_stdlib>

using namespace metal;
using namespace raytracing;

struct ReturnTypeIntersection {
    bool accept    [[accept_intersection]]; // Whether to accept or reject the intersection.
};

struct Foo {
    float3 a;
    float4 b;
};

[[intersection(triangle, triangle_data, instancing)]]
ReturnTypeIntersection ClosestHit(ray_data float3 & color [[payload]])
{
    ReturnTypeIntersection ret;
    ret.accept = false;
    
    color = float3(1.f, 0.f, 1.f);
    
    return ret;
}
