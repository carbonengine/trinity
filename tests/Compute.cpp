#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"

#if TRINITY_PLATFORM==TRINITY_OPENGL4 || TRINITY_PLATFORM==TRINITY_DIRECTX11

struct Compute: public WithValidRenderContext {};

using namespace Tr2RenderContextEnum;

TEST_F( Compute, CanReadCSResult )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( OutputVector.cs )
	};

	Tr2ShaderInputDefinition def;
	def.elements.resize( 1 );
	def.elements[0].usageIndex = 0;
	def.elements[0].registerIndex = 0;
	def.ComputeHash();

	Tr2ShaderAL cs;
	ASSERT_HRESULT_SUCCEEDED( cs.Create( 
		*renderContext, 
		COMPUTE_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		def ) );

	Tr2ShaderAL* shaders[] = { &cs };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 1, *renderContext ) );

	Tr2GpuBufferAL output;
	ASSERT_HRESULT_SUCCEEDED( output.Create( 1, PIXEL_FORMAT_R32G32B32A32_FLOAT, USAGE_CPU_READ, EX_NONE, nullptr, *renderContext ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetUav( COMPUTE_SHADER, 0, output ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->RunComputeShader( 1, 1, 1 ) );

	float* data;
	ASSERT_HRESULT_SUCCEEDED( output.Lock( 0, 0, (void**)&data, LOCK_READONLY, *renderContext ) );

	EXPECT_EQ( 1.0f, data[0] );
	EXPECT_EQ( 2.0f, data[1] );
	EXPECT_EQ( 3.0f, data[2] );
	EXPECT_EQ( 4.0f, data[3] );
	ASSERT_HRESULT_SUCCEEDED( output.Unlock( *renderContext ) );
}


TEST_F( Compute, DISABLED_CanAddInCS )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( AddVectors.cs )
	};

	Tr2ShaderInputDefinition def;
	def.elements.resize( 3 );
	def.elements[0].usageIndex = 1;
	def.elements[0].registerIndex = 0;
	def.elements[1].usageIndex = 1;
	def.elements[1].registerIndex = 1;
	def.elements[2].usageIndex = 0;
	def.elements[2].registerIndex = 0;
	def.ComputeHash();

	Tr2ShaderAL cs;
	ASSERT_HRESULT_SUCCEEDED( cs.Create( 
		*renderContext, 
		COMPUTE_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		def ) );

	Tr2ShaderAL* shaders[] = { &cs };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 1, *renderContext ) );

	float data1[] = { 1.f, 2.f, 3.f, 4.f };
	float data2[] = { 100.f, 200.f, 300.f, 400.f };

	Tr2GpuBufferAL arg1;
	ASSERT_HRESULT_SUCCEEDED( arg1.Create( 1, PIXEL_FORMAT_R32G32B32A32_FLOAT, USAGE_CPU_READ, EX_NONE, data1, *renderContext ) );
	Tr2GpuBufferAL arg2;
	ASSERT_HRESULT_SUCCEEDED( arg2.Create( 1, PIXEL_FORMAT_R32G32B32A32_FLOAT, USAGE_CPU_READ, EX_NONE, data2, *renderContext ) );

	Tr2GpuBufferAL output;
	ASSERT_HRESULT_SUCCEEDED( output.Create( 1, PIXEL_FORMAT_R32G32B32A32_FLOAT, USAGE_CPU_READ, EX_NONE, nullptr, *renderContext ) );

	Tr2ResourceSetDescriptionAL desc;
	desc.Set( Tr2RenderContextEnum::COMPUTE_SHADER, 0, arg1 );
	desc.Set( Tr2RenderContextEnum::COMPUTE_SHADER, 1, arg2 );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, *renderContext ) );


	ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetUav( COMPUTE_SHADER, 0, output ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->RunComputeShader( 1, 1, 1 ) );

	float* data;
	ASSERT_HRESULT_SUCCEEDED( output.Lock( 0, 0, (void**)&data, LOCK_READONLY, *renderContext ) );

	EXPECT_EQ( 101.0f, data[0] );
	EXPECT_EQ( 202.0f, data[1] );
	EXPECT_EQ( 303.0f, data[2] );
	EXPECT_EQ( 404.0f, data[3] );
	ASSERT_HRESULT_SUCCEEDED( output.Unlock( *renderContext ) );
}


