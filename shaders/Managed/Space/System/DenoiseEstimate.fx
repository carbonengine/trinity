// Copyright © 2026 CCP ehf.

#include "../../../include/Carbon/System.fxh"

Texture2D<float> Source;
SamplerState SourceSampler
{
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = None;
    AddressU  = Clamp;
    AddressV  = Clamp;
};

float4 SourceDimensions;


float4 VS( float4 pos : POSITION ) : SV_Position
{
    return pos;
}


float4 PS( float4 pixelPos: SV_Position ) : SV_Target
{
    float2 center = ( pixelPos.xy + 0.5 ) * SourceDimensions.zw;

    float dst = 0.0;
    const int R = 3;
    for( int2 delta = int2( -R, -R ); delta.y <= R; delta.y += 2 )
    {
        for( delta.x = -R; delta.x <= R; delta.x += 2 )
        {
            dst += Source.SampleLevel( SourceSampler, center + float2(delta) * SourceDimensions.zw, 0);
        }
    }
    int samples = R + 1;

    dst *= ( 1.0 / float( samples * samples ) );
    return dst;
}



technique Main
{
    pass P0
    {
        VertexShader = compile vs_5_0 VS();
        PixelShader = compile ps_5_0 PS();
    }
}
