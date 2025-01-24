struct VS_OUTPUT
{
	float4 Position : SV_Position;
};

struct VS_INPUT
{
    float3 Position : POSITION;
};

cbuffer cb0: register(b0)
{
    struct PerObjectVSData
    {
        float2 position;
    } PerObjectVS;
}

VS_OUTPUT main( VS_INPUT input )
{
	VS_OUTPUT Output;
	Output.Position = float4(input.Position + float3( PerObjectVS.position, 0 ), 1);
	return Output;    
}
