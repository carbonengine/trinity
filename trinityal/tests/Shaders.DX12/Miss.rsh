struct HitInfo
{
  float4 ShadedColorAndHitT;
};

[shader("miss")]
void Miss(inout HitInfo payload)
{
    payload.ShadedColorAndHitT = float4(1.f, 0.f, 0.f, -1.f);
}
