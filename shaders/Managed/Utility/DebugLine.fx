// Copyright © 2026 CCP ehf.

#include "../../include/Carbon/System.fxh"

float4x4 WorldMat;
float4x4 ViewProjectionMat;

// -------------------------------------------------------------
// Output channels
// -------------------------------------------------------------
struct VS_OUTPUT
{
    float4 pos : SV_Position;
    float4 col : COLOR0;
};

// -------------------------------------------------------------
// vertex shader function (input channels)
// -------------------------------------------------------------
VS_OUTPUT PerVertex( float3 pos : POSITION, float4 col : COLOR0 )
{
    VS_OUTPUT Out = (VS_OUTPUT)0;

    float4 worldpos = mul( float4( pos, 1.0f ), WorldMat );
    Out.pos = mul( worldpos, ViewProjectionMat );
    Out.col = col;
    return Out;
}


// -------------------------------------------------------------
// Pixel Shader (input channels):output channel
// -------------------------------------------------------------
float4 PerPixel( VS_OUTPUT inVtx ) : SV_Target
{
    return inVtx.col;
}


// -------------------------------------------------------------
//
// -------------------------------------------------------------
technique Main
{
    pass P0
    {
        ZEnable = FALSE;
        ZWriteEnable = FALSE;
        ZFunc = LESSEQUAL;
        CullMode = NONE;
        AlphaBlendEnable = FALSE;
        AlphaTestEnable = FALSE;
        FillMode = Wireframe;
        
        // compile shaders
        VertexShader = compile vs_5_0 PerVertex();
        PixelShader = compile ps_5_0 PerPixel();
    }
}