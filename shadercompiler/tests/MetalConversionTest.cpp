// Copyright © 2024 CCP ehf.

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

TEST( MetalConversion, AllowBindlessResources )
{
	const char* src = R"SRC(

Buffer<float4> HeapView_BufferFloat4[]
<
	bool IsHeapView = true;
>;

Texture2D<float4> HeapView_Texture2D[]
<
	bool IsHeapView = true;
>;

struct VSOut
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
};

VSOut vs( float4 pos : POSITION, float2 uv : TEXCOORD0 )
{
	VSOut output;
	output.position = pos;
	output.uv = uv;
	return output;
}

float4 ps( VSOut vsOut ): SV_Target
{
	return HeapView_Texture2D[0].Load( uint2( vsOut.uv ) ) + HeapView_BufferFloat4[0][uint(vsOut.uv.x)];
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

	ASSERT_TRUE( Compiles<EffectCompilerMetal>( src ) );
}

TEST( MetalConversion, AllowLocalBufferVars )
{
	const char* src = R"SRC(

Buffer<float4> HeapView_BufferFloat4[]
<
	bool IsHeapView = true;
>;

struct VSOut
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
};

VSOut vs( float4 pos : POSITION, float2 uv : TEXCOORD0 )
{
	VSOut output;
	output.position = pos;
	output.uv = uv;
	return output;
}

float4 ps( VSOut vsOut ): SV_Target
{
	Buffer<float4> buf = HeapView_BufferFloat4[0];
	return buf.Load( uint( vsOut.uv.x ) );
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

	ASSERT_TRUE( Compiles<EffectCompilerMetal>( src ) );
}


TEST( MetalConversion, AllowManySrvsUavs )
{
	const char* src = R"SRC(

RWTexture2D<float4> Tex0;
Texture2D<float4> Tex1;
Texture2D<float4> Tex2;
Texture2D<float4> Tex3;
Texture2D<float4> Tex4;
Texture2D<float4> Tex5;
Texture2D<float4> Tex6;
Texture2D<float4> Tex7;
Texture2D<float4> Tex8;
Texture2D<float4> Tex9;

RWTexture2D<float4> Tex10;
Texture2D<float4> Tex11;
Texture2D<float4> Tex12;
Texture2D<float4> Tex13;
Texture2D<float4> Tex14;
Texture2D<float4> Tex15;
Texture2D<float4> Tex16;
Texture2D<float4> Tex17;
Texture2D<float4> Tex18;
Texture2D<float4> Tex19;

RWBuffer<float4> Buf20;
Buffer<float4> Buf21;
Buffer<float4> Buf22;
Buffer<float4> Buf23;
Buffer<float4> Buf24;
Buffer<float4> Buf25;
Buffer<float4> Buf26;
Buffer<float4> Buf27;
Buffer<float4> Buf28;
Buffer<float4> Buf29;


struct VSOut
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
};

VSOut vs( float4 pos : POSITION, float2 uv : TEXCOORD0 )
{
	VSOut output;
	output.position = pos;
	output.uv = uv;
	return output;
}

