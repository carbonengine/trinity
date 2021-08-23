struct ShadowState {
	int invertedTest;
	int alphaTestRef;
	int alphaTestFunc;
	uint clipPlaneEnabled;
	float4 clipPlanes[4];
	float4 renderTargetSize;
};

#ifdef PATCHED_SHADER

#define SHADOW_VS_OUTPUT float clipDistance [[ clip_distance ]];

#define SHADOW_VS_PART(outVs, outPos)                                                              \
    outVs.outPos.xy += shadowState.renderTargetSize.xy * outVs.outPos.w;                           \
    outVs.clipDistance = 0.0;                                                                      \
    if( shadowState.clipPlaneEnabled & 1 )                                                         \
    {                                                                                              \
        outVs.clipDistance = dot( outVs.Position, shadowState.clipPlanes[0] );                     \
    }

#define SHADOW_PS_DECL , constant ShadowState& shadowState [[ CBUFFER(10) ]]

#define SHADOW_PS_PART(outColor)                                                                   \
int __alphaValue = int( saturate( outColor.a ) * 255 + 0.5 );                                      \
if( shadowState.alphaTestFunc == 0 )                                                               \
{                                                                                                  \
if( __alphaValue * shadowState.invertedTest + shadowState.alphaTestRef < 0 ) discard_fragment();   \
}                                                                                                  \
else                                                                                               \
{                                                                                                  \
if( ( __alphaValue == shadowState.alphaTestRef ) == shadowState.invertedTest ) discard_fragment(); \
}

#else

#define SHADOW_VS_OUTPUT

#define SHADOW_VS_PART(outVs, outPos)                                                   \
    ;//outVs.outPos.xy += shadowState.renderTargetSize.xy * outVs.outPos.w;

#define SHADOW_PS_DECL

#define SHADOW_PS_PART(outColor)

#endif

