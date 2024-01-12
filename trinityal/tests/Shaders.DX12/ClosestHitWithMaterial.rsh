struct HitInfo
{
  float4 ShadedColorAndHitT;
};
struct Attributes 
{
	float2 uv;
};

cbuffer PerObject: register(b0, space8)
{
  float4 material;
}

Texture2D<float4> Albedo : register(t0, space8);
sampler LinearSampler : register(s0, space8);


[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
    payload.ShadedColorAndHitT = material;
}

[shader("closesthit")]
void ClosestHitWithTexture(inout HitInfo payload, Attributes attrib)
{
    payload.ShadedColorAndHitT = Albedo.SampleLevel(LinearSampler, attrib.uv, 0) * material;
}
