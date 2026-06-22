// Copyright © 2024 CCP ehf.

#include <metal_stdlib>
#include "MetalDefines.h"
using namespace metal;
using namespace raytracing;

constant bool useIntersectionFunctions [[function_constant(0)]];

[[visible]] void Miss(thread float4 & color, __MetalHitSV, device __RtLocalMaterial*, constant float4&, constant ShaderTableT<float4, float4>&)
{
	color = float4(1.0f, 0.0f, 0.0f, 1.0f);
}
