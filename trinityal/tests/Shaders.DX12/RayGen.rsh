cbuffer cb0: register( b0 )
{
  float4x4 ViewMat;
  float4 viewOriginAndTanHalfFovY;
  float2 resolution;
}

RaytracingAccelerationStructure Scene : register( t1 );
RWTexture2D<float4> RTOutput : register( u0 );

struct HitInfo
{
  float4 ShadedColorAndHitT;
};

[shader("raygeneration")]
void RayGen()
{
  uint2 LaunchIndex = DispatchRaysIndex().xy;
  uint2 LaunchDimensions = DispatchRaysDimensions().xy;

  float2 d = (((LaunchIndex.xy + 0.5f) / resolution.xy) * 2.f - 1.f);
  float aspectRatio = (resolution.x / resolution.y);

  // Setup the ray
  RayDesc ray;
  ray.Origin = viewOriginAndTanHalfFovY.xyz;
  ray.Direction = normalize((d.x * ViewMat[0].xyz * viewOriginAndTanHalfFovY.w * aspectRatio) - (d.y * ViewMat[1].xyz * viewOriginAndTanHalfFovY.w) + ViewMat[2].xyz);
  ray.TMin = 0.1f;
  ray.TMax = 1000.f;	

  // Trace the ray
  HitInfo payload;
  payload.ShadedColorAndHitT = float4(0.f, 0.f, 0.f, 0.f);

  TraceRay(
    Scene,
    RAY_FLAG_NONE,
    0xFF,
    0,
    0,
    0,
    ray,
    payload);

  RTOutput[LaunchIndex.xy] = float4(payload.ShadedColorAndHitT.rgb + float3(0,ViewMat[0].x,0), 1.f);
}