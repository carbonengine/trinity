// Copyright © 2026 CCP ehf.

#include "gtest/gtest.h"
#include "EffectCompilerMetal.h"
#include "EffectCompilerDX12.h"
#include "EffectData.h"
#include "TesingUtils.h"

#if _WIN32
using DefaultCompiler = EffectCompilerDX12;
#else
using DefaultCompiler = EffectCompilerMetal;
#endif

namespace
{

// A minimal effect. The first %s is placed before the shaders (macros, #if checks), and the
// second is the expression the pixel shader returns.
const char* s_effect = R"SRC(
%s

float4 p0;

float4 vs(): SV_Position
{
	return float4( 0.0, 0.0, 0.0, 1.0 );
}

float4 ps(): SV_Target
{
	return %s;
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

std::string Effect( const char* prelude, const char* result = "float4( 0.0, 0.0, 0.0, 1.0 )" )
{
	return FormatString( s_effect, prelude, result );
}

}

// Guards the tests below: they rely on #error and bad code failing the compile.
TEST( Preprocessor, ErrorDirectiveFailsCompile )
{
	EXPECT_FALSE( Compiles<DefaultCompiler>( Effect( "#error \"expected\"\n" ).c_str() ) );
}

TEST( Preprocessor, UndeclaredNameFailsCompile )
{
	EXPECT_FALSE( Compiles<DefaultCompiler>( Effect( "", "UNDECLARED_NAME" ).c_str() ) );
}

TEST( Preprocessor, NestedCallInLastArgument )
{
	EXPECT_TRUE( Compiles<DefaultCompiler>( Effect( R"SRC(
#define FMA( x, y, z ) ( ( x ) * ( y ) + ( z ) )
#if FMA( 2, 3, FMA( 1, 1, 1 ) ) != 8
#error "wrong expansion"
#endif
)SRC" ).c_str() ) );
}

TEST( Preprocessor, NestedCallInFirstArgument )
{
	EXPECT_TRUE( Compiles<DefaultCompiler>( Effect( R"SRC(
#define FMA( x, y, z ) ( ( x ) * ( y ) + ( z ) )
#if FMA( FMA( 1, 1, 1 ), 3, 1 ) != 7
#error "wrong expansion"
#endif
)SRC" ).c_str() ) );
}

TEST( Preprocessor, ObjectLikeMacroAtStartOfArgument )
{
	EXPECT_TRUE( Compiles<DefaultCompiler>( Effect( R"SRC(
#define FMA( x, y, z ) ( ( x ) * ( y ) + ( z ) )
#define ONE 1
#if FMA( 2, 3, ONE + 1 ) != 8
#error "wrong expansion"
#endif
)SRC" ).c_str() ) );
}

TEST( Preprocessor, ObjectLikeMacroAtEndOfArgument )
{
	EXPECT_TRUE( Compiles<DefaultCompiler>( Effect( R"SRC(
#define FMA( x, y, z ) ( ( x ) * ( y ) + ( z ) )
#define ONE 1
#if FMA( 2, 3, 1 + ONE ) != 8
#error "wrong expansion"
#endif
)SRC" ).c_str() ) );
}

TEST( Preprocessor, ParameterPassedOnToAnotherMacro )
{
	EXPECT_TRUE( Compiles<DefaultCompiler>( Effect( R"SRC(
#define INNER( x, y ) ( ( x ) + ( y ) )
#define OUTER( a ) INNER( a, 1 )
#if OUTER( 5 ) != 6
#error "wrong expansion"
#endif
)SRC" ).c_str() ) );
}

TEST( Preprocessor, EmptyArgument )
{
	EXPECT_TRUE( Compiles<DefaultCompiler>( Effect( R"SRC(
#define PLUS_ONE( x ) x 1
#if PLUS_ONE() != 1
#error "wrong expansion"
#endif
)SRC" ).c_str() ) );
}

TEST( Preprocessor, NestedCallInShaderCode )
{
	auto data = Compile<DefaultCompiler>( Effect(
		"#define FMA( x, y, z ) ( ( x ) * ( y ) + ( z ) )\n",
		"float4( FMA( -p0.x, p0.x, FMA( -p0.y, p0.y, float( 1.0 ) ) ).xxxx )" ).c_str() );
	ASSERT_EQ( data.techniques.size(), 1u );
	EXPECT_EQ( data.techniques[0].passes[0].stages[1].source.find( "FMA" ), std::string::npos );
}

TEST( Preprocessor, CommaInsideParenthesesInArgument )
{
	auto data = Compile<DefaultCompiler>( Effect(
		"#define FMA( x, y, z ) ( ( x ) * ( y ) + ( z ) )\n",
		"float4( FMA( float2( p0.x, p0.y ), float2( 2.0, 3.0 ), p0.zw ), 0.0, 1.0 )" ).c_str() );
	ASSERT_EQ( data.techniques.size(), 1u );
	EXPECT_EQ( data.techniques[0].passes[0].stages[1].source.find( "FMA" ), std::string::npos );
}

TEST( Preprocessor, FunctionLikeMacroNameWithoutParenthesesIsNotExpanded )
{
	// A function-like macro's name not followed by '(' stays an identifier.
	auto data = Compile<DefaultCompiler>( Effect(
		"#define FMA( x, y, z ) ( ( x ) * ( y ) + ( z ) )\n"
		"static const float FMA = 0.5;\n",
		"float4( FMA, FMA, FMA, 1.0 )" ).c_str() );
	ASSERT_EQ( data.techniques.size(), 1u );
	EXPECT_NE( data.techniques[0].passes[0].stages[1].source.find( "FMA" ), std::string::npos );
}
