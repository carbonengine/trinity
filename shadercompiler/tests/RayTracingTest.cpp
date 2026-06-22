// Copyright © 2024 CCP ehf.

#include "gtest/gtest.h"
#include "EffectCompilerMetal.h"
#include "EffectCompilerDX12.h"
#include "EffectData.h"
#include "TesingUtils.h"

extern StringTable g_stringTable;

template <typename T>
class RayTracing : public testing::Test
{
public:
	using Compiler = T;
	static T shared_;
	T value_;
};

#if _WIN32
using RayTracingCompilers = ::testing::Types<EffectCompilerDX12, EffectCompilerMetal>;
#else
using RayTracingCompilers = ::testing::Types<EffectCompilerMetal>;
#endif

TYPED_TEST_SUITE( RayTracing, RayTracingCompilers );

TYPED_TEST( RayTracing, NumericInputsAreLocal )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

float LocalInput;

[shader("miss")]
void Miss(inout HitInfo payload)
{
	payload.visibility = LocalInput;
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

	auto data = Compile<typename TestFixture::Compiler>( src );
	EXPECT_FALSE( data.techniques[0].libraries[0].localInputs.constants.empty() );
	EXPECT_TRUE( data.techniques[0].libraries[0].globalInputs.constants.empty() );
}

TYPED_TEST( RayTracing, CanAccessLocalInputFromFunctions )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

float LocalInput;

float GetLocalInput()
{
	return LocalInput;
}

[shader("miss")]
void Miss(inout HitInfo payload)
{
	payload.visibility = GetLocalInput();
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

	ASSERT_TRUE( Compiles<typename TestFixture::Compiler>( src ) );
}

TYPED_TEST( RayTracing, SrvsAreGlobal )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

Texture2D GlobalInput;

[shader("miss")]
[globalinput("GlobalInput;")]
void Miss(inout HitInfo payload)
{
	payload.visibility = GlobalInput[uint2( 0, 0 )].r;
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

	auto data = Compile<typename TestFixture::Compiler>( src );
	EXPECT_TRUE( data.techniques[0].libraries[0].localInputs.textures.empty() );
	EXPECT_FALSE( data.techniques[0].libraries[0].globalInputs.textures.empty() );
}

TYPED_TEST( RayTracing, MissingSrvsInGlobalInputGeneratesError )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

Texture2D GlobalInput;

[shader("miss")]
void Miss(inout HitInfo payload)
{
	payload.visibility = GlobalInput[uint2( 0, 0 )].r;
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

	ASSERT_FALSE( Compiles<typename TestFixture::Compiler>( src ) );
}

TYPED_TEST( RayTracing, AssignsRegistersBasedOnGlobalInput )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

Texture2D GlobalInput;
Texture2D Other;

[shader("miss")]
[globalinput("Other; GlobalInput;")]
void Miss(inout HitInfo payload)
{
	payload.visibility = GlobalInput[uint2( 0, 0 )].r;
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

	auto data = Compile<typename TestFixture::Compiler>( src );
	// GlobalInpt texture should be assigned to register (t1, space0)
	EXPECT_TRUE( data.techniques[0].libraries[0].localInputs.textures.empty() );
	auto& textures = data.techniques[0].libraries[0].globalInputs.textures;
	auto found = std::find_if( begin( textures ), end( textures ), []( auto& tex ) { return strcmp( g_stringTable.GetString( tex.second.name ), "GlobalInput" ) == 0; } );
	ASSERT_NE( found, end( textures ) );
}

TYPED_TEST( RayTracing, AllowsTextureArraysInGlobalInput )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

Texture2D<float4> HeapView_Texture2D[]
<
	bool IsHeapView = true;
>;
Texture2D Other;

[shader("miss")]
[globalinput("HeapView_Texture2D; Other;")]
void Miss(inout HitInfo payload)
{
	payload.visibility = HeapView_Texture2D[0][uint2( 0, 0 )].r + Other[uint2( 0, 0 )].r;
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

	auto data = Compile<typename TestFixture::Compiler>( src );
	// GlobalInpt texture should be assigned to register (t1, space8)
	EXPECT_TRUE( data.techniques[0].libraries[0].localInputs.textures.empty() );
	ASSERT_EQ( data.techniques[0].libraries[0].globalInputs.registerInputs.size(), 2u );
	EXPECT_NE( data.techniques[0].libraries[0].globalInputs.registerInputs[0].registerIndex, data.techniques[0].libraries[0].globalInputs.registerInputs[1].registerIndex );
}

