// Copyright © 2026 CCP ehf.

#pragma permutation(NORMALS_INPUT, values=(NORMALS_INPUT_NONE, NORMALS_INPUT_NORMALS))

#include "../../../include/Carbon/System.fxh"

Texture2D<float> Source;
Texture2D<float> NoiseEstimate;
Texture2D<float> DepthBuffer;
Texture2D<float3> NormalBuffer;
#define LOAD_TEX( tex, coord ) ( tex[coord] )

float DepthWeight;
float NormalWeight;
float PlaneWeight;
float PassThrough;
int Radius;

float2 Axis;

float2 ClipInfo; // x - projection[3][2], y - projection[2][2]
float4x4 ProjectionInv;

float GetLinearDepth( float d )
{
    return ClipInfo.x / ( d + ClipInfo.y );
}

float3 GetViewSpacePos( float2 xy, float depth )
{
    float2 dd = ( xy * 2.f - 1.f );
    dd.y = -dd.y;
    float4 projPos = float4( dd, depth, 1 );
    float4 viewPos = mul( projPos, ProjectionInv );
    viewPos /= viewPos.w;
    return viewPos.xyz;
}

struct TapKey
{
    float csZ;
    float3 csPosition;
    float3 normal;
};

TapKey GetTapKey( int2 C, float2 fragCoord )
{
    TapKey key = (TapKey)0;

    if( ( DepthWeight != 0.0 ) || ( PlaneWeight != 0.0 ) )
    {
        key.csZ = GetLinearDepth( LOAD_TEX( DepthBuffer, C ) );
    }

    if( PlaneWeight != 0.0 )
    {
        key.csPosition = GetViewSpacePos( fragCoord.xy, key.csZ );
    }

    return key;
}

float CalculateBilateralWeight( TapKey center, TapKey tap )
{
    float depthWeight = 1.0;
    float normalWeight = 1.0;
    float planeWeight = 1.0;

    if( DepthWeight != 0.0 )
    {
        depthWeight = max( 0.0, 1.0 - abs( tap.csZ - center.csZ ) / max( abs( tap.csZ ), 0.01 ) * DepthWeight );
    }

    return depthWeight * normalWeight * planeWeight;
}

float4 VS( float4 pos : POSITION ) : SV_Position
{
    return pos;
}

float4 PS( float4 pixelPos: SV_Position ) : SV_Target
{
    int2 ssC = int2( pixelPos.xy );

    // 3* is because the estimator produces larger values than we want to use, for visualization purposes
    float gaussianRadius = saturate( NoiseEstimate[ssC] * 10.5 ) * Radius;

    float result = 0;

    // Detect sky and noiseless pixels and reject them
    if( ( PassThrough != 1 ) && ( LOAD_TEX( DepthBuffer, ssC ) > 0 ) && ( gaussianRadius > 0.5 ) )
    {
        float sum0 = 0;
        float totalWeight = 0.0;
        TapKey key = GetTapKey( ssC, pixelPos.xy );

        for( int r = -Radius; r <= Radius; ++r )
        {
            int2 tapOffset = int2( Axis * r );
            int2 tapLoc = ssC + tapOffset;

            float gaussian = exp( -pow( float( r ) / gaussianRadius, 2 ) );
            float weight = gaussian * ( ( r == 0 ) ? 1.0 : CalculateBilateralWeight( key, GetTapKey( tapLoc, pixelPos.xy + float2(tapOffset) ) ) );
            sum0 += Source[tapLoc].r * weight;
            totalWeight += weight;
        }

        // totalWeight >= gaussian[0], so no division by zero here
        result = sum0 / totalWeight;
    }
    else
    {
        // No denoising needed
        result = Source[ssC];
    }
    return result;
}

technique Main
{
    pass P0
    {
        VertexShader = compile vs_5_0 VS();
        PixelShader = compile ps_5_0 PS();
    }
}
