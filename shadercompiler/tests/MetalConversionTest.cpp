#include "gtest/gtest.h"
#include "EffectCompilerMetal.h"
#include "EffectData.h"
#include "TesingUtils.h"


TEST( MetalConversion, TextureIndexingWorks )
{
	const char* src = R"SRC(
Texture2D<float3> tex;

float4 vs(): SV_Position
{
	return float4( 0.0, 0.0, 0.0, 1.0 );
}

float4 ps(): SV_Target
{
	return float4( tex[int2( 0, 0 )], 1.0 );
}

technique t0
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs();	
		pixelshader = compile ps_3_0 ps();
	}
}

)SRC";

	auto data = Compile<EffectCompilerMetal>( src );
	EXPECT_NE( data.techniques[0].passes[0].stages[1].source.find( "( ( tex ).read( (uint2)( int2( 0, 0 ) ) ) ).xyz" ), std::string::npos );
}

TEST( MetalConversion, AppliesPackedModifiersToCBuffers )
{
	const char* src = R"SRC(
cbuffer cb0: register( b3 )
{
	float3 Color;
	float Intensity;
}

float3 Fog;

float4 vs(): SV_Position
{
	return float4( 0.0, 0.0, 0.0, 1.0 );
}

float4 ps(): SV_Target
{
	return float4( Color + Fog, 1.0 );
}

technique t0
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs();	
		pixelshader = compile ps_3_0 ps();
	}
}
)SRC";
	auto data = Compile<EffectCompilerMetal>( src );
	EXPECT_NE( data.techniques[0].passes[0].stages[1].source.find( "packed_float3 Color;" ), std::string::npos );
}

TEST( MetalConversion, AddsRowsToMatrixInitializers )
{
	const char* src = R"SRC(
float4 vs(): SV_Position
{
	return float4( 0.0, 0.0, 0.0, 1.0 );
}

Buffer<float3> Buff;

float4 ps(): SV_Target
{
	float3x3 m = { 
		1, 2, 3,
		4, 5, 6,
		7, 8, 9 };
	return float4( mul( m, Buff[0] ), 1.0 );
}

technique t0
{
	pass p0
	{
		vertexshader = compile vs_3_0 vs();	
		pixelshader = compile ps_3_0 ps();
	}
}
)SRC";
	auto data = Compile<EffectCompilerMetal>( src );
	// Transforms { 1, 2, 3, 4, 5, 6, 7, 8, 9 } into { { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 } }
	EXPECT_NE( data.techniques[0].passes[0].stages[1].source.find( "{ 1, 2, 3 }" ), std::string::npos );
}

TEST( MetalConversion, AppliesPackedModifiersToRtLocalBuffers )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};


cbuffer cb0: register( b3 )
{
	float3 Color;
	float Intensity;
}

[shader("miss")]
void Miss(inout HitInfo payload)
{
	payload.visibility = Intensity;
}

technique t0
{
	library p0
	{
		MissShader = compile lib_6_3 Miss();
		payloadsize = 4;
	}
}
)SRC";

	auto data = Compile<EffectCompilerMetal>( src );
	EXPECT_NE( data.techniques[0].libraries[0].source.find( "packed_float3 Color;" ), std::string::npos );
}
