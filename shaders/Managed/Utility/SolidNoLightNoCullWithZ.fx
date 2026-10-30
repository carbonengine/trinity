// Copyright © 2026 CCP ehf.

#include "Solids.fxh"

struct SolidColorVertex
{
    float3 position : POSITION;
    float4 color : TEXCOORD0;
};

struct SolidColorVertexPS
{
    float4 position : SV_Position;
    float4 color : TEXCOORD0;
};

SolidColorVertexPS SpriteSolidColorVS( SolidColorVertex inVtx )
{
    SolidColorVertexPS outVtx;
     
    const float4 posWorld = mul( float4( inVtx.position, 1.0f ), PerObjectVS.WorldMat );
    outVtx.position = mul( posWorld, PerFrameVS.ViewProjectionMat );
    
    outVtx.color = inVtx.color;
    return outVtx;
}

float4 SpriteSolidColorPS( SolidColorVertexPS inVtx ) : SV_Target
{
    return inVtx.color;
}

technique Main
{
    pass P0
    {
        ZWriteEnable = FALSE;
        ZEnable = TRUE;
        AlphaBlendEnable = TRUE;
        BlendOp = ADD;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        AlphaTestEnable = FALSE;
        CullMode = NONE;
        VertexShader = compile vs_3_0 SpriteSolidColorVS();
        PixelShader = compile ps_3_0 SpriteSolidColorPS();
    }
}
