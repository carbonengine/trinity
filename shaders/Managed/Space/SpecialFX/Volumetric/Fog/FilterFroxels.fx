// Copyright © 2026 CCP ehf.

#include "../../../../../include/Carbon/SpaceScene/FroxelFog.fxh"

cbuffer FroxelPerObjectCB : register( b3 )
{
    FroxelPerObjectData FroxelPerObject;
}

SamplerState NearestClampSampler = sampler_state
{
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = Point;
    AddressU = Clamp;
    AddressV = Clamp;
    AddressW = Clamp;
};
SamplerState LinearClampSampler = sampler_state
{
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
    AddressU = Clamp;
    AddressV = Clamp;
    AddressW = Clamp;
};

Texture3D<float4> InputTexture;
Texture3D<float4> PreviousTexture;


RWTexture3D<float4> OutputTexture;


void accumulate( inout float4 m1, inout float4 m2, float weight, float4 froxel )
{
    m1 += froxel * weight;
    m2 += ( froxel * froxel ) * weight;
}

[numthreads( 4, 4, 4 )]
void Test( uint3 coords : SV_DispatchThreadID )
{
    if( coords.x >= FroxelPerObject.Resolution.x || coords.y >= FroxelPerObject.Resolution.y || coords.z >= FroxelPerObject.Resolution.z )
    {
        return;
    }
	
    float3 texCoords = ( float3( coords ) + 0.5 ) / float3( FroxelPerObject.Resolution );

    float4 current = InputTexture.SampleLevel( NearestClampSampler, texCoords, 0 );

    float4 m1 = 0.0;
    float4 m2 = 0.0;

    float weight = 1.0 / 7.0;

    accumulate( m1, m2, weight, current );
    accumulate( m1, m2, weight, InputTexture.SampleLevel( NearestClampSampler, texCoords, 0, int3( -1, 0, 0 ) ) );
    accumulate( m1, m2, weight, InputTexture.SampleLevel( NearestClampSampler, texCoords, 0, int3( +1, 0, 0 ) ) );
    accumulate( m1, m2, weight, InputTexture.SampleLevel( NearestClampSampler, texCoords, 0, int3( 0, -1, 0 ) ) );
    accumulate( m1, m2, weight, InputTexture.SampleLevel( NearestClampSampler, texCoords, 0, int3( 0, +1, 0 ) ) );
    accumulate( m1, m2, weight, InputTexture.SampleLevel( NearestClampSampler, texCoords, 0, int3( 0, 0, -1 ) ) );
    accumulate( m1, m2, weight, InputTexture.SampleLevel( NearestClampSampler, texCoords, 0, int3( 0, 0, +1 ) ) );

    float4 mean = m1;
    float4 std = sqrt( max( m2 - mean * mean, 0.0 ) );

    float4 result = current;

	//if(true) //Could attempt an early out here to improve performance when std is small (i.e. tight clamping --> we basically ignore the previous frame anyway)
	{

#if 1
        float layerDistance = FroxelGetLayerDistance( texCoords.z, FroxelPerObject );
        float3 viewDirection = float3( texCoords.xy * FroxelPerObject.UnprojectParams.xy + FroxelPerObject.UnprojectParams.zw, -1.0 );
        float3 viewPosition = viewDirection * ( layerDistance * rsqrt( dot( viewDirection, viewDirection ) ) );

        float3 previousViewPosition = mul( float4( viewPosition, 1.0 ), FroxelPerObject.ReprojectionMatrix ).xyz; //no need to do perspective divide
        float2 previousCoords = ( previousViewPosition.xy / -previousViewPosition.z ) * FroxelPerObject.PreviousProjectParams.xy + FroxelPerObject.PreviousProjectParams.zw;
        float previousNormalizedZ = FroxelGetNormalizedZ( length( previousViewPosition ), FroxelPerObject );

        float3 previousTexCoords = float3( previousCoords, previousNormalizedZ );
        float4 previous = PreviousTexture.SampleLevel( LinearClampSampler, previousTexCoords, 0 );
#else
			float4 previous = PreviousTexture.SampleLevel(NearestClampSampler, texCoords, 0);
#endif


        float gamma = 2.0;
        previous = clamp( previous, mean - std * gamma, mean + std * gamma );

        result = lerp( previous, current, 0.05 );

		//result = float4(abs(texCoords - previousTexCoords) * 100.0, 1.0);

		//result = float4(abs(previousTexCoords - texCoords.xy), 0, current.a);
    }

    if( any( isnan( result ) ) || any( isinf( result ) ) )
    {
        result = float4( 1, 0, 0, 1 );
    }

    OutputTexture[coords] = result;
    
}



technique Main
{
    pass p0
    {
        ComputeShader = compile cs_5_0 Test();
    }
}
