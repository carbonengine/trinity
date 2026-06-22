// Copyright © 2023 CCP ehf.

struct VS_OUTPUT
{
	float4 Position : SV_Position;
};

struct VS_INPUT
{
    float3 Position : POSITION;
    float2 Offset : POSITION8;
};

VS_OUTPUT main( VS_INPUT input )
{
	VS_OUTPUT Output;
	Output.Position = float4(input.Position + float3(input.Offset, 0), 1);
	return Output;    
}