TEST_F( Compute, CanAddConstantInCS )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( AddConstantToBuffer.cs )
	};

	Tr2ShaderInputDefinition def;
	def.elements.resize( 3 );
	def.elements[0].usageIndex = 2;
	def.elements[0].registerIndex = 0;
	def.elements[1].usageIndex = 1;
	def.elements[1].registerIndex = 0;
	def.elements[2].usageIndex = 0;
	def.elements[2].registerIndex = 0;
	def.ComputeHash();

	Tr2ShaderAL cs;
	ASSERT_HRESULT_SUCCEEDED( cs.Create( 
		*renderContext, 
		COMPUTE_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		def ) );

	Tr2ShaderAL* shaders[] = { &cs };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 1, *renderContext ) );

	float data1[] = { 1.f, 2.f, 3.f, 4.f };
	float data2[] = { 100.f, 200.f, 300.f, 400.f };

	Tr2GpuBufferAL arg1;
	ASSERT_HRESULT_SUCCEEDED( arg1.Create( 1, PIXEL_FORMAT_R32G32B32A32_FLOAT, USAGE_CPU_READ, EX_NONE, data1, *renderContext ) );
	Tr2ConstantBufferAL arg2;
	ASSERT_HRESULT_SUCCEEDED( arg2.Create( sizeof( data2 ), *renderContext ) );
	float* data;
	ASSERT_HRESULT_SUCCEEDED( arg2.Lock( (void**)&data, *renderContext ) );
	memcpy( data, data2, sizeof( data2 ) );
	ASSERT_HRESULT_SUCCEEDED( arg2.Unlock( *renderContext ) );

	Tr2GpuBufferAL output;
	ASSERT_HRESULT_SUCCEEDED( output.Create( 1, PIXEL_FORMAT_R32G32B32A32_FLOAT, USAGE_CPU_READ, EX_NONE, nullptr, *renderContext ) );

	Tr2ResourceSetDescriptionAL desc;
	desc.Set( Tr2RenderContextEnum::COMPUTE_SHADER, 0, arg1 );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, *renderContext ) );


	ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );


	ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( arg2, COMPUTE_SHADER, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetUav( COMPUTE_SHADER, 0, output ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->RunComputeShader( 1, 1, 1 ) );

	ASSERT_HRESULT_SUCCEEDED( output.Lock( 0, 0, (void**)&data, LOCK_READONLY, *renderContext ) );

	EXPECT_EQ( 101.0f, data[0] );
	EXPECT_EQ( 202.0f, data[1] );
	EXPECT_EQ( 303.0f, data[2] );
	EXPECT_EQ( 404.0f, data[3] );
	ASSERT_HRESULT_SUCCEEDED( output.Unlock( *renderContext ) );
}


TEST_F( Compute, DISABLED_CanRead2DTextureInCS )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTexture.cs )
	};

	Tr2ShaderInputDefinition def;
	def.elements.resize( 3 );
	def.elements[0].usageIndex = 1;
	def.elements[0].registerIndex = 0;
	def.elements[1].usageIndex = 3;
	def.elements[1].registerIndex = 0;
	def.elements[2].usageIndex = 0;
	def.elements[2].registerIndex = 0;
	def.ComputeHash();

	Tr2ShaderAL cs;
	ASSERT_HRESULT_SUCCEEDED( cs.Create( 
		*renderContext, 
		COMPUTE_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		def ) );
	Tr2ShaderAL* shaders[] = { &cs };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 1, *renderContext ) );

	float data1[] = { 1.f, 2.f, 3.f, 4.f };
	float data2[] = { 100.f };

	Tr2SubresourceData texData[2];
	texData[0].m_sysMem = data1;
	texData[0].m_sysMemPitch = 2 * sizeof( float );
	texData[0].m_sysMemSlicePitch = sizeof( data1 );
	texData[1].m_sysMem = data2;
	texData[1].m_sysMemPitch = sizeof( float );
	texData[1].m_sysMemSlicePitch = sizeof( data2 );


	Tr2TextureAL input;
	ASSERT_HRESULT_SUCCEEDED( input.Create2D( 2, 2, 2, PIXEL_FORMAT_R32_FLOAT, USAGE_IMMUTABLE, texData, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_LINEAR,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			100.f ),
		*renderContext ) );

	Tr2ResourceSetDescriptionAL desc;
	desc.Set( Tr2RenderContextEnum::COMPUTE_SHADER, 0, input );
	desc.Set( Tr2RenderContextEnum::COMPUTE_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, *renderContext ) );


	ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );


	Tr2GpuBufferAL output;
	ASSERT_HRESULT_SUCCEEDED( output.Create( 1, PIXEL_FORMAT_R32G32B32A32_FLOAT, USAGE_CPU_READ, EX_NONE, nullptr, *renderContext ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetUav( COMPUTE_SHADER, 0, output ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->RunComputeShader( 1, 1, 1 ) );

	float* data;
	ASSERT_HRESULT_SUCCEEDED( output.Lock( 0, 0, (void**)&data, LOCK_READONLY, *renderContext ) );

	EXPECT_EQ( 10.0f, data[0] );
	EXPECT_EQ( 2.5f, data[1] );
	ASSERT_HRESULT_SUCCEEDED( output.Unlock( *renderContext ) );
}


TEST_F( Compute, CanDispatchCSGroups )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( GroupShared.cs )
	};

	Tr2ShaderInputDefinition def;
	def.elements.resize( 1 );
	def.elements[0].usageIndex = 0;
	def.elements[0].registerIndex = 0;
	def.ComputeHash();

	Tr2ShaderAL cs;
	ASSERT_HRESULT_SUCCEEDED( cs.Create( 
		*renderContext, 
		COMPUTE_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		def ) );
	Tr2ShaderAL* shaders[] = { &cs };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 1, *renderContext ) );

	Tr2GpuBufferAL output;
	ASSERT_HRESULT_SUCCEEDED( output.Create( 1, PIXEL_FORMAT_R32_UINT, USAGE_CPU_READ, EX_NONE, nullptr, *renderContext ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetUav( COMPUTE_SHADER, 0, output ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->RunComputeShader( 2, 2, 2 ) );

	uint32_t* data;
	ASSERT_HRESULT_SUCCEEDED( output.Lock( 0, 0, (void**)&data, LOCK_READONLY, *renderContext ) );

	EXPECT_EQ( 1900, data[0] );

	ASSERT_HRESULT_SUCCEEDED( output.Unlock( *renderContext ) );
}

#endif