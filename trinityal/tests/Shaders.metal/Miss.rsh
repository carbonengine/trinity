#include <metal_stdlib>
#include "MetalDefines.h"
using namespace metal;
using namespace raytracing;

constant bool useIntersectionFunctions [[function_constant(0)]];

[[visible]] void Miss(thread float4 & color)
{
	color = float4(1.0f, 0.0f, 0.0f, 1.0f);
}
