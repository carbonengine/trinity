#include <metal_stdlib>

using namespace metal;
using namespace raytracing;

[[visible]] void Miss(thread float3 & color)
{
	color = float3(1.0f, 0.0f, 0.0f);
}