TYPED_TEST( RayTracing, AllowsBuffersInGlobalInput )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

Buffer<float> SrvBuffer;

Texture2D Other;

[shader("miss")]
[globalinput("SrvBuffer; Other;")]
void Miss(inout HitInfo payload)
{
	payload.visibility = SrvBuffer[123] + Other[uint2( 0, 0 )].r;
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

	auto data = Compile<typename TestFixture::Compiler>( src );
	// GlobalInpt texture should be assigned to register (t1, space8)
	EXPECT_TRUE( data.techniques[0].libraries[0].localInputs.textures.empty() );
	ASSERT_EQ( data.techniques[0].libraries[0].globalInputs.registerInputs.size(), 2u );
	EXPECT_NE( data.techniques[0].libraries[0].globalInputs.registerInputs[0].registerIndex, data.techniques[0].libraries[0].globalInputs.registerInputs[1].registerIndex );
}

TYPED_TEST( RayTracing, AllowsBufferArraysInGlobalInput )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

Buffer<float> HeapView_Buffer[]
<
	bool IsHeapView = true;
>;
Texture2D Other;

[shader("miss")]
[globalinput("HeapView_Buffer; Other;")]
void Miss(inout HitInfo payload)
{
	payload.visibility = HeapView_Buffer[0][2] + Other[uint2( 0, 0 )].r;
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

	auto data = Compile<typename TestFixture::Compiler>( src );
	// GlobalInpt texture should be assigned to register (t1, space8)
	EXPECT_TRUE( data.techniques[0].libraries[0].localInputs.textures.empty() );
	ASSERT_EQ( data.techniques[0].libraries[0].globalInputs.registerInputs.size(), 2u );
	EXPECT_NE( data.techniques[0].libraries[0].globalInputs.registerInputs[0].registerIndex, data.techniques[0].libraries[0].globalInputs.registerInputs[1].registerIndex );
}

TYPED_TEST( RayTracing, PerFrameDataIsGlobal )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};


struct PerFrameVSData
{
    float coefficient;
} PerFrameVS : register( vs, c220 );

[shader("miss")]
[globalinput("PerFrameVS;")]
void Miss(inout HitInfo payload)
{
	payload.visibility = PerFrameVS.coefficient;
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

	auto data = Compile<typename TestFixture::Compiler>( src );
	EXPECT_TRUE( data.techniques[0].libraries[0].localInputs.registerInputs.empty() );
	ASSERT_EQ( data.techniques[0].libraries[0].globalInputs.registerInputs.size(), 1u );
	EXPECT_EQ( data.techniques[0].libraries[0].globalInputs.registerInputs[0].registerType, RT_CONSTANT_BUFFER );
}


TYPED_TEST( RayTracing, IncludesScalarAnnotations )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

float DefaultVisibility <bool TestAnnotation = true;>;

[shader("miss")]
void Miss(inout HitInfo payload)
{
	payload.visibility = DefaultVisibility;
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

	auto data = Compile<typename TestFixture::Compiler>( src );
	ASSERT_FALSE( data.techniques[0].libraries[0].localInputs.constants.empty() );
	ASSERT_EQ( std::string( "DefaultVisibility" ), g_stringTable.GetString( data.techniques[0].libraries[0].localInputs.constants[0].name ) );
	ASSERT_FALSE( data.annotations.empty() );
	auto found = find_if( begin( data.annotations ), end( data.annotations ), []( auto& x ) { return strcmp( g_stringTable.GetString( x.first ), "DefaultVisibility" ) == 0; } );
	ASSERT_NE( found, end( data.annotations ) );
	auto value = find_if( begin( found->second.annotations ), end( found->second.annotations ), []( auto& x ) { return strcmp( g_stringTable.GetString( x.first ), "TestAnnotation" ) == 0; } );
	ASSERT_NE( value, end( found->second.annotations ) );
}

TYPED_TEST( RayTracing, IncludesSrvAnnotations )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

Texture2D TestMap <bool TestAnnotation = true;>;

