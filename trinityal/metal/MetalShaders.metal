#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

// Include header shared between this Metal shader code and C code executing Metal API commands
// (Disable when converted to string)
#import "MetalShaderTypes.h"

// Vertex shader outputs and per-fragment inputs
typedef struct
{
    float4 clipSpacePosition [[position]];
    float2 texCoord;
} BlitPresentData;

vertex BlitPresentData
blitPresentVertexShader(uint vertexID [[ vertex_id ]],
                        constant BlitPresentVertex *vertexArray [[ buffer(BlitPresentInputIndexVertices) ]])

{
    BlitPresentData out;
    out.clipSpacePosition.xy = vertexArray[vertexID].position.xy;
    out.clipSpacePosition.z  = 0.0;
    out.clipSpacePosition.w  = 1.0;
    out.texCoord             = vertexArray[vertexID].texCoord;
    return out;
}

fragment float4
blitPresentFragmentShader(BlitPresentData in [[stage_in]],
                          texture2d<half> colorScaleTexture [[ texture(BlitPresentFragmentInputIndexTexture) ]])
{
    constexpr sampler textureSamplerLinear (mag_filter::linear,
                                            min_filter::linear,
                                            mip_filter::linear);
    constexpr sampler textureSamplerNearest (mag_filter::nearest,
                                             min_filter::nearest,
                                             mip_filter::nearest);

    return float4(colorScaleTexture.sample(textureSamplerLinear, in.texCoord));
}


