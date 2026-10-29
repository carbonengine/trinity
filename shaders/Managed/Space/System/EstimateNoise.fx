// Copyright © 2026 CCP ehf.

#include "../../../include/Carbon/System.fxh"

Texture2D<float> Source;

// Pixel radius for noise search. It relates to dithering
static const int NOISE_ESTIMATION_RADIUS = 3;


/* Sample the signal used for the noise estimation at offset from ssC */
float Tap( int2 ssC, int2 offset )
{
    return Source[ssC + offset].r;
}


///////////////////////////////////////////////////////////////////////////////
// Estimate desired radius from the second derivative of the signal itself in source0 relative to baseline,
// which is the noisier image because it contains shadows
float EstimateNoise( int2 ssC, float2 axis )
{
    float v2 = Tap( ssC, int2( -NOISE_ESTIMATION_RADIUS * axis ) );
    float v1 = Tap( ssC, int2( ( 1 - NOISE_ESTIMATION_RADIUS ) * axis ) );

    float d2mag = 0.0;
    // The first two points are accounted for above
    for( int r = -NOISE_ESTIMATION_RADIUS + 2; r <= NOISE_ESTIMATION_RADIUS; ++r )
    {
        float v0 = Tap( ssC, int2( axis * r ) );

        // Second derivative
        float d2 = v2 - v1 * 2.0 + v0;

        d2mag += abs( d2 );

        // Shift weights in the window
        v2 = v1;
        v1 = v0;
    }

    // Scaled value by 1.5 *before* clamping to visualize going out of range
    // It is clamped again when applied.
    return clamp( sqrt( d2mag * ( 1.0 / NOISE_ESTIMATION_RADIUS ) ) * ( 1.0 / 1.5 ), 0.0, 1.0 );
}


float Hash( float n )
{
    return frac( sin( n ) * 1e4 );
}


float4 VS( float4 pos : POSITION ) : SV_Position
{
    return pos;
}


float4 PS( float4 pixelPos : SV_Position ) : SV_Target
{

    float angle = Hash( pixelPos.x + pixelPos.y * 1920.0 );

    float result = 0;
    const int N = 3;
    for( float t = 0; t < N; ++t, angle += 3.1428 / float( N ) )
    {
        float c = cos( angle ), s = sin( angle );
        result = max( EstimateNoise( int2( pixelPos.xy ), float2( c, s ) ), result );
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