[shader("miss")]
[globalinput("TestMap")]
void Miss(inout HitInfo payload)
{
	payload.visibility = TestMap[uint2( 0, 0 )].r;
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

	auto data = Compile<typename TestFixture::Compiler>( src );
	ASSERT_FALSE( data.techniques[0].libraries[0].globalInputs.textures.empty() );
	ASSERT_EQ( std::string( "TestMap" ), g_stringTable.GetString( data.techniques[0].libraries[0].globalInputs.textures[0].name ) );
	ASSERT_FALSE( data.annotations.empty() );
	auto found = find_if( begin( data.annotations ), end( data.annotations ), []( auto& x ) { return strcmp( g_stringTable.GetString( x.first ), "TestMap" ) == 0; } );
	ASSERT_NE( found, end( data.annotations ) );
	auto value = find_if( begin( found->second.annotations ), end( found->second.annotations ), []( auto& x ) { return strcmp( g_stringTable.GetString( x.first ), "TestAnnotation" ) == 0; } );
	ASSERT_NE( value, end( found->second.annotations ) );
}

TYPED_TEST( RayTracing, AllowSamplersInGlobalInput )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

Texture2D TestMap;
SamplerState TestMapSampler
{
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Point;
    AddressU  = Clamp;
    AddressV  = Clamp;
};

[shader("miss")]
[globalinput("TestMap; TestMapSampler")]
void Miss(inout HitInfo payload)
{
	payload.visibility = TestMap.SampleLevel( TestMapSampler, float2( 0.0, 0.0 ), 0 ).r;
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

	auto data = Compile<typename TestFixture::Compiler>( src );
	ASSERT_FALSE( data.techniques[0].libraries[0].globalInputs.textures.empty() );
	// on metal static samplers are not exposed
	if( !std::is_same<typename TestFixture::Compiler, EffectCompilerMetal>::value )
	{
		ASSERT_FALSE( data.techniques[0].libraries[0].globalInputs.staticSamplers.empty() );
	}
}

TYPED_TEST( RayTracing, AllowBindlessSamplersInLocalInput )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

Texture2D TestMap;
BindlessHandleSampler TestMapSampler
{
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Point;
    AddressU  = Clamp;
    AddressV  = Clamp;
    IsDynamic = true;
};

SamplerState HeapView_Sampler[]
<
 bool IsHeapView = true;
>;

[shader("miss")]
[globalinput("TestMap;HeapView_Sampler")]
void Miss(inout HitInfo payload)
{
	payload.visibility = TestMap.SampleLevel( HeapView_Sampler[TestMapSampler], float2( 0.0, 0.0 ), 0 ).r;
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

	auto data = Compile<typename TestFixture::Compiler>( src );
	ASSERT_FALSE( data.techniques[0].libraries[0].globalInputs.textures.empty() );
	ASSERT_FALSE( data.techniques[0].libraries[0].localInputs.samplers.empty() );
}

TYPED_TEST( RayTracing, AllowMergedSamplersInGlobalInput )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

Texture2D TestMap;
SamplerState TestMapSampler1
{
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Point;
    AddressU  = Clamp;
    AddressV  = Clamp;
};
SamplerState TestMapSampler2
{
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Point;
    AddressU  = Clamp;
    AddressV  = Clamp;
};

[shader("miss")]
[globalinput("TestMap; TestMapSampler1; TestMapSampler2")]
void Miss(inout HitInfo payload)
{
	payload.visibility = TestMap.SampleLevel( TestMapSampler1, float2( 0.0, 0.0 ), 0 ).r + TestMap.SampleLevel( TestMapSampler2, float2( 0.5, 0.0 ), 0 ).r;
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

	auto data = Compile<typename TestFixture::Compiler>( src );
	ASSERT_FALSE( data.techniques[0].libraries[0].globalInputs.textures.empty() );
	// on metal static samplers are not exposed
	if( !std::is_same<typename TestFixture::Compiler, EffectCompilerMetal>::value )
	{
		ASSERT_FALSE( data.techniques[0].libraries[0].globalInputs.staticSamplers.empty() );
	}
}


