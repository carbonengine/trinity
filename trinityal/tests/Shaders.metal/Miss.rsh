#include <metal_stdlib>

using namespace metal;
using namespace raytracing;

[[intersection(triangle, triangle_data, instancing)]]
bool Miss(ray_data float3 & color [[payload]])
{
    color = float3(1.f, 0.f, 0.f);
    
    return 1;
}
