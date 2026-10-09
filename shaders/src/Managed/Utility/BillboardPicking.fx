// Copyright © 2026 CCP ehf.

#include "../../include/Carbon/System.fxh"
#include "PerFrameVSData.fxh"

struct PerObjectVSData
{
    float4x4 WorldMat : World;
} PerObjectVS : register( PEROBJECT_VS_STARTREGISTER );

struct SolidVertex
{
    float3 position : POSITION;
};

struct SolidVertexPS
{
    float4 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

// selected.AddTriangle((-1, -1, 0), (1,1,1,1), (1, -1, 0), (1,1,1,1), (-1, 1, 0), (1,1,1,1))

SolidVertexPS SpriteVS( SolidVertex inVtx )
{
    SolidVertexPS outVtx;
     
    float3 posWorld = mul( float4( 0, 0, 0, 1 ), PerObjectVS.WorldMat ).xyz;
    float3 posView = mul( float4( posWorld, 1.0f ), PerFrameVS.ViewMat ).xyz;
    posView += mul( inVtx.position, (float3x3)PerObjectVS.WorldMat );
    outVtx.position = mul( float4( posView, 1.0f ), PerFrameVS.ProjectionMat );
    outVtx.texCoord = inVtx.position.xy * 0.5 + 0.5;

    return outVtx;
}

float objectId = 0;
float areaId = 0;

float4 SpritePS( SolidVertexPS inVtx ) : SV_Target
{
    return float4( areaId + 1, objectId + 1, 1, 0 );
}

technique Main
{
    pass P0
    {
        ZWriteEnable = TRUE;
        ZEnable = TRUE;
        ZFunc = LESSEQUAL;
        AlphaBlendEnable = FALSE;
        AlphaTestEnable = FALSE;
        
        VertexShader = compile vs_5_0 SpriteVS();
        PixelShader = compile ps_5_0 SpritePS();
    }
}