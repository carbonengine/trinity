// Copyright © 2026 CCP ehf.

#pragma permutation(ENVIRONMENT_LIGHTING, values = ( ENVIRONMENT_LIGHTING_DISABLED, ENVIRONMENT_LIGHTING_ENABLED ) )

#include "../../../../../include/Carbon/System.fxh"
#include "../../../../../include/Carbon/SpaceScene/PerSceneData.fxh"
#include "../../../../../include/Carbon/SpaceScene/Volumetrics.fxh"
#include "../../../../../include/Carbon/SpaceScene/DepthMap.fxh"

EveSpaceSceneDataPS PerFramePS : register( PERFRAME_PS_STARTREGISTER );


SamplerState DepthMapSamplerClamp
{
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = None;
    AddressU = Clamp;
    AddressV = Clamp;
};


float4 vs( float4 pos : POSITION ) : SV_Position
{
    return pos;
};

float4 ps( float4 pos : SV_Position ) : SV_Target0
{
    float2 texCoords = pos.xy / PerFramePS.RenderTargetData.xy;
    float sceneDepth = DepthMap.SampleLevel( DepthMapSamplerClamp, texCoords, 0.0 ).r;

    float4 result;
#if ENVIRONMENT_LIGHTING == ENVIRONMENT_LIGHTING_ENABLED
    result = SampleEveSceneFroxelFog( texCoords, sceneDepth, PerFramePS, true );
#else
    result = SampleEveSceneFroxelFog( texCoords, sceneDepth, PerFramePS, false );
#endif
    return result;
}

technique Main
{
    pass P0
    {
        SrcBlend = One;
        DestBlend = SrcAlpha;
        CullMode = None;
        ZEnable = False;
        VertexShader = compile vs_5_0 vs();
        PixelShader = compile ps_5_0 ps();
    }
}
