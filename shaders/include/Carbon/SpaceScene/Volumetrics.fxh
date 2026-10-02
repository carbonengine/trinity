// Copyright © 2026 CCP ehf.

#ifndef CARBON_SPACESCENE_VOLUMETRICS_FXH
#define CARBON_SPACESCENE_VOLUMETRICS_FXH

#include "../System.fxh"
#include "../Intersections.fxh"
#include "PerSceneData.fxh"
#include "../Math.fxh"

// Depth slices for the rendered cloud volume (see EveChildCloud2)
Texture2DArray<float4> EveSceneFogVolumeMap <bool AutoRegister = true;>;

// 3D froxel fog map and Mie environment map for volumetric effects
Texture3D<float3> EveSceneFroxelFogMap <bool AutoRegister = true; >;
TextureCube EveSceneMieEnvironmentMap <bool AutoRegister = true; >;


SamplerState EveSceneFogSampler = sampler_state
{
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
    AddressU  = Clamp;
    AddressV  = Clamp;
    AddressW  = Clamp;
};

SamplerState EveSceneFogVolumeMapSampler = sampler_state
{
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;
    AddressU  = Clamp;
    AddressV  = Clamp;
    AddressW  = Clamp;
};


float4 _CubicWeights( float x )
{
    float x2 = x * x;
    float x3 = x2 * x;

    const float4x4 m = float4x4(
        +2, -3, -3, +5,
        -2, +3, +3, +1,
        +3, -6, +0, +4,
        +1, +0, +0, +0
    ) * (1.0 / 6.0);

    return mul( m, float4( x3, x2, x, 1 ) );
}


// Sample the 3D froxel fog map with bicubic filtering in the XY plane and bilinear filtering in the Z direction.
float3 SampleEveSceneFroxelFogRaw( float2 tc, float z )
{

    //Uncomment to disable bicubic filtering
    //return EveSceneFroxelFogMap.SampleLevel(EveSceneFogSampler, float3(tc, z), 0.0f);


    //The following code does 4x4 bicubic filtering on the froxel texture, using only bilinear for Z.

    //float2 size = PerFramePS.FroxelFogData.FroxelResolution.xy;
    //float2 invSize = PerFramePS.FroxelFogData.FroxelResolution.zw;

    float width, height, dummy;
    EveSceneFroxelFogMap.GetDimensions(width, height, dummy);
    float2 size = float2(width, height);
    float2 invSize = rcp(size);
   
    tc = tc * size - 0.5;
   
    float2 floored = floor(tc);
    float2 frac = tc - floored;
   
    float2 baseTC = floored * invSize;

    float4 weightsX = _CubicWeights(frac.x);
    float4 weightsY = _CubicWeights(frac.y);


    //Since the specific spline used for the cubic weights means that all 4 weights will be over 1.0,
    //we can sample a 2x2 area with specific weights for each of the 4 samples by carefully choosing
    //the point we sample to achieve the correct ratio of weight between them, and then multiplying
    //that ratio by the total of all 4 samples.

    //The problem is simplified due to the fact that we can calculate the weights along X and Y separately.

    //These are the offsets we need to use to get the correct ratio of the texels we want to sample.
    float2 offsets0 = float2(weightsX.z / weightsX.x - 0.5, weightsY.z / weightsY.x - 0.5) * invSize + baseTC; //Top left
    float2 offsets1 = float2(weightsX.w / weightsX.y + 1.5, weightsY.w / weightsY.y + 1.5) * invSize + baseTC; //Bot right

    //This is the total weight for each of the four samples.
    float2 w = float2(weightsX.x, weightsY.x) / float2(weightsX.x + weightsX.y, weightsY.x + weightsY.y); //Top left weight
    float2 iw = 1.0 - w; //Bot right weight

    //Do the 4 samples at the correct locations and apply the correct weights.
    return
        EveSceneFroxelFogMap.SampleLevel(EveSceneFogSampler, float3(offsets0.x, offsets0.y, z), 0.0f) * (( w.x) * ( w.y)) + 
        EveSceneFroxelFogMap.SampleLevel(EveSceneFogSampler, float3(offsets1.x, offsets0.y, z), 0.0f) * ((iw.x) * ( w.y)) + 
        EveSceneFroxelFogMap.SampleLevel(EveSceneFogSampler, float3(offsets0.x, offsets1.y, z), 0.0f) * (( w.x) * (iw.y)) + 
        EveSceneFroxelFogMap.SampleLevel(EveSceneFogSampler, float3(offsets1.x, offsets1.y, z), 0.0f) * ((iw.x) * (iw.y));
}


