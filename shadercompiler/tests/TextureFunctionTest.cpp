// Copyright © 2026 CCP ehf.

#include "gtest/gtest.h"
#include "EffectCompilerDX11.h"
#include "EffectCompilerDX12.h"
#include "EffectCompilerMetal.h"
#include "EffectData.h"
#include "TesingUtils.h"

template <typename T>
class TextureFunctions : public testing::Test
{
public:
	using Compiler = T;
};

#if _WIN32
using TextureFunctionCompilers = ::testing::Types<EffectCompilerDX11, EffectCompilerDX12, EffectCompilerMetal>;
#else
using TextureFunctionCompilers = ::testing::Types<EffectCompilerMetal>;
#endif

TYPED_TEST_SUITE( TextureFunctions, TextureFunctionCompilers );

// Calls every texture sampling, gather, load and GetDimensions function the Metal conversion
// supports, plus indexed reads, on float textures with 1 to 4 components. Each function is called
// on every texture type it applies to, with and without an offset where it takes one. Every test
// function returns a 4-component vector and adds gather results whole, so a gather that returns
// too few components fails to compile. The DX compilers pass these calls through unchanged; the
// Metal compiler rewrites them, so this mostly checks that the rewritten calls are valid Metal.
//
// The int and uint tests are separate because Metal limits the number of textures per shader
// stage. Metal also has no vector constructors that convert mixed component types, such as
// int3( float2, int ), so coordinates are converted one constructor at a time.
TYPED_TEST( TextureFunctions, FloatFunctionsCompile )
{
	if( !g_metalCompilerAvailable && std::is_same_v<typename TestFixture::Compiler, EffectCompilerMetal> )
	{
		GTEST_SKIP() << "Skipping test: Metal compiler is not available.";
	}

	// Split into several literals, as MSVC limits the length of a single string literal.
	const char* src = R"SRC(
SamplerState Sampler;
SamplerComparisonState ComparisonSampler;
)SRC"
					  R"SRC(
Texture2D<float> Texture2DFloat;
Texture2DArray<float> Texture2DArrayFloat;
Texture3D<float> Texture3DFloat;
TextureCube<float> TextureCubeFloat;
TextureCubeArray<float> TextureCubeArrayFloat;
DepthTexture2D<float> DepthTexture2DFloat;

float4 TestFloat( float4 pos )
{
	float2 uv = pos.xy;
	float3 uvw = float3( uv, 0.5 );
	float3 dir = normalize( uvw );
	float4 dirArray = float4( dir, 1.0 );
	int2 offset = int2( 1, -1 );
	float4 result = float4( 0.0, 0.0, 0.0, 0.0 );

	// Sample
	result.x += Texture2DFloat.Sample( Sampler, uv );
	result.x += Texture2DFloat.Sample( Sampler, uv, offset );
	result.x += Texture2DArrayFloat.Sample( Sampler, uvw );
	result.x += Texture2DArrayFloat.Sample( Sampler, uvw, offset );
	result.x += Texture3DFloat.Sample( Sampler, uvw );
	result.x += TextureCubeFloat.Sample( Sampler, dir );

	// SampleBias
	result.x += Texture2DFloat.SampleBias( Sampler, uv, 0.5 );
	result.x += Texture2DArrayFloat.SampleBias( Sampler, uvw, 0.5 );
	result.x += Texture3DFloat.SampleBias( Sampler, uvw, 0.5 );
	result.x += TextureCubeFloat.SampleBias( Sampler, dir, 0.5 );

	// SampleLevel
	result.x += Texture2DFloat.SampleLevel( Sampler, uv, 1.0 );
	result.x += Texture2DArrayFloat.SampleLevel( Sampler, uvw, 1.0 );
	result.x += Texture3DFloat.SampleLevel( Sampler, uvw, 1.0 );
	result.x += TextureCubeFloat.SampleLevel( Sampler, dir, 1.0 );

	// SampleGrad
	result.x += Texture2DFloat.SampleGrad( Sampler, uv, ddx( uv ), ddy( uv ) );
	result.x += Texture2DArrayFloat.SampleGrad( Sampler, uvw, ddx( uv ), ddy( uv ) );
	result.x += Texture3DFloat.SampleGrad( Sampler, uvw, ddx( uvw ), ddy( uvw ) );
	result.x += TextureCubeFloat.SampleGrad( Sampler, dir, ddx( dir ), ddy( dir ) );

	// SampleCmp / SampleCmpLevelZero
	result.x += DepthTexture2DFloat.SampleCmp( ComparisonSampler, uv, 0.5 );
	result.x += DepthTexture2DFloat.SampleCmp( ComparisonSampler, uv, 0.5, offset );
	result.x += DepthTexture2DFloat.SampleCmpLevelZero( ComparisonSampler, uv, 0.5 );
	result.x += DepthTexture2DFloat.SampleCmpLevelZero( ComparisonSampler, uv, 0.5, offset );

	// Gather
	result += Texture2DFloat.Gather( Sampler, uv );
	result += Texture2DFloat.Gather( Sampler, uv, offset );
	result += Texture2DArrayFloat.Gather( Sampler, uvw );
	result += Texture2DArrayFloat.Gather( Sampler, uvw, offset );
	result += TextureCubeFloat.Gather( Sampler, dir );
	result += TextureCubeArrayFloat.Gather( Sampler, dirArray );

	// GatherRed
	result += Texture2DFloat.GatherRed( Sampler, uv );
	result += Texture2DFloat.GatherRed( Sampler, uv, offset );
	result += Texture2DArrayFloat.GatherRed( Sampler, uvw );
	result += Texture2DArrayFloat.GatherRed( Sampler, uvw, offset );
	result += TextureCubeFloat.GatherRed( Sampler, dir );
	result += TextureCubeArrayFloat.GatherRed( Sampler, dirArray );

	// Load
	result.x += Texture2DFloat.Load( int3( int2( pos.xy ), 0 ) );
	result.x += Texture2DArrayFloat.Load( int4( int2( pos.xy ), 1, 0 ) );
	result.x += Texture3DFloat.Load( int4( int3( pos.xyz ), 0 ) );

	// Indexed reads
	result.x += Texture2DFloat[uint2( pos.xy )];
	result.x += Texture2DArrayFloat[uint3( uint2( pos.xy ), 1u )];
	result.x += Texture3DFloat[uint3( pos.xyz )];

	// GetDimensions
	uint width, height, depth, levels;
	uint dimensions = 0u;
	Texture2DFloat.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeFloat.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeFloat.GetDimensions( 1u, width, height, levels );
	dimensions += width + height + levels;
	Texture3DFloat.GetDimensions( width, height, depth );
	dimensions += width + height + depth;
	float d = float( dimensions );
	result += float4( d, d, d, d );

	return result;
}
)SRC"
					  R"SRC(
Texture2D<float2> Texture2DFloat2;
Texture2DArray<float2> Texture2DArrayFloat2;
Texture3D<float2> Texture3DFloat2;
TextureCube<float2> TextureCubeFloat2;
TextureCubeArray<float2> TextureCubeArrayFloat2;

