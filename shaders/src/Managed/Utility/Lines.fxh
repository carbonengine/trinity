// Copyright © 2026 CCP ehf.

#ifndef LINES_FXH
#define LINES_FXH

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
     
    const float4 posWorld = mul( float4( inVtx.position, 1.0f ), PerObjectVS.WorldMat );
    outVtx.position = mul( posWorld, PerFrameVS.ViewProjectionMat );
    outVtx.color = inVtx.color;
    
    return outVtx;
}

float4 SpritePS( LineVertexPS inVtx ) : COLOR
{
    return inVtx.color;
}

#endif