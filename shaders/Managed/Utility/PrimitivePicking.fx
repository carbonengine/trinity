// Copyright © 2026 CCP ehf.

#include "../../include/Carbon/System.fxh"
#include "PerFrameVSData.fxh"

float objectId = 0;
float areaId = 0;

struct PerObjectVSData
{
    float4x4 WorldMat : World;
} PerObjectVS : register( PEROBJECT_VS_STARTREGISTER );

struct PickingAppVtx
{
    float3 pos : POSITION;
    float2 tex : TEXCOORD0;
};

struct StandardShaderVtx
{
    float4 posClip : SV_Position;
    float4 eyeDirWorld : TEXCOORD0;
    float4 texcoord01 : TEXCOORD1;
};

float4 PickingPS( StandardShaderVtx inVtx ) : SV_Target
{
    return float4( areaId + 1, objectId + 1, inVtx.eyeDirWorld.w, 0 );
}

StandardShaderVtx PickingVS( PickingAppVtx inVtx )
{
    StandardShaderVtx outVtx = (StandardShaderVtx)0;
  
    float4 posWorld = mul( float4( inVtx.pos, 1.0f ), PerObjectVS.WorldMat );
    outVtx.posClip = mul( posWorld, PerFrameVS.ViewProjectionMat );
    outVtx.eyeDirWorld = mul( posWorld, PerFrameVS.ViewMat );
    outVtx.texcoord01 = float4( inVtx.tex, 0, 0 );
    return outVtx;
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
      
        VertexShader = compile vs_3_0 PickingVS();
        PixelShader = compile ps_3_0 PickingPS();
    }
}
