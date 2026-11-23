// Copyright © 2026 CCP ehf.

#include "../../include/Carbon/System.fxh"

struct PerFrameVSData
{
	// matrices
    float4x4 ViewInverseTransposeMat;
    float4x4 ViewProjectionMat;
} PerFrameVS : register( PERFRAME_VS_STARTREGISTER );

struct AppDebugVtx
{
    float3 position : POSITION;
    float2 index : TEXCOORD0;
};

struct DebugVtx
{
    float4 position : SV_Position;
    float4 output : TEXCOORD0;
};


DebugVtx DebugVS( AppDebugVtx inVtx )
{
    DebugVtx outVtx;
    outVtx.position = mul( float4( inVtx.position, 1 ), PerFrameVS.ViewProjectionMat );
    outVtx.output = float4( inVtx.index, outVtx.position.zw );
    return outVtx;
}

uint PickingPointerLowBits;
uint PickingPointerHighBits;
uint PickingArea;

uint4 DebugPS( DebugVtx inVtx ) : COLOR
{
    return uint4( PickingPointerLowBits, PickingPointerHighBits, PickingArea, 0 );
}

technique Main
{
    pass P0
    {
        ZEnable = True;
        ZWriteEnable = True;
        ZFunc = LessEqual;
        VertexShader = compile vs_3_0 DebugVS();
        PixelShader = compile ps_3_0 DebugPS();
    }
}