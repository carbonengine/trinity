// Copyright © 2026 CCP ehf.

// Compiled shaders and listings must not depend on thread timing or on leftover memory.

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <string>

#include "gtest/gtest.h"
#include "EffectCompilerMetal.h"
#if _WIN32
#include "EffectCompilerDX12.h"
#endif
#include "Macro.h"
#include "ModifiedTime.h"
#include "TesingUtils.h"

TEST( Determinism, SourceHashIgnoresLineEndings )
{
	std::error_code ignored;
	auto temp = std::filesystem::temp_directory_path( ignored );
	if( temp.empty() )
	{
		GTEST_SKIP() << "Skipping test: no temp folder";
	}
	auto folder = temp / ( "ShaderCompilerDeterminismTest-" + std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() ) );
	std::filesystem::create_directories( folder, ignored );

	// Each file has its own name: the include handler caches files by path.
	bool writeFailed = false;
	auto hash = [&]( const char* name, const char* contents ) {
		auto path = ( folder / name ).string();
		FILE* f = nullptr;
		writeFailed |= fopen_s( &f, path.c_str(), "wb" ) || fwrite( contents, strlen( contents ), 1, f ) != 1;
		if( f )
		{
			fclose( f );
		}
		return GetSourceHash( path.c_str(), {} );
	};
	auto lf = hash( "lf.fx", "float a;\nfloat b;\n" );
	auto crlf = hash( "crlf.fx", "float a;\r\nfloat b;\r\n" );
	auto actuallyDifferent = hash( "actuallyDifferent.fx", "float a;\nfloat c;\n" );
	std::filesystem::remove_all( folder, ignored );
	if( writeFailed )
	{
		GTEST_SKIP() << "Skipping test: can't write test files to " << folder.string();
	}

	EXPECT_EQ( lf, crlf ); // only differ by line breaks
	EXPECT_NE( actuallyDifferent, crlf ); // the file is actually read and hashed
}

TEST( Determinism, MetalPatchedShaderDoesNotDependOnEarlierStrings )
{
	// Metal shares one compile between permutations whose shader is the same before patching, so
	// patching must generate the same code for them. Token pasting allocates strings in the parser,
	// which used to change the numbers of the names that patching generates.
	if( !g_metalCompilerAvailable )
	{
		GTEST_SKIP() << "Skipping test: Metal compiler is not available.";
	}
	// Both sources have the same #define line, so the #line numbers in the generated code match.
	const char* withoutMacro = R"SRC(
#define CAT( a, b ) a ## b
float4 p0;
struct VsIn { float4 pos: POSITION; float2 uv: TEXCOORD0; };
struct VsOut { float4 pos: SV_Position; float2 uv: TEXCOORD0; };
VsOut vs( VsIn input ) { VsOut o; o.pos = input.pos; o.uv = input.uv; return o; }
float4 ps( VsOut input ): SV_Target { return ( p0 + p0 ) * input.uv.x; }
technique t0 { pass p0 { vertexshader = compile vs_3_0 vs(); pixelshader = compile ps_3_0 ps(); } }
)SRC";
	const char* withMacro = R"SRC(
#define CAT( a, b ) a ## b
float4 p0;
struct VsIn { float4 pos: POSITION; float2 uv: TEXCOORD0; };
struct VsOut { float4 pos: SV_Position; float2 uv: TEXCOORD0; };
VsOut vs( VsIn input ) { VsOut o; o.pos = input.pos; o.uv = input.uv; return o; }
float4 ps( VsOut input ): SV_Target { return ( CAT( p, 0 ) + CAT( p, 0 ) ) * input.uv.x; }
technique t0 { pass p0 { vertexshader = compile vs_3_0 vs(); pixelshader = compile ps_3_0 ps(); } }
)SRC";
	auto a = Compile<EffectCompilerMetal>( withoutMacro );
	auto b = Compile<EffectCompilerMetal>( withMacro );
	ASSERT_EQ( a.techniques.size(), 1u );
	ASSERT_EQ( b.techniques.size(), 1u );
	auto& stagesA = a.techniques[0].passes[0].stages;
	auto& stagesB = b.techniques[0].passes[0].stages;
	ASSERT_EQ( stagesA.size(), stagesB.size() );
	for( size_t i = 0; i < stagesA.size(); ++i )
	{
		auto& sourceA = stagesA[i].source;
		auto& sourceB = stagesB[i].source;
		EXPECT_NE( sourceA.find( "_p0000" ), std::string::npos ) << "stage " << i << ": patching should generate locally numbered names";
		auto diff = std::mismatch( sourceA.begin(), sourceA.end(), sourceB.begin(), sourceB.end() );
		if( diff.first != sourceA.end() || diff.second != sourceB.end() )
		{
			auto at = size_t( diff.first - sourceA.begin() );
			auto from = at > 200 ? at - 200 : 0;
			ADD_FAILURE() << "stage " << i << " differs at " << at << ":\n"
						  << sourceA.substr( from, 300 ) << "\n----- vs -----\n"
						  << sourceB.substr( from, 300 );
		}
	}
}

