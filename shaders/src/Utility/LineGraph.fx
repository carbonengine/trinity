// Copyright © 2026 CCP ehf.

#include "../include/Carbon/System.fxh"

float4 lineGraphColor;
float lineGraphScale;

struct AppVertex
{
    float2 pos : POSITION;
};

struct ShaderVertex
{
    float4 posClip : SV_Position;
};

ShaderVertex VS( AppVertex inVtx )
{
    ShaderVertex outVtx;
    outVtx.posClip.x = inVtx.pos.x;
    outVtx.posClip.y = inVtx.pos.y * lineGraphScale * 2.0f - 1.0f;
    outVtx.posClip.z = 0;
    outVtx.posClip.w = 1;
    return outVtx;
}

float4 PS( ShaderVertex inVtx ) : SV_Target
{
    return lineGraphColor;
}

technique Main
{
    pass P0
    {
        ZEnable = False;
        AlphaBlendEnable = False;
        AlphaTestEnable = False;
        VertexShader = compile vs_3_0 VS();
        PixelShader = compile ps_3_0 PS();
    }
}
