// Copyright © 2026 CCP ehf.

#include "../include/Carbon/System.fxh"

TextureCube Texture;
float4 ColorTransform0;
float4 ColorTransform1;
float4 ColorTransform2;
float4 ColorTransform3;
float4 MinValue;
float4 MaxValue;
float UseForceMipMap;
float ForceMipMap;
float MsaaType;

float4x4 ViewMat;

SamplerState AutoMipMapTexture
{
    AddressU = Border;
    AddressV = Border;
    BorderColor = 0;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
};

SamplerState NoMipMapTexture
{
    AddressU = Border;
    AddressV = Border;
    BorderColor = 0;
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = Point;
};

struct BlitAppVertex
{
    float4 pos : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct BlitVertex
{
    float4 pos : SV_Position;
    float2 texCoord : TEXCOORD0;
};


BlitVertex BlitVS( BlitAppVertex inVtx )
{
    BlitVertex outVtx = { inVtx.pos, inVtx.texCoord };
    return outVtx;
}

float4 BlitPS( BlitVertex inVtx ) : SV_Target
{
    float2 pos = inVtx.texCoord * 2 - 1;
    float3 viewDir = float3( pos.x, -pos.y, -1 );
    float3 dir = mul( (float3x3)ViewMat, viewDir );
    float4x4 ColorTransform = { ColorTransform0, ColorTransform1, ColorTransform2, ColorTransform3 };
    float4 color;

    color = lerp( Texture.Sample( AutoMipMapTexture, dir ), Texture.SampleLevel( NoMipMapTexture, dir, ForceMipMap ), UseForceMipMap );

    color = mul( color, ColorTransform );
    color = ( color - MinValue ) / ( MaxValue - MinValue );
    return float4( color.rgb, 1 );
}

technique Main
{
    pass P0
    {
        AlphaBlendEnable = False;
        
        VertexShader = compile vs_3_0 BlitVS();
        PixelShader = compile ps_3_0 BlitPS();
    }
}
