#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"

#if TRINITY_PLATFORM_SUPPORTS_COMPUTE

struct Compute: public WithValidRenderContext {};

using namespace Tr2RenderContextEnum;

TEST_F( Compute, CanReadCSResult )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( OutputVector.cs )
	};

	auto signature = Tr2ShaderSignatureAL().Add( Tr2ShaderRegisterAL::UAV, 0 );
	Tr2ShaderAL cs;
	ASSERT_HRESULT_SUCCEEDED( cs.Create( COMPUTE_SHADER, vsBytecode, signature, *renderContext ) );

	Tr2ShaderAL shaders[] = { cs };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 1, *renderContext ) );

	Tr2BufferAL output;
	ASSERT_HRESULT_SUCCEEDED( output.Create( PIXEL_FORMAT_R32G32B32A32_FLOAT, 1, Tr2GpuUsage::UNORDERED_ACCESS, Tr2CpuUsage::READ, nullptr, *renderContext ) );

	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetUav( Tr2RenderContextEnum::COMPUTE_SHADER, 0, output );
	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->RunComputeShader( 1, 1, 1 ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( Tr2ResourceSetAL() ) );

	const float* data;
	ASSERT_HRESULT_SUCCEEDED( output.MapForReading( data, *renderContext ) );

	EXPECT_EQ( 1.0f, data[0] );
	EXPECT_EQ( 2.0f, data[1] );
	EXPECT_EQ( 3.0f, data[2] );
	EXPECT_EQ( 4.0f, data[3] );
	output.UnmapForReading( *renderContext );
}


TEST_F( Compute, DISABLED_CanAddInCS )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( AddVectors.cs )
	};

	auto signature = Tr2ShaderSignatureAL()
		.Add( Tr2ShaderRegisterAL::UAV, 0 )
		.Add( Tr2ShaderRegisterAL::RESOURCE, 0 )
		.Add( Tr2ShaderRegisterAL::RESOURCE, 1 );

	Tr2ShaderAL cs;
	ASSERT_HRESULT_SUCCEEDED( cs.Create( COMPUTE_SHADER, vsBytecode, signature, *renderContext ) );

	Tr2ShaderAL shaders[] = { cs };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 1, *renderContext ) );

	float data1[] = { 1.f, 2.f, 3.f, 4.f };
	float data2[] = { 100.f, 200.f, 300.f, 400.f };

	Tr2BufferAL arg1;
	ASSERT_HRESULT_SUCCEEDED( arg1.Create( PIXEL_FORMAT_R32G32B32A32_FLOAT, 1, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, data1, *renderContext ) );
	Tr2BufferAL arg2;
	ASSERT_HRESULT_SUCCEEDED( arg2.Create( PIXEL_FORMAT_R32G32B32A32_FLOAT, 1, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, data2, *renderContext ) );

	Tr2BufferAL output;
	ASSERT_HRESULT_SUCCEEDED( output.Create( PIXEL_FORMAT_R32G32B32A32_FLOAT, 1, Tr2GpuUsage::UNORDERED_ACCESS, Tr2CpuUsage::READ, nullptr, *renderContext ) );

	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::COMPUTE_SHADER, 0, arg1 );
	desc.SetSrv( Tr2RenderContextEnum::COMPUTE_SHADER, 1, arg2 );
	desc.SetUav( Tr2RenderContextEnum::COMPUTE_SHADER, 0, output );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );


	ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->RunComputeShader( 1, 1, 1 ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( Tr2ResourceSetAL() ) );

	const float* data;
	ASSERT_HRESULT_SUCCEEDED( output.MapForReading( data, *renderContext ) );

	EXPECT_EQ( 101.0f, data[0] );
	EXPECT_EQ( 202.0f, data[1] );
	EXPECT_EQ( 303.0f, data[2] );
	EXPECT_EQ( 404.0f, data[3] );
	output.UnmapForReading( *renderContext );
}


TEST_F( Compute, CanAddConstantInCS )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( AddConstantToBuffer.cs )
	};

	auto signature = Tr2ShaderSignatureAL()
		.Add( Tr2ShaderRegisterAL::UAV, 0 )
		.Add( Tr2ShaderRegisterAL::RESOURCE, 0 )
		.Add( Tr2ShaderRegisterAL::CONSTANTS, 0 );

	Tr2ShaderAL cs;
	ASSERT_HRESULT_SUCCEEDED( cs.Create( COMPUTE_SHADER, vsBytecode, signature, *renderContext ) );

	Tr2ShaderAL shaders[] = { cs };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 1, *renderContext ) );

	float data1[] = { 1.f, 2.f, 3.f, 4.f };
	float data2[] = { 100.f, 200.f, 300.f, 400.f };

	Tr2BufferAL arg1;
	ASSERT_HRESULT_SUCCEEDED( arg1.Create( PIXEL_FORMAT_R32G32B32A32_FLOAT, 1, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::NONE, data1, *renderContext ) );
	Tr2ConstantBufferAL arg2;
	ASSERT_HRESULT_SUCCEEDED( arg2.Create( sizeof( data2 ), *renderContext ) );
	float* data;
	ASSERT_HRESULT_SUCCEEDED( arg2.Lock( (void**)&data, *renderContext ) );
	memcpy( data, data2, sizeof( data2 ) );
	ASSERT_HRESULT_SUCCEEDED( arg2.Unlock( *renderContext ) );

	Tr2BufferAL output;
	ASSERT_HRESULT_SUCCEEDED( output.Create( PIXEL_FORMAT_R32G32B32A32_FLOAT, 1, Tr2GpuUsage::UNORDERED_ACCESS, Tr2CpuUsage::READ, nullptr, *renderContext ) );

	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::COMPUTE_SHADER, 0, arg1 );
	desc.SetUav( Tr2RenderContextEnum::COMPUTE_SHADER, 0, output );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );


	ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );


	ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( arg2, COMPUTE_SHADER, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->RunComputeShader( 1, 1, 1 ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( Tr2ResourceSetAL() ) );

	const float* readData = nullptr;
	ASSERT_HRESULT_SUCCEEDED( output.MapForReading( readData, *renderContext ) );

	EXPECT_EQ( 101.0f, readData[0] );
	EXPECT_EQ( 202.0f, readData[1] );
	EXPECT_EQ( 303.0f, readData[2] );
	EXPECT_EQ( 404.0f, readData[3] );
	output.UnmapForReading( *renderContext );
}


