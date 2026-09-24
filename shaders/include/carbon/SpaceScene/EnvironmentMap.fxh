// Copyright © 2026 CCP ehf.

#ifndef CARBON_ENVIRONMENTMAP_FXH
#define CARBON_ENVIRONMENTMAP_FXH

#include "../System.fxh"


// --------------------------------------------------------------------------------------------------
// texture
// --------------------------------------------------------------------------------------------------
// Environment map (refleciton) texture set by EveSpaceScene
TextureCube EveSpaceSceneEnvMap
<
	bool AutoRegister = true;
	bool Tr2sRGB = true;
>;
SamplerState EveSpaceSceneEnvMapSampler
{
    MinFilter = Anisotropic;
    MagFilter = Linear;
    MipFilter = Linear;
    AddressU  = Wrap;
    AddressV  = Wrap;
};

// Helper function to convert PBR roughness to gloss value used for environment map lookup
float RoughnessToGloss( float roughness )
{
	const float GGX_MAX_SPEC_POWER = 28;
	const float EPSILON = 0.00052556;
	float gloss = 1.0;
	if( roughness >= EPSILON )
	{
		gloss = roughness * roughness;
		gloss *= gloss;
		gloss = ( 2.0f / gloss ) - 1.0f;
		gloss = log2( gloss ) / GGX_MAX_SPEC_POWER;
	}
	
	return gloss;
}

// Helper function to convert PBR roughness to mip level used for environment map lookup
float RoughnessToMip( TextureCube envMap, float roughness )
{
    uint width;
    uint height;
    uint mipLevels;    
    envMap.GetDimensions( width, height, mipLevels );

	return ( 1.0f - RoughnessToGloss( roughness ) ) * float( mipLevels - 1 );
}

// Helper function to convert PBR roughness to mip level used for environment map lookup for EveSpaceSceneEnvMap
float RoughnessToMip( float roughness )
{
	return RoughnessToMip( EveSpaceSceneEnvMap, roughness );
}

// Sample an environment map with given direction and surface roughness
float3 SampleEnvironmentMap( TextureCube envMap, float3 dir, float roughness )
{
	return envMap.SampleLevel( EveSpaceSceneEnvMapSampler, dir, RoughnessToMip( envMap, roughness ) ).rgb;
}

// Sample EveSpaceSceneEnvMap with given direction and surface roughness
float3 SampleEnvironmentMap( float3 dir, float roughness )
{
	return SampleEnvironmentMap( EveSpaceSceneEnvMap, dir, roughness );
}

#endif