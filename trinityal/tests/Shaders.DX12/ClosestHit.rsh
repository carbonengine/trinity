// Copyright © 2024 CCP ehf.

struct HitInfo
{
  float4 ShadedColorAndHitT;
};
struct Attributes 
{
	float2 uv;
};

[shader("closesthit")]
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
    payload.ShadedColorAndHitT = float4(0.f, 0.f, 1.f, -1.f);
}