TEST( Determinism, MetalPatchedShaderDoesNotDependOnEarlierStages )
{
	// Patching numbers its names from zero for every stage. Otherwise a stage's names would depend
	// on how many names the patches of earlier stages generated, which differs between
	// permutations that share the stage's compile.
	if( !g_metalCompilerAvailable )
	{
		GTEST_SKIP() << "Skipping test: Metal compiler is not available.";
	}
	// t0's vertex shader has one input in the first source and three in the second (same line, so
	// the #line numbers match). t1 is the same in both.
	const char* fewInputs = R"SRC(
struct VsIn { float4 pos: POSITION; float2 uv: TEXCOORD0; };
struct VsOut { float4 pos: SV_Position; float2 uv: TEXCOORD0; };
float4 vsFirst( float4 pos: POSITION ): SV_Position { return pos; }
VsOut vs( VsIn input ) { VsOut o; o.pos = input.pos; o.uv = input.uv; return o; }
float4 ps( VsOut input ): SV_Target { return float4( input.uv, 0.0, 1.0 ); }
technique t0 { pass p0 { vertexshader = compile vs_3_0 vsFirst(); pixelshader = compile ps_3_0 ps(); } }
technique t1 { pass p0 { vertexshader = compile vs_3_0 vs(); pixelshader = compile ps_3_0 ps(); } }
)SRC";
	const char* moreInputs = R"SRC(
struct VsIn { float4 pos: POSITION; float2 uv: TEXCOORD0; };
struct VsOut { float4 pos: SV_Position; float2 uv: TEXCOORD0; };
float4 vsFirst( float4 pos: POSITION, float4 c: COLOR0, float2 t: TEXCOORD1 ): SV_Position { return pos + c + t.xyxy; }
VsOut vs( VsIn input ) { VsOut o; o.pos = input.pos; o.uv = input.uv; return o; }
float4 ps( VsOut input ): SV_Target { return float4( input.uv, 0.0, 1.0 ); }
technique t0 { pass p0 { vertexshader = compile vs_3_0 vsFirst(); pixelshader = compile ps_3_0 ps(); } }
technique t1 { pass p0 { vertexshader = compile vs_3_0 vs(); pixelshader = compile ps_3_0 ps(); } }
)SRC";
	auto a = Compile<EffectCompilerMetal>( fewInputs );
	auto b = Compile<EffectCompilerMetal>( moreInputs );
	ASSERT_EQ( a.techniques.size(), 2u );
	ASSERT_EQ( b.techniques.size(), 2u );
	auto& stagesA = a.techniques[1].passes[0].stages;
	auto& stagesB = b.techniques[1].passes[0].stages;
	ASSERT_EQ( stagesA.size(), stagesB.size() );
	for( size_t i = 0; i < stagesA.size(); ++i )
	{
		auto& sourceA = stagesA[i].source;
		auto& sourceB = stagesB[i].source;
		auto diff = std::mismatch( sourceA.begin(), sourceA.end(), sourceB.begin(), sourceB.end() );
		if( diff.first != sourceA.end() || diff.second != sourceB.end() )
		{
			auto at = size_t( diff.first - sourceA.begin() );
			auto from = at > 200 ? at - 200 : 0;
			ADD_FAILURE() << "t1 stage " << i << " differs at " << at << ":\n"
						  << sourceA.substr( from, 300 ) << "\n----- vs -----\n"
						  << sourceB.substr( from, 300 );
		}
	}
}

#if _WIN32
TEST( Determinism, GeneratedNamesDoNotDependOnMacroUse )
{
	// Token pasting allocates strings in the parser; that must not renumber generated names.
	// Uniform arguments make the compiler wrap the entry point in a function with a generated name.
	const char* withoutMacro = R"SRC(
float4 p0;
float4 vs(): SV_Position { return float4( 0.0, 0.0, 0.0, 1.0 ); }
float4 ps( uniform float scale ): SV_Target { return ( p0 + p0 ) * scale; }
technique t0 { pass p0 { vertexshader = compile vs_3_0 vs(); pixelshader = compile ps_3_0 ps( 2.0 ); } }
)SRC";
	const char* withMacro = R"SRC(
#define CAT( a, b ) a ## b
float4 p0;
float4 vs(): SV_Position { return float4( 0.0, 0.0, 0.0, 1.0 ); }
float4 ps( uniform float scale ): SV_Target { return ( CAT( p, 0 ) + CAT( p, 0 ) ) * scale; }
technique t0 { pass p0 { vertexshader = compile vs_3_0 vs(); pixelshader = compile ps_3_0 ps( 2.0 ); } }
)SRC";
	auto a = Compile<EffectCompilerDX12>( withoutMacro );
	auto b = Compile<EffectCompilerDX12>( withMacro );
	auto name = []( const EffectData& data ) {
		auto& source = data.techniques[0].passes[0].stages[1].source;
		auto start = source.find( "_new_symbol_" );
		return start == std::string::npos ? std::string() : source.substr( start, 16 );
	};
	ASSERT_FALSE( name( a ).empty() );
	EXPECT_EQ( name( a ), name( b ) );
}
#endif