float4 TestFloat2( float4 pos )
{
	float2 uv = pos.xy;
	float3 uvw = float3( uv, 0.5 );
	float3 dir = normalize( uvw );
	float4 dirArray = float4( dir, 1.0 );
	int2 offset = int2( 1, -1 );
	float4 result = float4( 0.0, 0.0, 0.0, 0.0 );

	// Sample
	result.xy += Texture2DFloat2.Sample( Sampler, uv );
	result.xy += Texture2DFloat2.Sample( Sampler, uv, offset );
	result.xy += Texture2DArrayFloat2.Sample( Sampler, uvw );
	result.xy += Texture2DArrayFloat2.Sample( Sampler, uvw, offset );
	result.xy += Texture3DFloat2.Sample( Sampler, uvw );
	result.xy += TextureCubeFloat2.Sample( Sampler, dir );

	// SampleBias
	result.xy += Texture2DFloat2.SampleBias( Sampler, uv, 0.5 );
	result.xy += Texture2DArrayFloat2.SampleBias( Sampler, uvw, 0.5 );
	result.xy += Texture3DFloat2.SampleBias( Sampler, uvw, 0.5 );
	result.xy += TextureCubeFloat2.SampleBias( Sampler, dir, 0.5 );

	// SampleLevel
	result.xy += Texture2DFloat2.SampleLevel( Sampler, uv, 1.0 );
	result.xy += Texture2DArrayFloat2.SampleLevel( Sampler, uvw, 1.0 );
	result.xy += Texture3DFloat2.SampleLevel( Sampler, uvw, 1.0 );
	result.xy += TextureCubeFloat2.SampleLevel( Sampler, dir, 1.0 );

	// SampleGrad
	result.xy += Texture2DFloat2.SampleGrad( Sampler, uv, ddx( uv ), ddy( uv ) );
	result.xy += Texture2DArrayFloat2.SampleGrad( Sampler, uvw, ddx( uv ), ddy( uv ) );
	result.xy += Texture3DFloat2.SampleGrad( Sampler, uvw, ddx( uvw ), ddy( uvw ) );
	result.xy += TextureCubeFloat2.SampleGrad( Sampler, dir, ddx( dir ), ddy( dir ) );

	// Gather
	result += Texture2DFloat2.Gather( Sampler, uv );
	result += Texture2DFloat2.Gather( Sampler, uv, offset );
	result += Texture2DArrayFloat2.Gather( Sampler, uvw );
	result += Texture2DArrayFloat2.Gather( Sampler, uvw, offset );
	result += TextureCubeFloat2.Gather( Sampler, dir );
	result += TextureCubeArrayFloat2.Gather( Sampler, dirArray );

	// GatherRed
	result += Texture2DFloat2.GatherRed( Sampler, uv );
	result += Texture2DFloat2.GatherRed( Sampler, uv, offset );
	result += Texture2DArrayFloat2.GatherRed( Sampler, uvw );
	result += Texture2DArrayFloat2.GatherRed( Sampler, uvw, offset );
	result += TextureCubeFloat2.GatherRed( Sampler, dir );
	result += TextureCubeArrayFloat2.GatherRed( Sampler, dirArray );

	// GatherGreen
	result += Texture2DFloat2.GatherGreen( Sampler, uv );
	result += Texture2DFloat2.GatherGreen( Sampler, uv, offset );
	result += Texture2DArrayFloat2.GatherGreen( Sampler, uvw );
	result += Texture2DArrayFloat2.GatherGreen( Sampler, uvw, offset );
	result += TextureCubeFloat2.GatherGreen( Sampler, dir );
	result += TextureCubeArrayFloat2.GatherGreen( Sampler, dirArray );

	// Load
	result.xy += Texture2DFloat2.Load( int3( int2( pos.xy ), 0 ) );
	result.xy += Texture2DArrayFloat2.Load( int4( int2( pos.xy ), 1, 0 ) );
	result.xy += Texture3DFloat2.Load( int4( int3( pos.xyz ), 0 ) );

	// Indexed reads
	result.xy += Texture2DFloat2[uint2( pos.xy )];
	result.xy += Texture2DArrayFloat2[uint3( uint2( pos.xy ), 1u )];
	result.xy += Texture3DFloat2[uint3( pos.xyz )];

	// GetDimensions
	uint width, height, depth, levels;
	uint dimensions = 0u;
	Texture2DFloat2.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeFloat2.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeFloat2.GetDimensions( 1u, width, height, levels );
	dimensions += width + height + levels;
	Texture3DFloat2.GetDimensions( width, height, depth );
	dimensions += width + height + depth;
	float d = float( dimensions );
	result += float4( d, d, d, d );

	return result;
}
)SRC"
					  R"SRC(
Texture2D<float3> Texture2DFloat3;
Texture2DArray<float3> Texture2DArrayFloat3;
Texture3D<float3> Texture3DFloat3;
TextureCube<float3> TextureCubeFloat3;
TextureCubeArray<float3> TextureCubeArrayFloat3;

