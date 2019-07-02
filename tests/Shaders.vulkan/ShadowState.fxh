cbuffer DX11ShadowStateCB : register( b10 )
{
struct {
int invertedTest;
int alphaTestRef;
int alphaTestFunc;
uint clipPlaneEnabled;
float4 clipPlanes[4];
float4 renderTargetSize;
} DX11ShadowState;
}

#ifdef PATCHED_SHADER

#define SHADOW_VS_OUTPUT float clipDistance : SV_ClipDistance;

#define SHADOW_VS_PART(outVs, outPos)                                                   \
    outVs.outPos.xy += DX11ShadowState.renderTargetSize.xy * outVs.outPos.w;            \
    outVs.clipDistance = 0.0;                                                           \
    [branch] if( DX11ShadowState.clipPlaneEnabled & 1 )                                 \
    {                                                                                   \
        outVs.clipDistance.x = dot( outVs.Position, DX11ShadowState.clipPlanes[0] );    \
    }

#define SHADOW_PS_PART(outColor)                                                                    \
int __alphaValue = int( saturate( outColor.a ) * 255 + 0.5 );                                       \
[branch] if( DX11ShadowState.alphaTestFunc == 0 )                                                   \
{                                                                                                   \
if( __alphaValue * DX11ShadowState.invertedTest + DX11ShadowState.alphaTestRef < 0 ) discard;       \
}                                                                                                   \
else                                                                                                \
{                                                                                                   \
if( ( __alphaValue == DX11ShadowState.alphaTestRef ) == DX11ShadowState.invertedTest ) discard;     \
}

#else

#define SHADOW_VS_OUTPUT

#define SHADOW_VS_PART(outVs, outPos)
//                                                   \
//    outVs.outPos.xy += DX11ShadowState.renderTargetSize.xy * outVs.outPos.w;

#define SHADOW_PS_PART(outColor)

#endif

