// Copyright © 2026 CCP ehf.

#include "gtest/gtest.h"
#include "EffectCompilerDX11.h"
#include "EffectCompilerDX12.h"
#include "EffectCompilerMetal.h"
#include "EffectData.h"
#include "TesingUtils.h"

// Forward-declared functions must compile on every API. On Metal, the compiler turns globals
// (textures, samplers, cbuffers, loose constants) into function parameters, and the forward
// declaration has to be patched to match the definition, otherwise calls made between the
// declaration and the definition fail to compile.

template <typename T>
class ForwardDeclaration : public testing::Test
{
public:
	using Compiler = T;

protected:
	void SetUp() override
	{
		if( !g_metalCompilerAvailable && std::is_same_v<T, EffectCompilerMetal> )
		{
			GTEST_SKIP() << "Skipping test: Metal compiler is not available.";
		}
	}
};

#if _WIN32
using ForwardDeclarationCompilers = ::testing::Types<EffectCompilerDX11, EffectCompilerDX12, EffectCompilerMetal>;
#else
using ForwardDeclarationCompilers = ::testing::Types<EffectCompilerMetal>;
#endif

TYPED_TEST_SUITE( ForwardDeclaration, ForwardDeclarationCompilers );

TYPED_TEST( ForwardDeclaration, WithoutGlobals )
{
	const char* src = R"SRC(
float4 vs(): SV_Position
{
	return float4( 0.0, 0.0, 0.0, 1.0 );
}

float ForwardDeclared( float a );

float ps(): SV_Target
{
	return ForwardDeclared( 1.0 );
}

float ForwardDeclared( float a )
{
	return a;
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

	EXPECT_TRUE( Compiles<typename TestFixture::Compiler>( src ) );
}

TYPED_TEST( ForwardDeclaration, UsingTextureAndSampler )
{
	const char* src = R"SRC(
float4 vs(): SV_Position
{
	return float4( 0.0, 0.0, 0.0, 1.0 );
}

float ForwardDeclared( float a );

float ps(): SV_Target
{
	return ForwardDeclared( 1.0 );
}

Texture2D Tex;
SamplerState Sampler;

float ForwardDeclared( float a )
{
	return Tex.Sample( Sampler, float2( a, a ) ).x;
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

	EXPECT_TRUE( Compiles<typename TestFixture::Compiler>( src ) );
}

TYPED_TEST( ForwardDeclaration, UsingCBuffer )
{
	const char* src = R"SRC(
float4 vs(): SV_Position
{
	return float4( 0.0, 0.0, 0.0, 1.0 );
}

float ForwardDeclared( float a );

float ps(): SV_Target
{
	return ForwardDeclared( 1.0 );
}

cbuffer cb0: register( b3 )
{
	float Constant;
}

float ForwardDeclared( float a )
{
	return a + Constant;
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

	EXPECT_TRUE( Compiles<typename TestFixture::Compiler>( src ) );
}

TYPED_TEST( ForwardDeclaration, UsingLooseGlobalConstant )
{
	const char* src = R"SRC(
float4 vs(): SV_Position
{
	return float4( 0.0, 0.0, 0.0, 1.0 );
}

float ForwardDeclared( float a );

float ps(): SV_Target
{
	return ForwardDeclared( 1.0 );
}

float Constant;

float ForwardDeclared( float a )
{
	return a + Constant;
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

	EXPECT_TRUE( Compiles<typename TestFixture::Compiler>( src ) );
}