float4 TestFloat3( float4 pos )
{
	float2 uv = pos.xy;
	float3 uvw = float3( uv, 0.5 );
	float3 dir = normalize( uvw );
	float4 dirArray = float4( dir, 1.0 );
	int2 offset = int2( 1, -1 );
	float4 result = float4( 0.0, 0.0, 0.0, 0.0 );

	// Sample
	result.xyz += Texture2DFloat3.Sample( Sampler, uv );
	result.xyz += Texture2DFloat3.Sample( Sampler, uv, offset );
	result.xyz += Texture2DArrayFloat3.Sample( Sampler, uvw );
	result.xyz += Texture2DArrayFloat3.Sample( Sampler, uvw, offset );
	result.xyz += Texture3DFloat3.Sample( Sampler, uvw );
	result.xyz += TextureCubeFloat3.Sample( Sampler, dir );

	// SampleBias
	result.xyz += Texture2DFloat3.SampleBias( Sampler, uv, 0.5 );
	result.xyz += Texture2DArrayFloat3.SampleBias( Sampler, uvw, 0.5 );
	result.xyz += Texture3DFloat3.SampleBias( Sampler, uvw, 0.5 );
	result.xyz += TextureCubeFloat3.SampleBias( Sampler, dir, 0.5 );

	// SampleLevel
	result.xyz += Texture2DFloat3.SampleLevel( Sampler, uv, 1.0 );
	result.xyz += Texture2DArrayFloat3.SampleLevel( Sampler, uvw, 1.0 );
	result.xyz += Texture3DFloat3.SampleLevel( Sampler, uvw, 1.0 );
	result.xyz += TextureCubeFloat3.SampleLevel( Sampler, dir, 1.0 );

	// SampleGrad
	result.xyz += Texture2DFloat3.SampleGrad( Sampler, uv, ddx( uv ), ddy( uv ) );
	result.xyz += Texture2DArrayFloat3.SampleGrad( Sampler, uvw, ddx( uv ), ddy( uv ) );
	result.xyz += Texture3DFloat3.SampleGrad( Sampler, uvw, ddx( uvw ), ddy( uvw ) );
	result.xyz += TextureCubeFloat3.SampleGrad( Sampler, dir, ddx( dir ), ddy( dir ) );

	// Gather
	result += Texture2DFloat3.Gather( Sampler, uv );
	result += Texture2DFloat3.Gather( Sampler, uv, offset );
	result += Texture2DArrayFloat3.Gather( Sampler, uvw );
	result += Texture2DArrayFloat3.Gather( Sampler, uvw, offset );
	result += TextureCubeFloat3.Gather( Sampler, dir );
	result += TextureCubeArrayFloat3.Gather( Sampler, dirArray );

	// GatherRed
	result += Texture2DFloat3.GatherRed( Sampler, uv );
	result += Texture2DFloat3.GatherRed( Sampler, uv, offset );
	result += Texture2DArrayFloat3.GatherRed( Sampler, uvw );
	result += Texture2DArrayFloat3.GatherRed( Sampler, uvw, offset );
	result += TextureCubeFloat3.GatherRed( Sampler, dir );
	result += TextureCubeArrayFloat3.GatherRed( Sampler, dirArray );

	// GatherGreen
	result += Texture2DFloat3.GatherGreen( Sampler, uv );
	result += Texture2DFloat3.GatherGreen( Sampler, uv, offset );
	result += Texture2DArrayFloat3.GatherGreen( Sampler, uvw );
	result += Texture2DArrayFloat3.GatherGreen( Sampler, uvw, offset );
	result += TextureCubeFloat3.GatherGreen( Sampler, dir );
	result += TextureCubeArrayFloat3.GatherGreen( Sampler, dirArray );

	// GatherBlue
	result += Texture2DFloat3.GatherBlue( Sampler, uv );
	result += Texture2DFloat3.GatherBlue( Sampler, uv, offset );
	result += Texture2DArrayFloat3.GatherBlue( Sampler, uvw );
	result += Texture2DArrayFloat3.GatherBlue( Sampler, uvw, offset );
	result += TextureCubeFloat3.GatherBlue( Sampler, dir );
	result += TextureCubeArrayFloat3.GatherBlue( Sampler, dirArray );

	// Load
	result.xyz += Texture2DFloat3.Load( int3( int2( pos.xy ), 0 ) );
	result.xyz += Texture2DArrayFloat3.Load( int4( int2( pos.xy ), 1, 0 ) );
	result.xyz += Texture3DFloat3.Load( int4( int3( pos.xyz ), 0 ) );

	// Indexed reads
	result.xyz += Texture2DFloat3[uint2( pos.xy )];
	result.xyz += Texture2DArrayFloat3[uint3( uint2( pos.xy ), 1u )];
	result.xyz += Texture3DFloat3[uint3( pos.xyz )];

	// GetDimensions
	uint width, height, depth, levels;
	uint dimensions = 0u;
	Texture2DFloat3.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeFloat3.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeFloat3.GetDimensions( 1u, width, height, levels );
	dimensions += width + height + levels;
	Texture3DFloat3.GetDimensions( width, height, depth );
	dimensions += width + height + depth;
	float d = float( dimensions );
	result += float4( d, d, d, d );

	return result;
}
)SRC"
					  R"SRC(
Texture2D<float4> Texture2DFloat4;
Texture2DArray<float4> Texture2DArrayFloat4;
Texture3D<float4> Texture3DFloat4;
TextureCube<float4> TextureCubeFloat4;
TextureCubeArray<float4> TextureCubeArrayFloat4;

float4 TestFloat4( float4 pos )
{
	float2 uv = pos.xy;
	float3 uvw = float3( uv, 0.5 );
	float3 dir = normalize( uvw );
	float4 dirArray = float4( dir, 1.0 );
	int2 offset = int2( 1, -1 );
	float4 result = float4( 0.0, 0.0, 0.0, 0.0 );

	// Sample
	result += Texture2DFloat4.Sample( Sampler, uv );
	result += Texture2DFloat4.Sample( Sampler, uv, offset );
	result += Texture2DArrayFloat4.Sample( Sampler, uvw );
	result += Texture2DArrayFloat4.Sample( Sampler, uvw, offset );
	result += Texture3DFloat4.Sample( Sampler, uvw );
	result += TextureCubeFloat4.Sample( Sampler, dir );

	// SampleBias
	result += Texture2DFloat4.SampleBias( Sampler, uv, 0.5 );
	result += Texture2DArrayFloat4.SampleBias( Sampler, uvw, 0.5 );
	result += Texture3DFloat4.SampleBias( Sampler, uvw, 0.5 );
	result += TextureCubeFloat4.SampleBias( Sampler, dir, 0.5 );

	// SampleLevel
	result += Texture2DFloat4.SampleLevel( Sampler, uv, 1.0 );
	result += Texture2DArrayFloat4.SampleLevel( Sampler, uvw, 1.0 );
	result += Texture3DFloat4.SampleLevel( Sampler, uvw, 1.0 );
	result += TextureCubeFloat4.SampleLevel( Sampler, dir, 1.0 );

	// SampleGrad
	result += Texture2DFloat4.SampleGrad( Sampler, uv, ddx( uv ), ddy( uv ) );
	result += Texture2DArrayFloat4.SampleGrad( Sampler, uvw, ddx( uv ), ddy( uv ) );
	result += Texture3DFloat4.SampleGrad( Sampler, uvw, ddx( uvw ), ddy( uvw ) );
	result += TextureCubeFloat4.SampleGrad( Sampler, dir, ddx( dir ), ddy( dir ) );

	// Gather
	result += Texture2DFloat4.Gather( Sampler, uv );
	result += Texture2DFloat4.Gather( Sampler, uv, offset );
	result += Texture2DArrayFloat4.Gather( Sampler, uvw );
	result += Texture2DArrayFloat4.Gather( Sampler, uvw, offset );
	result += TextureCubeFloat4.Gather( Sampler, dir );
	result += TextureCubeArrayFloat4.Gather( Sampler, dirArray );

	// GatherRed
	result += Texture2DFloat4.GatherRed( Sampler, uv );
	result += Texture2DFloat4.GatherRed( Sampler, uv, offset );
	result += Texture2DArrayFloat4.GatherRed( Sampler, uvw );
	result += Texture2DArrayFloat4.GatherRed( Sampler, uvw, offset );
	result += TextureCubeFloat4.GatherRed( Sampler, dir );
	result += TextureCubeArrayFloat4.GatherRed( Sampler, dirArray );

	// GatherGreen
	result += Texture2DFloat4.GatherGreen( Sampler, uv );
	result += Texture2DFloat4.GatherGreen( Sampler, uv, offset );
	result += Texture2DArrayFloat4.GatherGreen( Sampler, uvw );
	result += Texture2DArrayFloat4.GatherGreen( Sampler, uvw, offset );
	result += TextureCubeFloat4.GatherGreen( Sampler, dir );
	result += TextureCubeArrayFloat4.GatherGreen( Sampler, dirArray );

	// GatherBlue
	result += Texture2DFloat4.GatherBlue( Sampler, uv );
	result += Texture2DFloat4.GatherBlue( Sampler, uv, offset );
	result += Texture2DArrayFloat4.GatherBlue( Sampler, uvw );
	result += Texture2DArrayFloat4.GatherBlue( Sampler, uvw, offset );
	result += TextureCubeFloat4.GatherBlue( Sampler, dir );
	result += TextureCubeArrayFloat4.GatherBlue( Sampler, dirArray );

	// GatherAlpha
	result += Texture2DFloat4.GatherAlpha( Sampler, uv );
	result += Texture2DFloat4.GatherAlpha( Sampler, uv, offset );
	result += Texture2DArrayFloat4.GatherAlpha( Sampler, uvw );
	result += Texture2DArrayFloat4.GatherAlpha( Sampler, uvw, offset );
	result += TextureCubeFloat4.GatherAlpha( Sampler, dir );
	result += TextureCubeArrayFloat4.GatherAlpha( Sampler, dirArray );

	// Load
	result += Texture2DFloat4.Load( int3( int2( pos.xy ), 0 ) );
	result += Texture2DArrayFloat4.Load( int4( int2( pos.xy ), 1, 0 ) );
	result += Texture3DFloat4.Load( int4( int3( pos.xyz ), 0 ) );

	// Indexed reads
	result += Texture2DFloat4[uint2( pos.xy )];
	result += Texture2DArrayFloat4[uint3( uint2( pos.xy ), 1u )];
	result += Texture3DFloat4[uint3( pos.xyz )];

	// GetDimensions
	uint width, height, depth, levels;
	uint dimensions = 0u;
	Texture2DFloat4.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeFloat4.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeFloat4.GetDimensions( 1u, width, height, levels );
	dimensions += width + height + levels;
	Texture3DFloat4.GetDimensions( width, height, depth );
	dimensions += width + height + depth;
	float d = float( dimensions );
	result += float4( d, d, d, d );

	return result;
}

