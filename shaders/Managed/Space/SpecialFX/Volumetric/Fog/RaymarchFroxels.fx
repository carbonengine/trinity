// Copyright © 2026 CCP ehf.

#pragma permutation(INPUT_SOURCE, values = ( INPUT_SOURCE_INPUT_TEXTURE, INPUT_SOURCE_OUTPUT_TEXTURE ) )

#include "Fog.fxh"

Texture3D<float3> InputTexture;

RWTexture3D<float3> OutputTexture;

[numthreads( 8, 4, 1 )]
void Test( uint3 coords : SV_DispatchThreadID )
{
    if( coords.x >= FroxelPerObject.Resolution.x || coords.y >= FroxelPerObject.Resolution.y || coords.z >= FroxelPerObject.Resolution.z )
    {
        return;
    }
	
    float inverseLayers = 1.0 / FroxelPerObject.Resolution.z;

    float4 total = 0.0;

    float previousDistance = 0.0;

    for( uint i = 0; i < FroxelPerObject.Resolution.z; i++ )
    {

        float normalizedZ = ( float( i ) + 1.0 ) * inverseLayers;

        float layerDistance = min( FroxelPerObject.Far, GetLayerDistance( normalizedZ ) );
        float stepSize = layerDistance - previousDistance;
        previousDistance = layerDistance;

#if INPUT_SOURCE == INPUT_SOURCE_INPUT_TEXTURE
        float3 froxel = InputTexture[uint3( coords.xy, i )];
#else
		float3 froxel = OutputTexture[uint3(coords.xy, i)];
#endif
        float density = stepSize * FroxelPerObject.BaseDensity;
		
		//Naive integration explodes when density is large, greatly overestimating the scattering
		//This is especially bad at high fog density with the last layer, as that one goes to the far plane
		//float3 color = froxel * Scattering * density;

		//Physically correct integration makes sure we don't emit more light than is coming in.
        float3 color = froxel * FroxelPerObject.Scattering * ( 1.0 - exp( -density ) );

        float visibility = exp( -total.a );

        total += float4( color * visibility, density );

        OutputTexture[uint3( coords.xy, i )] = total.rgb;
    }

    
}



technique Main
{
    pass p0
    {
        ComputeShader = compile cs_5_0 Test();
    }
}
