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