float4 vs(): SV_Position
{
	return float4( 0.0, 0.0, 0.0, 1.0 );
}

float4 ps( float4 pos: SV_Position ): SV_Target
{
	float4 result = float4( 0.0, 0.0, 0.0, 0.0 );
	result += TestFloat( pos );
	result += TestFloat2( pos );
	result += TestFloat3( pos );
	result += TestFloat4( pos );
	return result;
}

technique t0
{
	pass p0
	{
		vertexshader = compile vs_5_0 vs();
		pixelshader = compile ps_5_0 ps();
	}
}
)SRC";

	EXPECT_TRUE( Compiles<typename TestFixture::Compiler>( src ) );
}

// See FloatFunctionsCompile. Runs the same functions on int textures, except for the sample
// functions.
TYPED_TEST( TextureFunctions, IntFunctionsCompile )
{
	if( !g_metalCompilerAvailable && std::is_same_v<typename TestFixture::Compiler, EffectCompilerMetal> )
	{
		GTEST_SKIP() << "Skipping test: Metal compiler is not available.";
	}

	// Split into several literals, as MSVC limits the length of a single string literal.
	const char* src = R"SRC(
SamplerState Sampler;
)SRC"
					  R"SRC(
Texture2D<int> Texture2DInt;
Texture2DArray<int> Texture2DArrayInt;
Texture3D<int> Texture3DInt;
TextureCube<int> TextureCubeInt;
TextureCubeArray<int> TextureCubeArrayInt;

int4 TestInt( float4 pos )
{
	float2 uv = pos.xy;
	float3 uvw = float3( uv, 0.5 );
	float3 dir = normalize( uvw );
	float4 dirArray = float4( dir, 1.0 );
	int2 offset = int2( 1, -1 );
	int4 result = int4( 0, 0, 0, 0 );

	// Gather
	result += Texture2DInt.Gather( Sampler, uv );
	result += Texture2DInt.Gather( Sampler, uv, offset );
	result += Texture2DArrayInt.Gather( Sampler, uvw );
	result += Texture2DArrayInt.Gather( Sampler, uvw, offset );
	result += TextureCubeInt.Gather( Sampler, dir );
	result += TextureCubeArrayInt.Gather( Sampler, dirArray );

	// GatherRed
	result += Texture2DInt.GatherRed( Sampler, uv );
	result += Texture2DInt.GatherRed( Sampler, uv, offset );
	result += Texture2DArrayInt.GatherRed( Sampler, uvw );
	result += Texture2DArrayInt.GatherRed( Sampler, uvw, offset );
	result += TextureCubeInt.GatherRed( Sampler, dir );
	result += TextureCubeArrayInt.GatherRed( Sampler, dirArray );

	// Load
	result.x += Texture2DInt.Load( int3( int2( pos.xy ), 0 ) );
	result.x += Texture2DArrayInt.Load( int4( int2( pos.xy ), 1, 0 ) );
	result.x += Texture3DInt.Load( int4( int3( pos.xyz ), 0 ) );

	// Indexed reads
	result.x += Texture2DInt[uint2( pos.xy )];
	result.x += Texture2DArrayInt[uint3( uint2( pos.xy ), 1u )];
	result.x += Texture3DInt[uint3( pos.xyz )];

	// GetDimensions
	uint width, height, depth, levels;
	uint dimensions = 0u;
	Texture2DInt.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeInt.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeInt.GetDimensions( 1u, width, height, levels );
	dimensions += width + height + levels;
	Texture3DInt.GetDimensions( width, height, depth );
	dimensions += width + height + depth;
	int d = int( dimensions );
	result += int4( d, d, d, d );

	return result;
}
)SRC"
					  R"SRC(
Texture2D<int2> Texture2DInt2;
Texture2DArray<int2> Texture2DArrayInt2;
Texture3D<int2> Texture3DInt2;
TextureCube<int2> TextureCubeInt2;
TextureCubeArray<int2> TextureCubeArrayInt2;

