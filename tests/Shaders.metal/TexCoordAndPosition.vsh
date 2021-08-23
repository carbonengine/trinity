#include "MetalDefines.h"
#include "ShadowState.fxh"

#include <metal_stdlib>
using namespace metal;

struct VS_OUTPUT
{
	float4 Position [[ position ]];
    float2 TexCoord;
	SHADOW_VS_OUTPUT
};

struct VS_INPUT
{
    float2 TexCoord [[ attribute(0) ]];
    float3 Position [[ attribute(1) ]];
};

vertex VS_OUTPUT mainVS( VS_INPUT input [[ stage_in ]] SHADOW_PS_DECL )
{
	VS_OUTPUT Output;
	Output.Position = float4(input.Position, 1);
	Output.TexCoord = input.TexCoord;
    SHADOW_VS_PART(Output, Position);
	return Output;
}
