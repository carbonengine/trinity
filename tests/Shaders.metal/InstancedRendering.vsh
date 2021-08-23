#include "MetalDefines.h"
#include "ShadowState.fxh"

#include <metal_stdlib>
using namespace metal;

struct VS_OUTPUT
{
	float4 Position [[ position ]];
	SHADOW_VS_OUTPUT
};

struct VS_INPUT
{
    float3 Position [[ attribute(0) ]];
    float2 Offset [[ attribute(1) ]];
};

vertex VS_OUTPUT mainVS( VS_INPUT input [[ stage_in ]] SHADOW_PS_DECL )
{
	VS_OUTPUT Output;
	Output.Position = float4(input.Position + float3(input.Offset, 0), 1);

	SHADOW_VS_PART(Output, Position);

	return Output;
}
