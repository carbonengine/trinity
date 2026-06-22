// Copyright © 2023 CCP ehf.

struct VS_OUTPUT
{
	float4 Position : SV_Position;
};

struct VS_INPUT
{
    float3 Position : POSITION;
};

VS_OUTPUT main( VS_INPUT input )
{
	VS_OUTPUT Output;
	Output.Position = float4(input.Position, 1);
	return Output;    
}