// Get the froxel fog color for the given screen-space texture coordinates, scene depth.
float4 SampleEveSceneFroxelFog( float2 texCoords, float sceneDepth, EveSpaceSceneDataPS sceneData, bool sampleEnvironmentLighting ) 
{
 	float4 viewPositionProj = mul(float4(texCoords.x * 2.0 - 1.0, texCoords.y * -2.0 + 1.0, sceneDepth, 1.0), sceneData.ProjectionInverseMat);
    float3 viewPosition = viewPositionProj.xyz / viewPositionProj.w;



    float dist = min(sceneData.FroxelFogData.MaxDistance, length(viewPosition));
    if(sceneDepth <= 0.0)
    {
        //If we're at the far plane, override the distance to match the game's expected back clip.
        //This makes sure that the fog is the same even if the back clip is adjusted in Graphite.
        dist = sceneData.FroxelFogData.MaxDistance;
    }

	float4x4 inverseViewMatrix = transpose(sceneData.ViewInverseTransposeMat);

	float3 cameraWorldPosition = mul(float4(0.0, 0.0, 0.0, 1.0), inverseViewMatrix).xyz;

	float3 viewDirection = normalize(viewPosition);
	float3 worldDirection = mul(float4(viewDirection, 0.0), inverseViewMatrix).xyz;

	[unroll]
	for( int index = 0; index < 2; ++index)
	{
		float4 sphereData = sceneData.FroxelFogData.Planets[index];
        SphereHitInfo hitInfo = SphereRayHitTest( InitSphere( sphereData.xyz, sphereData.w ), InitRay( cameraWorldPosition, worldDirection ) );
		if( hitInfo.occurred )
		{
			dist = min( dist, hitInfo.firstIntersection.w );
		}
	}

    float normalizedZ = saturate((exp(-dist * sceneData.FroxelFogData.BaseDensity) - 1.0) / (sceneData.FroxelFogData.MaxDistanceVisibility - 1.0));

    float3 froxel = SampleEveSceneFroxelFogRaw(texCoords, normalizedZ);

	float extinction = lerp(sceneData.FroxelFogData.BackgroundVisibility, 1.0, exp(-dist * sceneData.FroxelFogData.BaseDensity));



	if( sampleEnvironmentLighting )
	{
		float3 environmentLighting = EveSceneMieEnvironmentMap.SampleLevel(EveSceneFogSampler, worldDirection, 0.0).rgb;
		
		//float shadowing = sceneDepth != 0.0 ? pow(saturate(1.0 - extinction), -sceneData.FroxelFogData.EnvironmentG * 4.0) : 1.0;
		float shadowing = 1.0; //Shadowing the environment lighting causes a mismatch between clouds and opaque objects.

		froxel += ((1.0 - extinction) * shadowing * sceneData.FroxelFogData.EnvironmentIntensity) * (environmentLighting * sceneData.FroxelFogData.FogColor);
	}
    

    return float4( froxel, extinction );
}


float4 SampleEveSceneFog( float4 pos, EveSpaceSceneDataPS sceneData )  // pos is SV_Position
{
    float t0 = sceneData.VolumetricSlices.x;
    float t1 = sceneData.VolumetricSlices.y;
    float t2 = sceneData.VolumetricSlices.z;
	float t3 = sceneData.VolumetricSlices.w;

	float i0, i1;
    float z;
    float depth = sceneData.ProjectionData.x / ( pos.z + sceneData.ProjectionData.y );
	float weight = 1;
    if( depth > t2 )
    {
		i0 = 2;
		i1 = 3;
        z = saturate( LinStep( t2, t3, depth ) );
    }
    else if( depth > t1 )
    {
		i0 = 1;
		i1 = 2;
        z = LinStep(t1, t2, depth);
    }
    else if( depth > t0 )
    {
		i0 = 0;
		i1 = 1;
        z = LinStep(t0, t1, depth);
    }
    else
    {
		i0 = 0;
		i1 = 0;
        z = LinStep(0, t0, depth);
		weight = depth / t0;
    }

	float2 texCoords = pos.xy / sceneData.RenderTargetData.xy;

	float4 color0 = EveSceneFogVolumeMap.SampleLevel( EveSceneFogVolumeMapSampler, float3( texCoords, i0 ), 0 ); 
	float4 color1 = EveSceneFogVolumeMap.SampleLevel( EveSceneFogVolumeMapSampler, float3( texCoords, i1 ), 0 ); 
    float4 color = lerp( color0, color1, z );
	color *= weight;
	return color;
}

float4 SampleEveSceneVolumetrics( float4 pos, EveSpaceSceneDataPS sceneData )  // pos is SV_Position
{
    float4 result = SampleEveSceneFog( pos, sceneData );
    float2 texCoords = pos.xy / sceneData.RenderTargetData.xy;
	float4 froxels = SampleEveSceneFroxelFog( texCoords, pos.z, sceneData, sceneData.FroxelFogData.EnvironmentIntensity > 0.0 );
	froxels.a = 1.0 - froxels.a;
	result += (1.0 - result.a) * froxels;

    return result;
}

#endif