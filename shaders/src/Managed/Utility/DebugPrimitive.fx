// Copyright © 2026 CCP ehf.

#include "../../include/Carbon/System.fxh"
#include "../../include/Carbon/SpaceScene/PerSceneData.fxh"

struct PerFrameVSData
{
	// matrices
    float4x4 ViewInverseTransposeMat;
    float4x4 ViewProjectionMat;
} PerFrameVS : register( PERFRAME_VS_STARTREGISTER );

EveSpaceSceneDataPS PerFramePS : register( PERFRAME_PS_STARTREGISTER );

struct AppDebugVtx
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 zPassColor : COLOR1;
    float4 zFailColor : COLOR2;
};

struct DebugVtx
{
    float4 position : SV_Position;
    float3 normal : TEXCOORD0;
    float4 color : TEXCOORD1;
    float4 colorBack : TEXCOORD2;
};

Texture2D<float> DepthMap <bool AutoRegister = true; >;
sampler DepthMapSamplerClamp = sampler_state
{
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = None;
    AddressU = Clamp;
    AddressV = Clamp;
};


float Get3dUISceneDepthOcclusion( float4 clipPos )
{
    float depth = clipPos.z;
    float sceneDepth = DepthMap.Sample( DepthMapSamplerClamp, clipPos.xy / PerFramePS.RenderTargetData.xy ).r;
    return (float)( depth > sceneDepth );
}


DebugVtx DebugVS( AppDebugVtx inVtx, uniform bool front )
{
    DebugVtx outVtx;
    outVtx.position = mul( float4( inVtx.position, 1 ), PerFrameVS.ViewProjectionMat );
    outVtx.normal = inVtx.normal;
    outVtx.color = inVtx.zPassColor;
    outVtx.colorBack = inVtx.zFailColor;
    return outVtx;
}

float4 DebugPS( DebugVtx inVtx ) : SV_Target
{
    float4 color = lerp( inVtx.colorBack, inVtx.color, Get3dUISceneDepthOcclusion( inVtx.position ) );
    color = color.bgra;
    if( any( inVtx.normal != 0 ) )
    {
        float3 normal = normalize( inVtx.normal );
        float diffuse = lerp( 0.3, 1, saturate( dot( normal, 1 ) ) );
        color.rgb *= diffuse;
    }
    return color;
}

technique Main
{
    pass P0
    {
        ZFunc = LessEqual;
        AlphaTestEnable = False;
        AlphaBlendEnable = True;
        SrcBlend = SrcAlpha;
        DestBlend = InvSrcAlpha;
        VertexShader = compile vs_3_0 DebugVS( true );
        PixelShader = compile ps_3_0 DebugPS();
    }
}