TYPED_TEST( RayTracing, RealLifeTest )
{
	const char* src = R"SRC(

RaytracingAccelerationStructure Scene <bool SasUiVisible = true; >;
RWTexture2D<float4> ShadowDest <bool SasUiVisible = true;>;
Texture2D<float> DepthMap <bool AutoRegister = true;>;

struct PerFrameVSData
{
	// matrices
    float4x4 ViewInverseTransposeMat;
    float4x4 ViewProjectionMat;  
    float4x4 ViewMat;
    float4x4 ProjectionMat;
    float4x4 ShadowViewMat;
    float4x4 ShadowViewProjectionMat;
    float4x4 EnvMapRotationMat;
    float4x4 ViewProjectionLast;
    float4x4 ViewLast;
    float4x4 ProjLast;
    // lighting info
    float4 SunDirection;
    float4 SunDiffuseColor;
	float3 FogFactors; // Depth multiplier in x, constant in y and max fog in z
	// rendertarget resolution and fovXY (rt pixelsize in .xy, fovXY in .zw)
	float4 TargetResolution;
	float4 ViewportAdjustment;
	float4 MiscParams; // x - time, y - upscaling amount, zw - viewport size
} PerFrameVS : register( vs, c220 );


struct HitInfo
{
    float visibility;
};

float4x4 ProjectionInv;

#define GLOBAL_INPUT "Scene; ShadowDest; DepthMap; PerFrameVS;"


[shader("raygeneration")]
[globalinput(GLOBAL_INPUT)]
void RayGen()
{
    // Which pixel spawned our ray?
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();

    float depth = DepthMap[launchIndex.xy];
    if( depth == 0 )
	{
		return;
	}
	float3 sunDirection = -normalize( PerFrameVS.SunDirection.xyz );
	float3 rayDirection = sunDirection;

	float2 dd = (float2( launchIndex.xy ) + 0.5) / PerFrameVS.TargetResolution.xy * 2.0f - 1.0f;
	dd.y = -dd.y;
	float4 projPos = float4( dd, depth, 1 );
	float4 viewPos = mul( projPos, ProjectionInv );
	viewPos /= viewPos.w;
	float3 worldPos = mul( PerFrameVS.ViewInverseTransposeMat, viewPos ).xyz;

	RayDesc ray;
	ray.Origin = worldPos;
	ray.Direction = rayDirection;

	ray.TMin = 0.0f;
	ray.TMax = 10000000.0; // camera far plane value
	
	HitInfo payload = {0.0};
	TraceRay(
		Scene,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
		0xFF,
		0,
		0,
		0,
		ray,
		payload );

	ShadowDest[launchIndex.xy] = payload.visibility;
}

[shader("miss")]
[globalinput(GLOBAL_INPUT)]
void Miss(inout HitInfo payload)
{
	payload.visibility = 1;
}

struct Attributes 
{
	float2 uv;
};

struct PerObjectPSData {
  float4x4 WorldMat : World;
  float4x4 WorldMatLast;
  float4x4 InvWorldMat;
  float4 Shipdata;
  float4 Clipdata1;
  float4 Miscdata;
  float4 ShLighting[7];
  float4 CustomMaskMaterialIDs[2];
  float4 CustomMaskTargets[2];
  float4 CustomMaskClamps;
  float4 ScreenSize;
} PerObjectPS  : register( ps, c40 );

[shader("closesthit")]
[globalinput(GLOBAL_INPUT)]
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{
}

[shader("anyhit")] 
void AnyHit( inout HitInfo payload, Attributes attrib ) 
{
	float3 positionOS = ObjectRayOrigin() + RayTCurrent() * ObjectRayDirection();
	float3 dist = positionOS - PerObjectPS.Clipdata1.xyz;
	if( sign( PerObjectPS.Clipdata1.w ) * dot( dist, dist ) - PerObjectPS.Clipdata1.w < 0 ) 
	{
		IgnoreHit();
	}
}

technique t0
{
	library p0
	{
		RayGenShader = compile lib_6_3 RayGen();
		MissShader = compile lib_6_3 Miss();
		payloadsize = 4;
	}
}

technique t1
{
	library p0
	{
		ClosestHitShader = compile lib_6_3 ClosestHit();
		AnyHitShader = compile lib_6_3 AnyHit();
		payloadsize = 4;
	}
}
)SRC";

	auto data = Compile<typename TestFixture::Compiler>( src );
	ASSERT_EQ( data.techniques[0].libraries[0].localInputs.registerInputs.size(), 1u );
	EXPECT_EQ( data.techniques[0].libraries[0].localInputs.registerInputs[0].registerType, RT_CONSTANT_BUFFER );

	ASSERT_FALSE( data.techniques[0].libraries[0].globalInputs.registerInputs.empty() );
}