TEST_F( Compute, DISABLED_CanRead2DTextureInCS )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTexture.cs )
	};

	auto signature = Tr2ShaderSignatureAL()
		.Add( Tr2ShaderRegisterAL::UAV, 0 )
		.Add( Tr2ShaderRegisterAL::RESOURCE, 0 )
		.Add( Tr2ShaderRegisterAL::SAMPLER, 0 );

	Tr2ShaderAL cs;
	ASSERT_HRESULT_SUCCEEDED( cs.Create( COMPUTE_SHADER, vsBytecode, signature, *renderContext ) );
	Tr2ShaderAL shaders[] = { cs };
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
	ASSERT_HRESULT_SUCCEEDED( input.Create( Tr2BitmapDimensions( 2, 2, 2, PIXEL_FORMAT_R32_FLOAT ), Tr2GpuUsage::SHADER_RESOURCE, texData, *renderContext ) );

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

	Tr2BufferAL output;
	ASSERT_HRESULT_SUCCEEDED( output.Create( PIXEL_FORMAT_R32G32B32A32_FLOAT, 1, Tr2GpuUsage::UNORDERED_ACCESS, Tr2CpuUsage::READ, nullptr, *renderContext ) );

	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::COMPUTE_SHADER, 0, input );
	desc.SetSampler( Tr2RenderContextEnum::COMPUTE_SHADER, 0, sampl );
	desc.SetUav( Tr2RenderContextEnum::COMPUTE_SHADER, 0, output );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );


	ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->RunComputeShader( 1, 1, 1 ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( Tr2ResourceSetAL() ) );

	const float* data;
	ASSERT_HRESULT_SUCCEEDED( output.MapForReading( data, *renderContext ) );

	EXPECT_EQ( 10.0f, data[0] );
	EXPECT_EQ( 2.5f, data[1] );
	output.UnmapForReading( *renderContext );
}


TEST_F( Compute, CanDispatchCSGroups )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( GroupShared.cs )
	};

	auto signature = Tr2ShaderSignatureAL()
		.Add( Tr2ShaderRegisterAL::UAV, 0 );

	Tr2ShaderAL cs;
	ASSERT_HRESULT_SUCCEEDED( cs.Create( COMPUTE_SHADER, vsBytecode, signature, *renderContext ) );
	Tr2ShaderAL shaders[] = { cs };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 1, *renderContext ) );

	Tr2BufferAL output;
	ASSERT_HRESULT_SUCCEEDED( output.Create( PIXEL_FORMAT_R32_UINT, 1, Tr2GpuUsage::UNORDERED_ACCESS, Tr2CpuUsage::READ, nullptr, *renderContext ) );

	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetUav( Tr2RenderContextEnum::COMPUTE_SHADER, 0, output );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );


	ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->RunComputeShader( 2, 2, 2 ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( Tr2ResourceSetAL() ) );

	const uint32_t* data;
	ASSERT_HRESULT_SUCCEEDED( output.MapForReading( data, *renderContext ) );

	EXPECT_EQ( 1900, data[0] );

	output.UnmapForReading( *renderContext );
}

TEST_F( Compute, CanUseBufferCounter )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( UseBufferCounter.cs )
	};

	auto signature = Tr2ShaderSignatureAL()
		.Add( Tr2ShaderRegisterAL::UAV, 0 );

	Tr2ShaderAL cs;
	ASSERT_HRESULT_SUCCEEDED( cs.Create( COMPUTE_SHADER, vsBytecode, signature, *renderContext ) );
	Tr2ShaderAL shaders[] = { cs };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 1, *renderContext ) );

	Tr2BufferAL output;
	ASSERT_HRESULT_SUCCEEDED( output.Create( 4, 4, Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::BUFFER_COUNTER, Tr2CpuUsage::READ, nullptr, *renderContext ) );

	const uint32_t initialValue = 5;

	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetUav( Tr2RenderContextEnum::COMPUTE_SHADER, 0, output, initialValue );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );


	ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->RunComputeShader( 1, 1, 1 ) );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( Tr2ResourceSetAL() ) );

	const uint32_t* data;
	ASSERT_HRESULT_SUCCEEDED( output.MapForReading( data, *renderContext ) );

	EXPECT_LE( initialValue, data[0] );
	EXPECT_GT( initialValue + 4, data[0] );

	EXPECT_LE( initialValue, data[1] );
	EXPECT_GT( initialValue + 4, data[1] );

	EXPECT_LE( initialValue, data[2] );
	EXPECT_GT( initialValue + 4, data[2] );

	EXPECT_LE( initialValue, data[3] );
	EXPECT_GT( initialValue + 4, data[3] );

	output.UnmapForReading( *renderContext );
}

#endif