int4 TestInt2( float4 pos )
{
	float2 uv = pos.xy;
	float3 uvw = float3( uv, 0.5 );
	float3 dir = normalize( uvw );
	float4 dirArray = float4( dir, 1.0 );
	int2 offset = int2( 1, -1 );
	int4 result = int4( 0, 0, 0, 0 );

	// Gather
	result += Texture2DInt2.Gather( Sampler, uv );
	result += Texture2DInt2.Gather( Sampler, uv, offset );
	result += Texture2DArrayInt2.Gather( Sampler, uvw );
	result += Texture2DArrayInt2.Gather( Sampler, uvw, offset );
	result += TextureCubeInt2.Gather( Sampler, dir );
	result += TextureCubeArrayInt2.Gather( Sampler, dirArray );

	// GatherRed
	result += Texture2DInt2.GatherRed( Sampler, uv );
	result += Texture2DInt2.GatherRed( Sampler, uv, offset );
	result += Texture2DArrayInt2.GatherRed( Sampler, uvw );
	result += Texture2DArrayInt2.GatherRed( Sampler, uvw, offset );
	result += TextureCubeInt2.GatherRed( Sampler, dir );
	result += TextureCubeArrayInt2.GatherRed( Sampler, dirArray );

	// GatherGreen
	result += Texture2DInt2.GatherGreen( Sampler, uv );
	result += Texture2DInt2.GatherGreen( Sampler, uv, offset );
	result += Texture2DArrayInt2.GatherGreen( Sampler, uvw );
	result += Texture2DArrayInt2.GatherGreen( Sampler, uvw, offset );
	result += TextureCubeInt2.GatherGreen( Sampler, dir );
	result += TextureCubeArrayInt2.GatherGreen( Sampler, dirArray );

	// Load
	result.xy += Texture2DInt2.Load( int3( int2( pos.xy ), 0 ) );
	result.xy += Texture2DArrayInt2.Load( int4( int2( pos.xy ), 1, 0 ) );
	result.xy += Texture3DInt2.Load( int4( int3( pos.xyz ), 0 ) );

	// Indexed reads
	result.xy += Texture2DInt2[uint2( pos.xy )];
	result.xy += Texture2DArrayInt2[uint3( uint2( pos.xy ), 1u )];
	result.xy += Texture3DInt2[uint3( pos.xyz )];

	// GetDimensions
	uint width, height, depth, levels;
	uint dimensions = 0u;
	Texture2DInt2.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeInt2.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeInt2.GetDimensions( 1u, width, height, levels );
	dimensions += width + height + levels;
	Texture3DInt2.GetDimensions( width, height, depth );
	dimensions += width + height + depth;
	int d = int( dimensions );
	result += int4( d, d, d, d );

	return result;
}
)SRC"
					  R"SRC(
Texture2D<int3> Texture2DInt3;
Texture2DArray<int3> Texture2DArrayInt3;
Texture3D<int3> Texture3DInt3;
TextureCube<int3> TextureCubeInt3;
TextureCubeArray<int3> TextureCubeArrayInt3;

int4 TestInt3( float4 pos )
{
	float2 uv = pos.xy;
	float3 uvw = float3( uv, 0.5 );
	float3 dir = normalize( uvw );
	float4 dirArray = float4( dir, 1.0 );
	int2 offset = int2( 1, -1 );
	int4 result = int4( 0, 0, 0, 0 );

	// Gather
	result += Texture2DInt3.Gather( Sampler, uv );
	result += Texture2DInt3.Gather( Sampler, uv, offset );
	result += Texture2DArrayInt3.Gather( Sampler, uvw );
	result += Texture2DArrayInt3.Gather( Sampler, uvw, offset );
	result += TextureCubeInt3.Gather( Sampler, dir );
	result += TextureCubeArrayInt3.Gather( Sampler, dirArray );

	// GatherRed
	result += Texture2DInt3.GatherRed( Sampler, uv );
	result += Texture2DInt3.GatherRed( Sampler, uv, offset );
	result += Texture2DArrayInt3.GatherRed( Sampler, uvw );
	result += Texture2DArrayInt3.GatherRed( Sampler, uvw, offset );
	result += TextureCubeInt3.GatherRed( Sampler, dir );
	result += TextureCubeArrayInt3.GatherRed( Sampler, dirArray );

	// GatherGreen
	result += Texture2DInt3.GatherGreen( Sampler, uv );
	result += Texture2DInt3.GatherGreen( Sampler, uv, offset );
	result += Texture2DArrayInt3.GatherGreen( Sampler, uvw );
	result += Texture2DArrayInt3.GatherGreen( Sampler, uvw, offset );
	result += TextureCubeInt3.GatherGreen( Sampler, dir );
	result += TextureCubeArrayInt3.GatherGreen( Sampler, dirArray );

	// GatherBlue
	result += Texture2DInt3.GatherBlue( Sampler, uv );
	result += Texture2DInt3.GatherBlue( Sampler, uv, offset );
	result += Texture2DArrayInt3.GatherBlue( Sampler, uvw );
	result += Texture2DArrayInt3.GatherBlue( Sampler, uvw, offset );
	result += TextureCubeInt3.GatherBlue( Sampler, dir );
	result += TextureCubeArrayInt3.GatherBlue( Sampler, dirArray );

	// Load
	result.xyz += Texture2DInt3.Load( int3( int2( pos.xy ), 0 ) );
	result.xyz += Texture2DArrayInt3.Load( int4( int2( pos.xy ), 1, 0 ) );
	result.xyz += Texture3DInt3.Load( int4( int3( pos.xyz ), 0 ) );

	// Indexed reads
	result.xyz += Texture2DInt3[uint2( pos.xy )];
	result.xyz += Texture2DArrayInt3[uint3( uint2( pos.xy ), 1u )];
	result.xyz += Texture3DInt3[uint3( pos.xyz )];

	// GetDimensions
	uint width, height, depth, levels;
	uint dimensions = 0u;
	Texture2DInt3.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeInt3.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeInt3.GetDimensions( 1u, width, height, levels );
	dimensions += width + height + levels;
	Texture3DInt3.GetDimensions( width, height, depth );
	dimensions += width + height + depth;
	int d = int( dimensions );
	result += int4( d, d, d, d );

	return result;
}
)SRC"
					  R"SRC(
Texture2D<int4> Texture2DInt4;
Texture2DArray<int4> Texture2DArrayInt4;
Texture3D<int4> Texture3DInt4;
TextureCube<int4> TextureCubeInt4;
TextureCubeArray<int4> TextureCubeArrayInt4;

