#pragma permutation(SOURCE_TYPE, values=(SOURCE_TYPE_ARRAY, SOURCE_TYPE_TEXTURE))

#include "../../../../include/Carbon/SpaceScene/PerSceneData.fxh"
#include "../../../../include/Carbon/SpaceScene/Volumetrics.fxh"

EveSpaceSceneDataPS PerFramePS : register( PERFRAME_PS_STARTREGISTER );


float4 vs( float4 pos : POSITION ) : SV_Position
{
    return pos;
};


SamplerState EveSceneFogVolumeMapPSampler
{
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = Point;
    AddressU = Clamp;
    AddressV = Clamp;
    AddressW = Clamp;
};

float4 DepthSizes;
Texture2D<float> VolumetricDepthMap;
Texture2D<half4> SourceMap;

float GetLinearDepth( float d )
{
    return PerFramePS.ProjectionData.x / ( d + PerFramePS.ProjectionData.y );
}

float4 ps( float4 pos : SV_Position ) : SV_Target
{
    float2 uv = pos.xy / DepthSizes.xy;
    float2 invTexelSize = DepthSizes.zw;

    float centerDepth = GetLinearDepth( VolumetricDepthMap.SampleLevel( EveSceneFogVolumeMapPSampler, uv, 0 ) );

    half4 color = 0;
    half weight = 0;

#define HALFTAP    2
    const half weights[HALFTAP * 2 + 1] = { 18, 46, 90, 46, 18 };

    [unroll]
    for( float i = -HALFTAP; i <= HALFTAP; ++i )
    {
        float depth = GetLinearDepth( VolumetricDepthMap.SampleLevel( EveSceneFogVolumeMapPSampler, uv + i * invTexelSize, 0 ) );

        half w = weights[i + HALFTAP];
        float denom = 1 + min( depth, centerDepth );
        w *= half( 1 - saturate( abs( depth - centerDepth ) / denom ) );
#if SOURCE_TYPE == SOURCE_TYPE_TEXTURE
        color += w * SourceMap.SampleLevel( EveSceneFogVolumeMapPSampler, uv + i * invTexelSize, 0 );
#else
        color += w * half4( EveSceneFogVolumeMap.SampleLevel( EveSceneFogVolumeMapPSampler, float3( uv + i * invTexelSize, 3 ), 0 ) );
#endif
        weight += w;
    }
    color /= weight;

    color.a = saturate( color.a ); //half precision math means we can get over 1.0 alpha, so saturate it.

    return float4( color );
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
