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

struct PerObjectVSData
{
	float2 position;
};

vertex VS_OUTPUT mainVS( VS_INPUT input [[ stage_in ]], constant PerObjectVSData& PerObjectVS [[ CBUFFER(0) ]] SHADOW_PS_DECL )
{
	VS_OUTPUT Output;
	Output.Position = float4(input.Position + float3( PerObjectVS.position, 0 ), 1);
	return Output;
}