int4 TestInt4( float4 pos )
{
	float2 uv = pos.xy;
	float3 uvw = float3( uv, 0.5 );
	float3 dir = normalize( uvw );
	float4 dirArray = float4( dir, 1.0 );
	int2 offset = int2( 1, -1 );
	int4 result = int4( 0, 0, 0, 0 );

	// Gather
	result += Texture2DInt4.Gather( Sampler, uv );
	result += Texture2DInt4.Gather( Sampler, uv, offset );
	result += Texture2DArrayInt4.Gather( Sampler, uvw );
	result += Texture2DArrayInt4.Gather( Sampler, uvw, offset );
	result += TextureCubeInt4.Gather( Sampler, dir );
	result += TextureCubeArrayInt4.Gather( Sampler, dirArray );

	// GatherRed
	result += Texture2DInt4.GatherRed( Sampler, uv );
	result += Texture2DInt4.GatherRed( Sampler, uv, offset );
	result += Texture2DArrayInt4.GatherRed( Sampler, uvw );
	result += Texture2DArrayInt4.GatherRed( Sampler, uvw, offset );
	result += TextureCubeInt4.GatherRed( Sampler, dir );
	result += TextureCubeArrayInt4.GatherRed( Sampler, dirArray );

	// GatherGreen
	result += Texture2DInt4.GatherGreen( Sampler, uv );
	result += Texture2DInt4.GatherGreen( Sampler, uv, offset );
	result += Texture2DArrayInt4.GatherGreen( Sampler, uvw );
	result += Texture2DArrayInt4.GatherGreen( Sampler, uvw, offset );
	result += TextureCubeInt4.GatherGreen( Sampler, dir );
	result += TextureCubeArrayInt4.GatherGreen( Sampler, dirArray );

	// GatherBlue
	result += Texture2DInt4.GatherBlue( Sampler, uv );
	result += Texture2DInt4.GatherBlue( Sampler, uv, offset );
	result += Texture2DArrayInt4.GatherBlue( Sampler, uvw );
	result += Texture2DArrayInt4.GatherBlue( Sampler, uvw, offset );
	result += TextureCubeInt4.GatherBlue( Sampler, dir );
	result += TextureCubeArrayInt4.GatherBlue( Sampler, dirArray );

	// GatherAlpha
	result += Texture2DInt4.GatherAlpha( Sampler, uv );
	result += Texture2DInt4.GatherAlpha( Sampler, uv, offset );
	result += Texture2DArrayInt4.GatherAlpha( Sampler, uvw );
	result += Texture2DArrayInt4.GatherAlpha( Sampler, uvw, offset );
	result += TextureCubeInt4.GatherAlpha( Sampler, dir );
	result += TextureCubeArrayInt4.GatherAlpha( Sampler, dirArray );

	// Load
	result += Texture2DInt4.Load( int3( int2( pos.xy ), 0 ) );
	result += Texture2DArrayInt4.Load( int4( int2( pos.xy ), 1, 0 ) );
	result += Texture3DInt4.Load( int4( int3( pos.xyz ), 0 ) );

	// Indexed reads
	result += Texture2DInt4[uint2( pos.xy )];
	result += Texture2DArrayInt4[uint3( uint2( pos.xy ), 1u )];
	result += Texture3DInt4[uint3( pos.xyz )];

	// GetDimensions
	uint width, height, depth, levels;
	uint dimensions = 0u;
	Texture2DInt4.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeInt4.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeInt4.GetDimensions( 1u, width, height, levels );
	dimensions += width + height + levels;
	Texture3DInt4.GetDimensions( width, height, depth );
	dimensions += width + height + depth;
	int d = int( dimensions );
	result += int4( d, d, d, d );

	return result;
}

float4 vs(): SV_Position
{
	return float4( 0.0, 0.0, 0.0, 1.0 );
}

float4 ps( float4 pos: SV_Position ): SV_Target
{
	float4 result = float4( 0.0, 0.0, 0.0, 0.0 );
	result += float4( TestInt( pos ) );
	result += float4( TestInt2( pos ) );
	result += float4( TestInt3( pos ) );
	result += float4( TestInt4( pos ) );
	return result;
}

technique t0
{
	pass p0
	{
		vertexshader = compile vs_5_0 vs();
		pixelshader = compile ps_5_0 ps();
	}
}
)SRC";

	EXPECT_TRUE( Compiles<typename TestFixture::Compiler>( src ) );
}

// See FloatFunctionsCompile. Runs the same functions on uint textures, except for the sample
// functions.
TYPED_TEST( TextureFunctions, UintFunctionsCompile )
{
	if( !g_metalCompilerAvailable && std::is_same_v<typename TestFixture::Compiler, EffectCompilerMetal> )
	{
		GTEST_SKIP() << "Skipping test: Metal compiler is not available.";
	}

	// Split into several literals, as MSVC limits the length of a single string literal.
	const char* src = R"SRC(
SamplerState Sampler;
)SRC"
					  R"SRC(
Texture2D<uint> Texture2DUint;
Texture2DArray<uint> Texture2DArrayUint;
Texture3D<uint> Texture3DUint;
TextureCube<uint> TextureCubeUint;
TextureCubeArray<uint> TextureCubeArrayUint;

uint4 TestUint( float4 pos )
{
	float2 uv = pos.xy;
	float3 uvw = float3( uv, 0.5 );
	float3 dir = normalize( uvw );
	float4 dirArray = float4( dir, 1.0 );
	int2 offset = int2( 1, -1 );
	uint4 result = uint4( 0u, 0u, 0u, 0u );

	// Gather
	result += Texture2DUint.Gather( Sampler, uv );
	result += Texture2DUint.Gather( Sampler, uv, offset );
	result += Texture2DArrayUint.Gather( Sampler, uvw );
	result += Texture2DArrayUint.Gather( Sampler, uvw, offset );
	result += TextureCubeUint.Gather( Sampler, dir );
	result += TextureCubeArrayUint.Gather( Sampler, dirArray );

	// GatherRed
	result += Texture2DUint.GatherRed( Sampler, uv );
	result += Texture2DUint.GatherRed( Sampler, uv, offset );
	result += Texture2DArrayUint.GatherRed( Sampler, uvw );
	result += Texture2DArrayUint.GatherRed( Sampler, uvw, offset );
	result += TextureCubeUint.GatherRed( Sampler, dir );
	result += TextureCubeArrayUint.GatherRed( Sampler, dirArray );

	// Load
	result.x += Texture2DUint.Load( int3( int2( pos.xy ), 0 ) );
	result.x += Texture2DArrayUint.Load( int4( int2( pos.xy ), 1, 0 ) );
	result.x += Texture3DUint.Load( int4( int3( pos.xyz ), 0 ) );

	// Indexed reads
	result.x += Texture2DUint[uint2( pos.xy )];
	result.x += Texture2DArrayUint[uint3( uint2( pos.xy ), 1u )];
	result.x += Texture3DUint[uint3( pos.xyz )];

	// GetDimensions
	uint width, height, depth, levels;
	uint dimensions = 0u;
	Texture2DUint.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeUint.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeUint.GetDimensions( 1u, width, height, levels );
	dimensions += width + height + levels;
	Texture3DUint.GetDimensions( width, height, depth );
	dimensions += width + height + depth;
	result += uint4( dimensions, dimensions, dimensions, dimensions );

	return result;
}
)SRC"
					  R"SRC(
Texture2D<uint2> Texture2DUint2;
Texture2DArray<uint2> Texture2DArrayUint2;
Texture3D<uint2> Texture3DUint2;
TextureCube<uint2> TextureCubeUint2;
TextureCubeArray<uint2> TextureCubeArrayUint2;