TYPED_TEST( RayTracing, NumericConstantsAreInlined )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

static const float LocalInput = 123;

[shader("miss")]
void Miss(inout HitInfo payload)
{
	payload.visibility = LocalInput;
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

	auto data = Compile<typename TestFixture::Compiler>( src );
	EXPECT_TRUE( data.techniques[0].libraries[0].localInputs.constants.empty() );
	EXPECT_TRUE( data.techniques[0].libraries[0].globalInputs.constants.empty() );
}

TYPED_TEST( RayTracing, CanProvideLocalCBuffers )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

cbuffer PerFrame: register( b2, space7 )
{
    float coefficient;
};


[shader("miss")]
//[globalinput("PerFrame")]
void Miss(inout HitInfo payload)
{
	payload.visibility = coefficient;
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

	auto data = Compile<typename TestFixture::Compiler>( src );
	EXPECT_TRUE( data.techniques[0].libraries[0].globalInputs.constants.empty() );
	ASSERT_FALSE( data.techniques[0].libraries[0].localInputs.registerInputs.empty() );
	EXPECT_EQ( data.techniques[0].libraries[0].localInputs.registerInputs[0].registerType, RT_CONSTANT_BUFFER );
	EXPECT_EQ( data.techniques[0].libraries[0].localInputs.registerInputs[0].registerIndex, 2u );
}

TYPED_TEST( RayTracing, CanProvideGlobalCBuffers )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

struct Foo
{
	float coefficient;
};

cbuffer PerFrame: register( b2, space7 )
{
    Foo foo;
};


[shader("miss")]
[globalinput("PerFrame")]
void Miss(inout HitInfo payload)
{
	payload.visibility = foo.coefficient;
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

	auto data = Compile<typename TestFixture::Compiler>( src );
	EXPECT_TRUE( data.techniques[0].libraries[0].localInputs.constants.empty() );
	ASSERT_FALSE( data.techniques[0].libraries[0].globalInputs.registerInputs.empty() );
	EXPECT_EQ( data.techniques[0].libraries[0].globalInputs.registerInputs[0].registerType, RT_CONSTANT_BUFFER );
	EXPECT_EQ( data.techniques[0].libraries[0].globalInputs.registerInputs[0].registerIndex, 2u );
}

TYPED_TEST( RayTracing, NotUsedGlobalInputsAreAccepted )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

struct Foo
{
	float coefficient;
};

cbuffer PerFrame: register( b2 )
{
    Foo foo;
};

Texture2D Bar;

[shader("miss")]
[globalinput("PerFrame;Bar")]
void Miss(inout HitInfo payload)
{
	payload.visibility = 1;
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

	ASSERT_TRUE( Compiles<typename TestFixture::Compiler>( src ) );
}

