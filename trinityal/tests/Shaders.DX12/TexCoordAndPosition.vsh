struct VS_OUTPUT
{
	float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD;
};

struct VS_INPUT
{
    float2 TexCoord : TEXCOORD;
    float3 Position : POSITION;
};

VS_OUTPUT main( VS_INPUT input )
{
	VS_OUTPUT Output;
	Output.Position = float4(input.Position, 1);
	Output.TexCoord = input.TexCoord;
	return Output;    
}
