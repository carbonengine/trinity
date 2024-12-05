#include "gtest/gtest.h"

#include "EffectCompilerMetal.h"
#include "EffectData.h"
#include "Macro.h"

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

	EffectCompilerMetal compiler;
	ASSERT_TRUE( compiler.Create() );

	EffectData data;

	ASSERT_TRUE( compiler.CompileEffect( src + 1, strlen( src ), {}, data ) );
	EXPECT_NE( data.techniques[0].passes[0].stages[1].source.find( "( ( tex ).read( (uint2)( int2( 0, 0 ) ) ) ).xyz" ), std::string::npos );
}

TEST( MetalConversion, PacksConstantBuffers )
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

	EffectCompilerMetal compiler;
	ASSERT_TRUE( compiler.Create() );

	EffectData data;

	ASSERT_TRUE( compiler.CompileEffect( src + 1, strlen( src ), {}, data ) );
	EXPECT_NE( data.techniques[0].passes[0].stages[1].source.find( "( ( tex ).read( (uint2)( int2( 0, 0 ) ) ) ).xyz" ), std::string::npos );
}


extern std::string g_metalToolsPath;

int main( int argc, char** argv )
{
#if _WIN32
	char metalToolsPath[MAX_PATH] = { 0 };
	size_t metalToolsPathSize;
	if( getenv_s( &metalToolsPathSize, metalToolsPath, "METAL_TOOLS_PATH" ) == 0 )
	{
		g_metalToolsPath = metalToolsPath;
	}
#else
	if( auto metalToolsPath = getenv( "METAL_TOOLS_PATH" ) )
	{
		g_metalToolsPath = metalToolsPath;
	}
#endif
	for( int i = 1; i < argc; ++i )
	{
		if( strcmp( argv[i], "/metal" ) == 0 )
		{
			++i;
			if( i < argc )
			{
				g_metalToolsPath = argv[i];
			}
			else
			{
				return 1;
			}
		}
	}

	::testing::InitGoogleTest( &argc, argv );
	return RUN_ALL_TESTS();
}