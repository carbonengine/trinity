// Copyright © 2024 CCP ehf.

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

TEST( GlobalInput, GlobalInputWithNoArgumentsIsAnError )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

[shader("miss")]
[globalinput]  // <- testing this line
void Miss(inout HitInfo payload)
{
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

	EXPECT_FALSE( Compiles<DefaultCompiler>( src ) );
}

static const char* s_globalInputTest = R"SRC(
struct HitInfo
{
    float visibility;
};

%s // <- declarations

[shader("miss")]
[globalinput("%s")]  // <- testing this line
void Miss(inout HitInfo payload)
{
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

TEST( GlobalInput, EmptyGlobalInputIsAccepted )
{
	EXPECT_TRUE( Compiles<DefaultCompiler>( FormatString( s_globalInputTest, "", "" ).c_str() ) );
}

TEST( GlobalInput, ExpectsVarDeclarations )
{
	EXPECT_FALSE( Compiles<DefaultCompiler>( FormatString( s_globalInputTest, "", "blah" ).c_str() ) );
}

TEST( GlobalInput, NoNumericValues )
{
	EXPECT_FALSE( Compiles<DefaultCompiler>( FormatString( s_globalInputTest, "float abc;", "abc" ).c_str() ) );
}

TEST( GlobalInput, CanSpecifyCBuffer )
{
	EXPECT_TRUE( Compiles<DefaultCompiler>( FormatString( s_globalInputTest, "cbuffer Abc: register( b3 ) { float def; };", "Abc" ).c_str() ) );
}

TEST( GlobalInput, CanSpecifySrv )
{
	EXPECT_TRUE( Compiles<DefaultCompiler>( FormatString( s_globalInputTest, "Texture2D a;", "a" ).c_str() ) );
}

TEST( GlobalInput, AllowTrailingSemicolumn )
{
	EXPECT_TRUE( Compiles<DefaultCompiler>( FormatString( s_globalInputTest, "Texture2D a;", "a;" ).c_str() ) );
}

TEST( GlobalInput, AllowsMultipleDeclarations )
{
	EXPECT_TRUE( Compiles<DefaultCompiler>( FormatString( s_globalInputTest, "Texture2D a; Texture2D b; Buffer<uint> c; SamplerState d;", "a;b; c; d" ).c_str() ) );
}

TEST( GlobalInput, MustMatchInsideLibrary )
{
	const char* templ = R"SRC(
struct HitInfo
{
    float visibility;
};
struct Attributes
{
	float2 uv;
};

Texture2D abc;
Texture2D def;

[shader("miss")]
[globalinput("abc")]
void Miss(inout HitInfo payload)
{
}
[shader("closesthit")]
[globalinput("%s")]  // <- Testing this line (vs annotation to the Miss function)
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
}
technique t0
{
	library p0
	{
		MissShader = compile lib_6_3 Miss();
		ClosestHitShader = compile lib_6_3 ClosestHit();
		payloadsize = 4;
	}
}
)SRC";

	ASSERT_TRUE( Compiles<DefaultCompiler>( FormatString( templ, "abc;" ).c_str() ) );

	EXPECT_FALSE( Compiles<DefaultCompiler>( FormatString( templ, "def;" ).c_str() ) ); // Name mismatch
	EXPECT_FALSE( Compiles<DefaultCompiler>( FormatString( templ, "abc; def;" ).c_str() ) ); // More inputs
}
