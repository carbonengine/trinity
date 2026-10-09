// Copyright © 2026 CCP ehf.

#include "../../include/Carbon/System.fxh"
#include "PerFrameVSData.fxh"

struct PerObjectVSData
{
    float4x4 WorldMat : World;
} PerObjectVS : register( PEROBJECT_VS_STARTREGISTER );

struct LineVertex
{
    float3 position : POSITION;
    float4 color : TEXCOORD0;
};

struct LineVertexPS
{
    float4 position : SV_Position;
    float4 color : TEXCOORD0;
};

LineVertexPS SpriteVS( LineVertex inVtx )
{
    LineVertexPS outVtx;
    const float4 posCenter = mul( float4( 0.0, 0.0, 0.0, 1.0 ), PerObjectVS.WorldMat );
    const float4 normal = float4( PerFrameVS.ViewMat[0][2], PerFrameVS.ViewMat[1][2], PerFrameVS.ViewMat[2][2], 1.0 );
    
    const float4 posWorld = mul( float4( inVtx.position, 1.0f ), PerObjectVS.WorldMat );
    outVtx.position = mul( posWorld, PerFrameVS.ViewProjectionMat );
    outVtx.color = inVtx.color;
    if( dot( posWorld - posCenter, normal ) < 0.0 )
    {
        outVtx.color.a = 0.0;
    }

    return outVtx;
}
float4 SpritePS( LineVertexPS inVtx ) : SV_Target
{
    return inVtx.color;
}

technique Main
{
    pass P0
    {
        ZWriteEnable = FALSE;
        ZEnable = FALSE;
        AlphaBlendEnable = TRUE;
        BlendOp = ADD;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        AlphaTestEnable = FALSE;
        
        VertexShader = compile vs_3_0 SpriteVS();
        PixelShader = compile ps_3_0 SpritePS();
    }
}