float4 ps( VSOut vsOut ): SV_Target
{
	float4 color = 0;
	color += Tex0[uint2( vsOut.uv )];
	color += Tex1.Load( uint2( vsOut.uv ) );
	color += Tex2.Load( uint2( vsOut.uv ) );
	color += Tex3.Load( uint2( vsOut.uv ) );
	color += Tex4.Load( uint2( vsOut.uv ) );
	color += Tex5.Load( uint2( vsOut.uv ) );
	color += Tex6.Load( uint2( vsOut.uv ) );
	color += Tex7.Load( uint2( vsOut.uv ) );
	color += Tex8.Load( uint2( vsOut.uv ) );
	color += Tex9.Load( uint2( vsOut.uv ) );
	
	color += Tex10[uint2( vsOut.uv )];
	color += Tex11.Load( uint2( vsOut.uv ) );
	color += Tex12.Load( uint2( vsOut.uv ) );
	color += Tex13.Load( uint2( vsOut.uv ) );
	color += Tex14.Load( uint2( vsOut.uv ) );
	color += Tex15.Load( uint2( vsOut.uv ) );
	color += Tex16.Load( uint2( vsOut.uv ) );
	color += Tex17.Load( uint2( vsOut.uv ) );
	color += Tex18.Load( uint2( vsOut.uv ) );
	color += Tex19.Load( uint2( vsOut.uv ) );
	
	color += Buf20[0];
	color += Buf21[0];
	color += Buf22[0];
	color += Buf23[0];
	color += Buf24[0];
	color += Buf25[0];
	color += Buf26[0];
	color += Buf27[0];
	color += Buf28[0];
	color += Buf29[0];

	return color;
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
	EXPECT_EQ( data.techniques[0].passes[0].stages[1].registerInputs.size(), 30 );
	EXPECT_EQ( data.techniques[0].passes[0].stages[1].textures.size(), 27 );
	EXPECT_EQ( data.techniques[0].passes[0].stages[1].uavs.size(), 3 );
	ASSERT_TRUE( Compiles<EffectCompilerMetal>( src ) );
}


TEST( MetalShaderPatching, CanHaveVertexID )
{
	const char* src = R"SRC(

struct VSIn
{
	float4 position : POSITION;
};

struct VSOut
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
};

VSOut vs( VSIn input, uint vertexID : SV_VertexID )
{
	VSOut output;
	output.position = input.position;
	output.uv = float2( vertexID, 0 );
	return output;
}

float4 ps( VSOut vsOut ) : SV_Target
{
	return float4( 1, 0, 0, 1 );
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
	EXPECT_NE( data.techniques[0].passes[0].stages[0].source.find( "[[ vertex_id ]]" ), std::string::npos );
	EXPECT_NE( data.techniques[0].passes[0].stages[0].source.find( "[[ base_vertex ]]" ), std::string::npos );
}

TEST( MetalShaderPatching, CanHaveVertexIDWithIAInputs )
{
	const char* src = R"SRC(

struct VSIn
{
	float4 position : POSITION;
	uint vertexID : SV_VertexID;
};

struct VSOut
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
};

VSOut vs( VSIn input )
{
	VSOut output;
	output.position = input.position;
	output.uv = float2( input.vertexID, 0 );
	return output;
}

float4 ps( VSOut vsOut ) : SV_Target
{
	return float4( 1, 0, 0, 1 );
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
	EXPECT_NE( data.techniques[0].passes[0].stages[0].source.find( "[[ vertex_id ]]" ), std::string::npos );
	EXPECT_NE( data.techniques[0].passes[0].stages[0].source.find( "[[ base_vertex ]]" ), std::string::npos );
}

TEST( MetalShaderPatching, CanHaveNestedStructs )
{
	const char* src = R"SRC(

struct VSIn
{
	float4 position : POSITION;
};

struct VSInExtended
{
	VSIn base;
	float2 tex : TEXCOORD;
};

struct VSOut
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
};

VSOut vs( VSInExtended input )
{
	VSOut output;
	output.position = input.base.position;
	output.uv = input.tex;
	return output;
}

float4 ps( VSOut vsOut ) : SV_Target
{
	return float4( 1, 0, 0, 1 );
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

	ASSERT_TRUE( Compiles<EffectCompilerMetal>( src ) );
}

TEST( MetalShaderPatching, CanHaveNestedStructsWithSystemSemanticsWithIAInputs1 )
{
	const char* src = R"SRC(

struct VSIn
{
	float4 position : POSITION;
	uint vertexID : SV_VertexID;
};

struct VSInExtended
{
	VSIn base;
	float2 tex : TEXCOORD;
};

struct VSOut
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
};