uint4 TestUint2( float4 pos )
{
	float2 uv = pos.xy;
	float3 uvw = float3( uv, 0.5 );
	float3 dir = normalize( uvw );
	float4 dirArray = float4( dir, 1.0 );
	int2 offset = int2( 1, -1 );
	uint4 result = uint4( 0u, 0u, 0u, 0u );

	// Gather
	result += Texture2DUint2.Gather( Sampler, uv );
	result += Texture2DUint2.Gather( Sampler, uv, offset );
	result += Texture2DArrayUint2.Gather( Sampler, uvw );
	result += Texture2DArrayUint2.Gather( Sampler, uvw, offset );
	result += TextureCubeUint2.Gather( Sampler, dir );
	result += TextureCubeArrayUint2.Gather( Sampler, dirArray );

	// GatherRed
	result += Texture2DUint2.GatherRed( Sampler, uv );
	result += Texture2DUint2.GatherRed( Sampler, uv, offset );
	result += Texture2DArrayUint2.GatherRed( Sampler, uvw );
	result += Texture2DArrayUint2.GatherRed( Sampler, uvw, offset );
	result += TextureCubeUint2.GatherRed( Sampler, dir );
	result += TextureCubeArrayUint2.GatherRed( Sampler, dirArray );

	// GatherGreen
	result += Texture2DUint2.GatherGreen( Sampler, uv );
	result += Texture2DUint2.GatherGreen( Sampler, uv, offset );
	result += Texture2DArrayUint2.GatherGreen( Sampler, uvw );
	result += Texture2DArrayUint2.GatherGreen( Sampler, uvw, offset );
	result += TextureCubeUint2.GatherGreen( Sampler, dir );
	result += TextureCubeArrayUint2.GatherGreen( Sampler, dirArray );

	// Load
	result.xy += Texture2DUint2.Load( int3( int2( pos.xy ), 0 ) );
	result.xy += Texture2DArrayUint2.Load( int4( int2( pos.xy ), 1, 0 ) );
	result.xy += Texture3DUint2.Load( int4( int3( pos.xyz ), 0 ) );

	// Indexed reads
	result.xy += Texture2DUint2[uint2( pos.xy )];
	result.xy += Texture2DArrayUint2[uint3( uint2( pos.xy ), 1u )];
	result.xy += Texture3DUint2[uint3( pos.xyz )];

	// GetDimensions
	uint width, height, depth, levels;
	uint dimensions = 0u;
	Texture2DUint2.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeUint2.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeUint2.GetDimensions( 1u, width, height, levels );
	dimensions += width + height + levels;
	Texture3DUint2.GetDimensions( width, height, depth );
	dimensions += width + height + depth;
	result += uint4( dimensions, dimensions, dimensions, dimensions );

	return result;
}
)SRC"
					  R"SRC(
Texture2D<uint3> Texture2DUint3;
Texture2DArray<uint3> Texture2DArrayUint3;
Texture3D<uint3> Texture3DUint3;
TextureCube<uint3> TextureCubeUint3;
TextureCubeArray<uint3> TextureCubeArrayUint3;

uint4 TestUint3( float4 pos )
{
	float2 uv = pos.xy;
	float3 uvw = float3( uv, 0.5 );
	float3 dir = normalize( uvw );
	float4 dirArray = float4( dir, 1.0 );
	int2 offset = int2( 1, -1 );
	uint4 result = uint4( 0u, 0u, 0u, 0u );

	// Gather
	result += Texture2DUint3.Gather( Sampler, uv );
	result += Texture2DUint3.Gather( Sampler, uv, offset );
	result += Texture2DArrayUint3.Gather( Sampler, uvw );
	result += Texture2DArrayUint3.Gather( Sampler, uvw, offset );
	result += TextureCubeUint3.Gather( Sampler, dir );
	result += TextureCubeArrayUint3.Gather( Sampler, dirArray );

	// GatherRed
	result += Texture2DUint3.GatherRed( Sampler, uv );
	result += Texture2DUint3.GatherRed( Sampler, uv, offset );
	result += Texture2DArrayUint3.GatherRed( Sampler, uvw );
	result += Texture2DArrayUint3.GatherRed( Sampler, uvw, offset );
	result += TextureCubeUint3.GatherRed( Sampler, dir );
	result += TextureCubeArrayUint3.GatherRed( Sampler, dirArray );

	// GatherGreen
	result += Texture2DUint3.GatherGreen( Sampler, uv );
	result += Texture2DUint3.GatherGreen( Sampler, uv, offset );
	result += Texture2DArrayUint3.GatherGreen( Sampler, uvw );
	result += Texture2DArrayUint3.GatherGreen( Sampler, uvw, offset );
	result += TextureCubeUint3.GatherGreen( Sampler, dir );
	result += TextureCubeArrayUint3.GatherGreen( Sampler, dirArray );

	// GatherBlue
	result += Texture2DUint3.GatherBlue( Sampler, uv );
	result += Texture2DUint3.GatherBlue( Sampler, uv, offset );
	result += Texture2DArrayUint3.GatherBlue( Sampler, uvw );
	result += Texture2DArrayUint3.GatherBlue( Sampler, uvw, offset );
	result += TextureCubeUint3.GatherBlue( Sampler, dir );
	result += TextureCubeArrayUint3.GatherBlue( Sampler, dirArray );

	// Load
	result.xyz += Texture2DUint3.Load( int3( int2( pos.xy ), 0 ) );
	result.xyz += Texture2DArrayUint3.Load( int4( int2( pos.xy ), 1, 0 ) );
	result.xyz += Texture3DUint3.Load( int4( int3( pos.xyz ), 0 ) );

	// Indexed reads
	result.xyz += Texture2DUint3[uint2( pos.xy )];
	result.xyz += Texture2DArrayUint3[uint3( uint2( pos.xy ), 1u )];
	result.xyz += Texture3DUint3[uint3( pos.xyz )];

	// GetDimensions
	uint width, height, depth, levels;
	uint dimensions = 0u;
	Texture2DUint3.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeUint3.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeUint3.GetDimensions( 1u, width, height, levels );
	dimensions += width + height + levels;
	Texture3DUint3.GetDimensions( width, height, depth );
	dimensions += width + height + depth;
	result += uint4( dimensions, dimensions, dimensions, dimensions );

	return result;
}
)SRC"
					  R"SRC(
Texture2D<uint4> Texture2DUint4;
Texture2DArray<uint4> Texture2DArrayUint4;
Texture3D<uint4> Texture3DUint4;
TextureCube<uint4> TextureCubeUint4;
TextureCubeArray<uint4> TextureCubeArrayUint4;

