// Copyright © 2026 CCP ehf.

#ifndef SOLIDS_FXH
#define SOLIDS_FXH

#include "../../include/Carbon/System.fxh"
#include "PerFrameVSData.fxh"

struct PerObjectVSData
{
    float4x4 WorldMat : World;
} PerObjectVS : register( PEROBJECT_VS_STARTREGISTER );

struct SolidVertex
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 color : TEXCOORD0;
};

struct SolidVertexPS
{
    float4 position : SV_Position;
    float4 color : TEXCOORD0;
    float3 viewNormal : TEXCOORD1;
    float3 vtxNormal : TEXCOORD2;
};

SolidVertexPS SpriteVS( SolidVertex inVtx )
{
    SolidVertexPS outVtx;
     
    const float4 posWorld = mul( float4( inVtx.position, 1.0f ), PerObjectVS.WorldMat );
    outVtx.position = mul( posWorld, PerFrameVS.ViewProjectionMat );
    const float4 normal = float4( PerFrameVS.ViewMat[0][2], PerFrameVS.ViewMat[1][2], PerFrameVS.ViewMat[2][2], 0.0 );
    
    outVtx.color = inVtx.color;
    outVtx.vtxNormal = normalize( mul( float4( inVtx.normal, 0.0f ), PerObjectVS.WorldMat ).xyz );
    outVtx.viewNormal = normal.xyz;

    return outVtx;
}

float4 SpritePS( SolidVertexPS inVtx ) : SV_Target
{
    float alpha = inVtx.color.a;
    float4 color = inVtx.color * saturate( dot( inVtx.vtxNormal, inVtx.viewNormal ) );
    color.a = alpha;
    return color;
}

#endif