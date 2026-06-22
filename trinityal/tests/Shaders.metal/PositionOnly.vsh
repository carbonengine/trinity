// Copyright © 2023 CCP ehf.

#include "MetalDefines.h"

#include <metal_stdlib>
using namespace metal;

struct VS_OUTPUT
{
	float4 Position [[ position ]];
};

struct VS_INPUT
{
    float3 Position [[ attribute(0) ]];
};

vertex VS_OUTPUT mainVS( VS_INPUT input [[ stage_in ]] )
{
	VS_OUTPUT Output;
	Output.Position = float4(input.Position, 1);
	return Output;
}
