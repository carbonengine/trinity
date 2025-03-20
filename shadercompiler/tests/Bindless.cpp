#include "gtest/gtest.h"
#include "EffectCompilerDX12.h"
#include "EffectCompilerMetal.h"
#include "EffectData.h"
#include "TesingUtils.h"

extern StringTable g_stringTable;

#if _WIN32
using BindlessCompiler = EffectCompilerDX12;
#else
using BindlessCompiler = EffectCompilerMetal;
#endif

TEST( Bindless, SamplerInitializerStored )
{
	const char* src = R"SRC(
Texture2D<float4> tex;

BindlessHandleSampler sampl{
    IsDynamic = true;
    MinFilter = Anisotropic;
    MagFilter = Linear;
    MipFilter = Linear;
    AddressU  = Wrap;
    AddressV  = Wrap;
};

SamplerState HeapView_Sampler[]
<
 bool IsHeapView = true;
>;

float4 vs(): SV_Position
{
	return float4( 0.0, 0.0, 0.0, 1.0 );
}

float4 ps(): SV_Target
{
	return float4( tex.Sample( HeapView_Sampler[sampl], float2( 0, 0 ) ) );
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

	auto data = Compile<BindlessCompiler>( src );

	ASSERT_FALSE( data.techniques[0].passes[0].stages[1].samplers.empty() );
	auto& samplers = data.techniques[0].passes[0].stages[1].samplers;
	auto sampl = find_if( begin( samplers ), end( samplers ), []( auto& s ) {
		return strcmp( g_stringTable.GetString( s.second.name ), "sampl" ) == 0;
	} );
	ASSERT_NE( sampl, end( samplers ) );
	EXPECT_EQ( sampl->second.addressU, 1 );
	EXPECT_EQ( sampl->second.addressV, 1 );
	EXPECT_EQ( sampl->second.addressW, 3 );
	EXPECT_EQ( sampl->second.minFilter, 3 );
}