uint4 TestUint4( float4 pos )
{
	float2 uv = pos.xy;
	float3 uvw = float3( uv, 0.5 );
	float3 dir = normalize( uvw );
	float4 dirArray = float4( dir, 1.0 );
	int2 offset = int2( 1, -1 );
	uint4 result = uint4( 0u, 0u, 0u, 0u );

	// Gather
	result += Texture2DUint4.Gather( Sampler, uv );
	result += Texture2DUint4.Gather( Sampler, uv, offset );
	result += Texture2DArrayUint4.Gather( Sampler, uvw );
	result += Texture2DArrayUint4.Gather( Sampler, uvw, offset );
	result += TextureCubeUint4.Gather( Sampler, dir );
	result += TextureCubeArrayUint4.Gather( Sampler, dirArray );

	// GatherRed
	result += Texture2DUint4.GatherRed( Sampler, uv );
	result += Texture2DUint4.GatherRed( Sampler, uv, offset );
	result += Texture2DArrayUint4.GatherRed( Sampler, uvw );
	result += Texture2DArrayUint4.GatherRed( Sampler, uvw, offset );
	result += TextureCubeUint4.GatherRed( Sampler, dir );
	result += TextureCubeArrayUint4.GatherRed( Sampler, dirArray );

	// GatherGreen
	result += Texture2DUint4.GatherGreen( Sampler, uv );
	result += Texture2DUint4.GatherGreen( Sampler, uv, offset );
	result += Texture2DArrayUint4.GatherGreen( Sampler, uvw );
	result += Texture2DArrayUint4.GatherGreen( Sampler, uvw, offset );
	result += TextureCubeUint4.GatherGreen( Sampler, dir );
	result += TextureCubeArrayUint4.GatherGreen( Sampler, dirArray );

	// GatherBlue
	result += Texture2DUint4.GatherBlue( Sampler, uv );
	result += Texture2DUint4.GatherBlue( Sampler, uv, offset );
	result += Texture2DArrayUint4.GatherBlue( Sampler, uvw );
	result += Texture2DArrayUint4.GatherBlue( Sampler, uvw, offset );
	result += TextureCubeUint4.GatherBlue( Sampler, dir );
	result += TextureCubeArrayUint4.GatherBlue( Sampler, dirArray );

	// GatherAlpha
	result += Texture2DUint4.GatherAlpha( Sampler, uv );
	result += Texture2DUint4.GatherAlpha( Sampler, uv, offset );
	result += Texture2DArrayUint4.GatherAlpha( Sampler, uvw );
	result += Texture2DArrayUint4.GatherAlpha( Sampler, uvw, offset );
	result += TextureCubeUint4.GatherAlpha( Sampler, dir );
	result += TextureCubeArrayUint4.GatherAlpha( Sampler, dirArray );

	// Load
	result += Texture2DUint4.Load( int3( int2( pos.xy ), 0 ) );
	result += Texture2DArrayUint4.Load( int4( int2( pos.xy ), 1, 0 ) );
	result += Texture3DUint4.Load( int4( int3( pos.xyz ), 0 ) );

	// Indexed reads
	result += Texture2DUint4[uint2( pos.xy )];
	result += Texture2DArrayUint4[uint3( uint2( pos.xy ), 1u )];
	result += Texture3DUint4[uint3( pos.xyz )];

	// GetDimensions
	uint width, height, depth, levels;
	uint dimensions = 0u;
	Texture2DUint4.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeUint4.GetDimensions( width, height );
	dimensions += width + height;
	TextureCubeUint4.GetDimensions( 1u, width, height, levels );
	dimensions += width + height + levels;
	Texture3DUint4.GetDimensions( width, height, depth );
	dimensions += width + height + depth;
	result += uint4( dimensions, dimensions, dimensions, dimensions );

	return result;
}
)SRC"
					  R"SRC(
float4 vs(): SV_Position
{
	return float4( 0.0, 0.0, 0.0, 1.0 );
}

float4 ps( float4 pos: SV_Position ): SV_Target
{
	float4 result = float4( 0.0, 0.0, 0.0, 0.0 );
	result += float4( TestUint( pos ) );
	result += float4( TestUint2( pos ) );
	result += float4( TestUint3( pos ) );
	result += float4( TestUint4( pos ) );
	return result;
}

technique t0
{
	pass p0
	{
		vertexshader = compile vs_5_0 vs();
		pixelshader = compile ps_5_0 ps();
	}
}
)SRC";

	EXPECT_TRUE( Compiles<typename TestFixture::Compiler>( src ) );
}

// Calls the legacy tex*() functions on sampler_state samplers, the way older shaders use them,
// for 2D, 3D and cube textures. Each texture is untyped; its type is inferred from the tex*()
// functions its sampler is used with. All compilers convert these calls to the modern texture
// functions before compiling.
TYPED_TEST( TextureFunctions, LegacyFunctionsCompile )
{
	if( !g_metalCompilerAvailable && std::is_same_v<typename TestFixture::Compiler, EffectCompilerMetal> )
	{
		GTEST_SKIP() << "Skipping test: Metal compiler is not available.";
	}

	const char* src = R"SRC(
texture LegacyTexture2D;
texture LegacyTexture3D;
texture LegacyTextureCube;

sampler LegacySampler2D = sampler_state
{
	Texture = <LegacyTexture2D>;
};

sampler LegacySampler3D = sampler_state
{
	Texture = <LegacyTexture3D>;
};

sampler LegacySamplerCube = sampler_state
{
	Texture = <LegacyTextureCube>;
};

float4 TestLegacy( float4 pos )
{
	float2 uv = pos.xy;
	float3 uvw = float3( uv, 0.5 );
	float3 dir = normalize( uvw );
	float4 result = float4( 0.0, 0.0, 0.0, 0.0 );

	// tex2D*
	result += tex2D( LegacySampler2D, uv );
	result += tex2D( LegacySampler2D, uv, ddx( uv ), ddy( uv ) );
	result += tex2Dbias( LegacySampler2D, float4( uv, 0.0, 0.5 ) );
	result += tex2Dgrad( LegacySampler2D, uv, ddx( uv ), ddy( uv ) );
	result += tex2Dlod( LegacySampler2D, float4( uv, 0.0, 1.0 ) );
	result += tex2Dproj( LegacySampler2D, float4( uv, 0.0, 2.0 ) );

	// tex3D*
	result += tex3D( LegacySampler3D, uvw );
	result += tex3D( LegacySampler3D, uvw, ddx( uvw ), ddy( uvw ) );
	result += tex3Dbias( LegacySampler3D, float4( uvw, 0.5 ) );
	result += tex3Dgrad( LegacySampler3D, uvw, ddx( uvw ), ddy( uvw ) );
	result += tex3Dlod( LegacySampler3D, float4( uvw, 1.0 ) );
	result += tex3Dproj( LegacySampler3D, float4( uvw, 2.0 ) );

	// texCUBE*
	result += texCUBE( LegacySamplerCube, dir );
	result += texCUBE( LegacySamplerCube, dir, ddx( dir ), ddy( dir ) );
	result += texCUBEbias( LegacySamplerCube, float4( dir, 0.5 ) );
	result += texCUBEgrad( LegacySamplerCube, dir, ddx( dir ), ddy( dir ) );
	result += texCUBElod( LegacySamplerCube, float4( dir, 1.0 ) );
	result += texCUBEproj( LegacySamplerCube, float4( dir, 2.0 ) );

	return result;
}

float4 vs(): SV_Position
{
	return float4( 0.0, 0.0, 0.0, 1.0 );
}

float4 ps( float4 pos: SV_Position ): SV_Target
{
	return TestLegacy( pos );
}

technique t0
{
	pass p0
	{
		vertexshader = compile vs_5_0 vs();
		pixelshader = compile ps_5_0 ps();
	}
}
)SRC";

	EXPECT_TRUE( Compiles<typename TestFixture::Compiler>( src ) );
}
