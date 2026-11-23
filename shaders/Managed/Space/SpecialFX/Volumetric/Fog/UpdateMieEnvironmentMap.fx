// Copyright © 2026 CCP ehf.

#include "../../../../../include/Carbon/System.fxh"


uint Resolution;
float2 Jitter;

float EnvironmentG;

float BlendWeight;

float Random;

TextureCube<float3> EveSpaceSceneStaticEnvMap;

SamplerState LinearClampSampler = sampler_state
{
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
    AddressU = Clamp;
    AddressV = Clamp;
    AddressW = Clamp;
};

RWTexture2DArray<float3> PrecomputedMieEnvironmentMap;


float dither( uint2 coords, float scale )
{
    float g = 1.32471795724474602596;
    return frac( Random + dot( float2( coords ), 1.0 / float2( g, g * g ) ) ) * ( 2.0 * scale ) - scale;
}


[numthreads( 8, 8, 1 )]
void Test( uint3 coords : SV_DispatchThreadID )
{
    if( coords.x >= Resolution || coords.y >= Resolution )
    {
        return;
    }

    float u = ( coords.x + 0.5 ) / Resolution * +2.0 - 1.0;
    float v = ( coords.y + 0.5 ) / Resolution * -2.0 + 1.0;
    uint face = coords.z;


    float3 direction;
	
    if( face == 0 )
        direction = float3( 1, v, -u );
    else if( face == 1 )
        direction = float3( -1, v, u );
    else if( face == 2 )
        direction = float3( u, 1, -v );
    else if( face == 3 )
        direction = float3( u, -1, v );
    else if( face == 4 )
        direction = float3( u, v, 1 );
    else
        direction = float3( -u, v, -1 );

    direction = normalize( direction );

    float3 normal = -direction;

    float3 up = float3( 1, 0, 0 );
    if( abs( dot( normal, up ) ) > 0.9 )
    {
        up = float3( 0, 1, 0 );
    }
    float3 tangent = normalize( cross( normal, up ) );
    float3 bitangent = cross( normal, tangent );

    float4x4 transform = float4x4(
		float4( tangent, 0.0 ),
		float4( bitangent, 0.0 ),
		float4( normal, 0.0 ),
		float4( 0.0, 0.0, 0.0, 1.0 )
	);

    const int ENVIRONMENT_SAMPLES = 32;
	
    const float PI = 3.14159265359;

    float g = EnvironmentG;
    float g2 = g * g;

    const float GRADIENT_STRENGTH = 32.0;
    float gradient = GRADIENT_STRENGTH / -g - GRADIENT_STRENGTH;

    float mipmap = log2( gradient );

	
    float weight = rcp( float( ENVIRONMENT_SAMPLES ) );

    float3 environmentLighting = 0.0;
    for( int i = 0; i < ENVIRONMENT_SAMPLES; i++ )
    {

        float2 randomOffset = float2( ( i + 0.5 ) / ENVIRONMENT_SAMPLES, i * 0.61803398875 );

        randomOffset = frac( randomOffset + Jitter );


        float a = ( 1.0 / ( 2.0 * g ) ) * ( 1.0 + g2 - pow( ( 1.0 - g2 ) / ( 1.0 - g + 2.0 * g * randomOffset.x ), 2.0 ) );
        float b = randomOffset.y * ( PI * 2.0 );

        float2 vertical = float2( a, sqrt( 1.0 - a * a ) );

        float2 horizontal = float2( cos( b ), sin( b ) );
        float3 direction = mul( float4( horizontal.x * vertical.y, horizontal.y * vertical.y, vertical.x, 0.0 ), transform ).xyz;
        float3 environmentColor = EveSpaceSceneStaticEnvMap.SampleLevel( LinearClampSampler, direction, 0.0 ).rgb;

        environmentLighting += environmentColor * weight;
    }

    float3 previous = PrecomputedMieEnvironmentMap[coords];

    float3 result = lerp( previous, environmentLighting, BlendWeight );
	

    if( any( isnan( result ) ) || any( isinf( result ) ) )
    {
        result = float3( 1, 0, 0 );
    }

	//None of this helps with the precision issues :(

	//result *= 1.0 + dither(coords.xy, 1.0 / 512.0);

	//float3 test = asfloat(int3(asint(result)) + int(dither(coords.xy, 512.0 * 160)));
	//result = test;

    PrecomputedMieEnvironmentMap[coords] = result;
}



technique Main
{
    pass p0
    {
        ComputeShader = compile cs_5_0 Test();
    }
}
