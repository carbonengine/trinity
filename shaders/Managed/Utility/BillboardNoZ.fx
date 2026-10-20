// Copyright © 2026 CCP ehf.

#include "../../include/Carbon/System.fxh"
#include "PerFrameVSData.fxh"

struct PerObjectVSData
{
    float4x4 WorldMat : World;
} PerObjectVS : register( PEROBJECT_VS_STARTREGISTER );

Texture2D TextureMap
<
    bool SasUiVisible = true;
>;

SamplerState LinearSampler
{
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
    AddressU = Clamp;
    AddressV = Clamp;
};

struct SolidVertex
{
    float3 position : POSITION;
    float4 color : TEXCOORD0;
};

struct SolidVertexPS
{
    float4 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : TEXCOORD1;
};

SolidVertexPS SpriteVS( SolidVertex inVtx )
{
    SolidVertexPS outVtx;
     
    float3 posWorld = mul( float4( 0, 0, 0, 1 ), PerObjectVS.WorldMat ).xyz;
    float3 posView = mul( float4( posWorld, 1.0f ), PerFrameVS.ViewMat ).xyz;
    posView += mul( inVtx.position, (float3x3)PerObjectVS.WorldMat );
    outVtx.position = mul( float4( posView, 1.0f ), PerFrameVS.ProjectionMat );
    outVtx.color = inVtx.color;
    outVtx.texCoord = inVtx.position.xy * 0.5 + 0.5;

    return outVtx;
}

float4 SpritePS( SolidVertexPS inVtx ) : SV_Target
{
    return inVtx.color * TextureMap.Sample( LinearSampler, inVtx.texCoord );
}

technique Main
{
    pass P0
    {
        ZWriteEnable = False;
        ZEnable = False;
        AlphaBlendEnable = True;
        BlendOp = ADD;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        CullMode = NONE;
        
        VertexShader = compile vs_5_0 SpriteVS();
        PixelShader = compile ps_5_0 SpritePS();
    }
}