VSOut vs( VSInExtended input )
{
	VSOut output;
	output.position = input.base.position;
	output.uv = input.tex;
	output.uv1 = float2( input.base.vertexID, 0 );
	return output;
}

float4 ps( VSOut vsOut ) : SV_Target
{
	return float4( 1, 0, 0, 1 );
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

	ASSERT_TRUE( Compiles<EffectCompilerMetal>( src ) );
}

TEST( MetalShaderPatching, CanHaveNestedStructsWithSystemSemanticsWithIAInputs2 )
{
	const char* src = R"SRC(

struct VSIn
{
	float4 position : POSITION;
};

struct VSInExtended
{
	VSIn base;
	float2 tex : TEXCOORD;
	uint vertexID : SV_VertexID;
};

struct VSOut
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
};

VSOut vs( VSInExtended input )
{
	VSOut output;
	output.position = input.base.position;
	output.uv = input.tex;
	output.uv1 = float2( input.vertexID, 0 );
	return output;
}

float4 ps( VSOut vsOut ) : SV_Target
{
	return float4( 1, 0, 0, 1 );
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

	ASSERT_TRUE( Compiles<EffectCompilerMetal>( src ) );
}

TEST( MetalShaderPatching, CanHaveInstanceID )
{
	const char* src = R"SRC(

struct VSIn
{
	float4 position : POSITION;
};

struct VSOut
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
};

VSOut vs( VSIn input, uint instanceID : SV_InstanceID )
{
	VSOut output;
	output.position = input.position;
	output.uv = float2( instanceID, 0 );
	return output;
}

float4 ps( VSOut vsOut ) : SV_Target
{
	return float4( 1, 0, 0, 1 );
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
	EXPECT_NE( data.techniques[0].passes[0].stages[0].source.find( "[[ instance_id ]]" ), std::string::npos );
	EXPECT_NE( data.techniques[0].passes[0].stages[0].source.find( "[[ base_instance ]]" ), std::string::npos );
}

TEST( MetalShaderPatching, CanHaveInstanceIDWithIAInputs )
{
	const char* src = R"SRC(

struct VSIn
{
	float4 position : POSITION;
	uint instanceID : SV_InstanceID;
};

struct VSOut
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
};

VSOut vs( VSIn input )
{
	VSOut output;
	output.position = input.position;
	output.uv = float2( input.instanceID, 0 );
	return output;
}

float4 ps( VSOut vsOut ) : SV_Target
{
	return float4( 1, 0, 0, 1 );
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
	EXPECT_NE( data.techniques[0].passes[0].stages[0].source.find( "[[ instance_id ]]" ), std::string::npos );
	EXPECT_NE( data.techniques[0].passes[0].stages[0].source.find( "[[ base_instance ]]" ), std::string::npos );
}

TEST( MetalShaderPatching, CanHaveSystemSemanticMixedParameterAndIAInputs )
{
	const char* src = R"SRC(

struct VSIn
{
	float4 position : POSITION;
	uint instanceID : SV_InstanceID;
};

struct VSOut
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
};

VSOut vs( VSIn input, uint vertexID : SV_VertexID )
{
	VSOut output;
	output.position = input.position;
	output.uv = float2( input.instanceID, vertexID );
	return output;
}

float4 ps( VSOut vsOut ) : SV_Target
{
	return float4( 1, 0, 0, 1 );
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
	EXPECT_NE( data.techniques[0].passes[0].stages[0].source.find( "[[ vertex_id ]]" ), std::string::npos );
	EXPECT_NE( data.techniques[0].passes[0].stages[0].source.find( "[[ base_vertex ]]" ), std::string::npos );
	EXPECT_NE( data.techniques[0].passes[0].stages[0].source.find( "[[ instance_id ]]" ), std::string::npos );
	EXPECT_NE( data.techniques[0].passes[0].stages[0].source.find( "[[ base_instance ]]" ), std::string::npos );
}