TYPED_TEST( RayTracing, NotUsedGlobalInputsAreExported )
{
	const char* src = R"SRC(
struct HitInfo
{
    float visibility;
};

struct Foo
{
	float coefficient;
};

cbuffer PerFrame: register( b2 )
{
    Foo foo;
};

Texture2D Tex;
Texture2D Bar;

[shader("miss")]
[globalinput("PerFrame;Tex;Bar")]
void Miss(inout HitInfo payload)
{
	payload.visibility = // foo.coefficient + 
		Bar.Load( uint3( 0, 0, 0 ) ).r;
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

	auto data = Compile<typename TestFixture::Compiler>( src );
	ASSERT_EQ( data.techniques[0].libraries[0].globalInputs.textures.size(), 2 );
	ASSERT_EQ( data.techniques[0].libraries[0].globalInputs.registerInputs.size(), 3 );
}


TYPED_TEST( RayTracing, NotUsedSamplerGlobalInputsAreExported )
{
	const char* src = R"SRC(
SamplerState TestMapSampler1
{
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Point;
    AddressU  = Clamp;
    AddressV  = Clamp;
};

struct HitInfo
{
    float visibility;
};

struct Foo
{
	float coefficient;
};

cbuffer PerFrame: register( b2 )
{
    Foo foo;
};

Texture2D Tex;
Texture2D Bar;

[shader("miss")]
[globalinput("PerFrame;Tex;Bar;TestMapSampler1")]
void Miss(inout HitInfo payload)
{
	payload.visibility = // foo.coefficient + 
		Bar.Load( uint3( 0, 0, 0 ) ).r;
	//payload.visibility += Tex.SampleLevel( TestMapSampler1, float2( 0.5, 0.0 ), 0 ).r;
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

	auto data = Compile<typename TestFixture::Compiler>( src );
	ASSERT_EQ( data.techniques[0].libraries[0].globalInputs.textures.size(), 2 );
	ASSERT_EQ( data.techniques[0].libraries[0].globalInputs.registerInputs.size(), 3 );
	// on metal static samplers are not exposed
	if( !std::is_same<typename TestFixture::Compiler, EffectCompilerMetal>::value )
	{
		ASSERT_FALSE( data.techniques[0].libraries[0].globalInputs.staticSamplers.empty() );
	}
}



TYPED_TEST( RayTracing, CanCallTraceRayInFunction )
{
	const char* src = R"SRC(

RaytracingAccelerationStructure Scene <bool SasUiVisible = true; >;
RWTexture2D<float4> ShadowDest <bool SasUiVisible = true;>;


struct HitInfo
{
    float visibility;
};

float4x4 ProjectionInv;

#define GLOBAL_INPUT "Scene; ShadowDest;"

float GetVisibility( float2 dd )
{
	RayDesc ray;
	ray.Origin = float3( dd, 0 );
	ray.Direction = float3( 1, 0, 0 );

	ray.TMin = 0.0f;
	ray.TMax = 10000000.0; // camera far plane value
	
	HitInfo payload = {0.0};
	TraceRay(
		Scene,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
		0xFF,
		0,
		0,
		0,
		ray,
		payload );
	return payload.visibility;
}

[shader("raygeneration")]
[globalinput(GLOBAL_INPUT)]
void RayGen()
{
    uint3 launchIndex = DispatchRaysIndex();

	float2 dd = ( float2( launchIndex.xy ) + 0.5) / 500 * 2.0f - 1.0f;
	dd.y = -dd.y;

	ShadowDest[launchIndex.xy] = GetVisibility( dd );
}

technique t0
{
	library p0
	{
		RayGenShader = compile lib_6_3 RayGen();
		payloadsize = 4;
	}
}
)SRC";

	ASSERT_TRUE( Compiles<typename TestFixture::Compiler>( src ) );
}

TYPED_TEST( RayTracing, CanCallTraceRayInFunctionChain )
{
	const char* src = R"SRC(

RaytracingAccelerationStructure Scene <bool SasUiVisible = true; >;
RWTexture2D<float4> ShadowDest <bool SasUiVisible = true;>;


struct HitInfo
{
    float visibility;
};

float4x4 ProjectionInv;

#define GLOBAL_INPUT "Scene; ShadowDest;"

float GetVisibility( float2 dd )
{
	RayDesc ray;
	ray.Origin = float3( dd, 0 );
	ray.Direction = float3( 1, 0, 0 );

	ray.TMin = 0.0f;
	ray.TMax = 10000000.0; // camera far plane value
	
	HitInfo payload = {0.0};
	TraceRay(
		Scene,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
		0xFF,
		0,
		0,
		0,
		ray,
		payload );
	return payload.visibility;
}

float GetVisibility2( float2 dd )
{
	return GetVisibility( dd ) + GetVisibility( dd );
}

[shader("raygeneration")]
[globalinput(GLOBAL_INPUT)]
void RayGen()
{
    uint3 launchIndex = DispatchRaysIndex();

	float2 dd = ( float2( launchIndex.xy ) + 0.5) / 500 * 2.0f - 1.0f;
	dd.y = -dd.y;

	ShadowDest[launchIndex.xy] = GetVisibility2( dd );
}

technique t0
{
	library p0
	{
		RayGenShader = compile lib_6_3 RayGen();
		payloadsize = 4;
	}
}
)SRC";

	ASSERT_TRUE( Compiles<typename TestFixture::Compiler>( src ) );
}
