// Copyright © 2026 CCP ehf.

#include "../../../../include/Carbon/System.fxh"
#include "../../../../include/Carbon/SpaceScene/DepthMap.fxh"


float4 vs( float4 pos : POSITION ) : SV_Position
{
    return pos;
}


float4 DepthSizes;

float4 ps( float4 pos : SV_Position ) : SV_Target
{
    return DepthMap.Load( float3( pos.xy / DepthSizes.xy * DepthSizes.zw, 0 ) );
}


technique Main
{
    pass P0
    {
        AlphaBlendEnable = false;
        CullMode = None;
        ZEnable = False;
        VertexShader = compile vs_5_0 vs();
        PixelShader = compile ps_5_0 ps();
    }
}
