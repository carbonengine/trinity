#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"

using namespace Tr2RenderContextEnum;

struct Rendering: public WithValidRenderContext {};

namespace
{

struct PerObjectData
{
	float x;
	float y;
	float padding[2];
};


ALResult CreatePositionOnlyVS( Tr2ShaderAL& shader, Tr2PrimaryRenderContextAL& renderContext )
{
	uint32_t bytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
	};

	auto input = Tr2ShaderSignatureAL().Add( Tr2VertexDefinition::POSITION, 0, 0 );

	return shader.Create( VERTEX_SHADER, bytecode, input, renderContext );
}

ALResult CreateConstantColorPS( Tr2ShaderAL& shader, Tr2PrimaryRenderContextAL& renderContext )
{
	uint32_t bytecode[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
	};

	return shader.Create( PIXEL_SHADER, bytecode, Tr2ShaderSignatureAL(), renderContext );
}

ALResult CreateTexCoordAndPositionVS( Tr2ShaderAL& shader, Tr2PrimaryRenderContextAL& renderContext )
{
	uint32_t bytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	auto input = Tr2ShaderSignatureAL()
		.Add( Tr2VertexDefinition::TEXCOORD, 0, 0 )
		.Add( Tr2VertexDefinition::POSITION, 0, 1 );

	return shader.Create( VERTEX_SHADER, bytecode, input, renderContext );
}

ALResult CreateSampleTextureFromTexCoordPS( Tr2ShaderAL& shader, Tr2PrimaryRenderContextAL& renderContext )
{
	uint32_t bytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	auto input = Tr2ShaderSignatureAL()
		.Add( Tr2ShaderRegisterAL::RESOURCE, 0 )
		.Add( Tr2ShaderRegisterAL::SAMPLER, 0 );

	return shader.Create( PIXEL_SHADER, bytecode, input, renderContext );
}

ALResult CreatePositionOnlyWithPerObjectDataVS( Tr2ShaderAL& shader, Tr2PrimaryRenderContextAL& renderContext )
{
	uint32_t bytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnlyWithPerObjectData.vs )
	};

	auto input = Tr2ShaderSignatureAL()
		.Add( Tr2VertexDefinition::TEXCOORD, 0, 0 )
		.Add( Tr2VertexDefinition::POSITION, 0, 1 )
		.Add( Tr2ShaderRegisterAL::CONSTANTS, 0 );

	return shader.Create( VERTEX_SHADER, bytecode, input, renderContext );
}

ALResult CreateInstancedRenderingVS( Tr2ShaderAL& shader, Tr2PrimaryRenderContextAL& renderContext )
{
	uint32_t bytecode[] = {
#include INCLUDE_SHADER_CODE( InstancedRendering.vs )
	};

	auto input = Tr2ShaderSignatureAL()
		.Add( Tr2VertexDefinition::POSITION, 0, 0 )
		.Add( Tr2VertexDefinition::POSITION, 8, 1 );

	return shader.Create( VERTEX_SHADER, bytecode, input, renderContext );
}

ALResult CreateOutputTexCoordPS( Tr2ShaderAL& shader, Tr2PrimaryRenderContextAL& renderContext )
{
	uint32_t bytecode[] = {
#include INCLUDE_SHADER_CODE( OutputTexCoord.ps )
	};

	return shader.Create( PIXEL_SHADER, bytecode, Tr2ShaderSignatureAL(), renderContext );
}

ALResult CreateSampleTextureMipFromTexCoordPS( Tr2ShaderAL& shader, Tr2PrimaryRenderContextAL& renderContext )
{
	uint32_t bytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureMipFromTexCoord.ps )
	};

	auto input = Tr2ShaderSignatureAL()
		.Add( Tr2ShaderRegisterAL::CONSTANTS, 0 )
		.Add( Tr2ShaderRegisterAL::RESOURCE, 0 )
		.Add( Tr2ShaderRegisterAL::SAMPLER, 0 );

	return shader.Create( PIXEL_SHADER, bytecode, input, renderContext );
}

ALResult CreateSampleVolumeTexturePS( Tr2ShaderAL& shader, Tr2PrimaryRenderContextAL& renderContext )
{
	uint32_t bytecode[] = {
#include INCLUDE_SHADER_CODE( SampleVolumeTexture.ps )
	};

	auto input = Tr2ShaderSignatureAL()
		.Add( Tr2ShaderRegisterAL::CONSTANTS, 0 )
		.Add( Tr2ShaderRegisterAL::RESOURCE, 0 )
		.Add( Tr2ShaderRegisterAL::SAMPLER, 0 );

	return shader.Create( PIXEL_SHADER, bytecode, input, renderContext );
}

#if TRINITY_PLATFORM_SUPPORTS_UNORDERED_ACCESS

ALResult CreateWriteToUavPS( Tr2ShaderAL& shader, Tr2PrimaryRenderContextAL& renderContext )
{
	uint32_t bytecode[] = {
#include INCLUDE_SHADER_CODE( WriteToUav.ps )
	};

	auto input = Tr2ShaderSignatureAL()
		.Add( Tr2ShaderRegisterAL::UAV, 1 );

	return shader.Create( PIXEL_SHADER, bytecode, input, renderContext );
}

#endif

#if TRINITY_PLATFORM_SUPPORTS_MSAA_SAMPLE

ALResult CreateLoadMsaaTexturePS( Tr2ShaderAL& shader, Tr2PrimaryRenderContextAL& renderContext )
{
	uint32_t bytecode[] = {
#include INCLUDE_SHADER_CODE( LoadMsaaTexture.ps )
	};

	auto input = Tr2ShaderSignatureAL()
		.Add( Tr2ShaderRegisterAL::RESOURCE, 0 );

	return shader.Create( PIXEL_SHADER, bytecode, input, renderContext );
}

#endif

}

TEST_F( Rendering, CanClearBackBuffer )
{
	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );
}

TEST_F( Rendering, CanRenderASingleTriangle )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreatePositionOnlyVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};
	const uint32_t vbStride = 3 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

TEST_F( Rendering, CanRenderTriangleStrip )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreatePositionOnlyVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.5f, 0.5f, 0.0f,
	};
	const uint32_t vbStride = 3 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xffff0000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

TEST_F( Rendering, CanRenderIndexedTriangles )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreatePositionOnlyVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.5f, 0.5f, 0.0f,
	};
	const uint32_t vbStride = 3 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	uint16_t indices[] = { 0, 1, 2, 1, 2, 3 };
	Tr2BufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT, sizeof( indices ) / sizeof( indices[0] ), Tr2GpuUsage::INDEX_BUFFER, Tr2CpuUsage::NONE, indices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff550000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetIndices( ib ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawIndexedPrimitive( 4, 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetIndices( Tr2BufferAL() ) );
}

TEST_F( Rendering, CanReorderInputsToVertexShader )
{
	// This test case checks if AL will correctly match (Position, TexCoord) vertex layout
	// to (TexCoord, Position) vertex input (notice the different order here). This test
	// mostly makes sence for platforms that don't provide out of the box layout matching
	// (these are OpenGL and Orbis).

	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	auto vsInput = Tr2ShaderSignatureAL()
		.Add( Tr2VertexDefinition::TEXCOORD, 0, 0 )
		.Add( Tr2VertexDefinition::POSITION, 0, 1 );

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( VERTEX_SHADER, vsBytecode, vsInput, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateOutputTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

TEST_F( Rendering, CanSampleTexture )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleTextureFromTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t texturePixels0[] = {
		0xff00ff00, 0xff0000ff, 0xff00ff00, 0xff0000ff,
		0xffff0000, 0x00ffffff, 0xffff0000, 0x00ffffff,
		0xff00ff00, 0xff0000ff, 0xff00ff00, 0xff0000ff,
		0xffff0000, 0x00ffffff, 0xffff0000, 0x00ffffff,
	};
	Tr2SubresourceData textureData[1];
	textureData[0].m_sysMem = texturePixels0;
	textureData[0].m_sysMemPitch = 16;
	textureData[0].m_sysMemSlicePitch = sizeof( texturePixels0 );

	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 4, 4, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, textureData, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );

	uint32_t g = 127;

	Tr2ResourceSetDescriptionAL resourceSetDescription( sp );
	resourceSetDescription.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex );
	resourceSetDescription.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( resourceSetDescription, sp, *renderContext ) );

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}


TEST_F( Rendering, CanSampleMipMappedTexture )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleTextureFromTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t texturePixels0[] = {
		0xff00ff00, 0xff0000ff, 0xff00ff00, 0xff0000ff,
		0xffff0000, 0x00ffffff, 0xffff0000, 0x00ffffff,
		0xff00ff00, 0xff0000ff, 0xff00ff00, 0xff0000ff,
		0xffff0000, 0x00ffffff, 0xffff0000, 0x00ffffff,
	};
	uint32_t texturePixels1[] = {
		0xff00ff00, 0xff0000ff,
		0xffff0000, 0x00ffffff,
	};
	uint32_t texturePixels2[] = {
		0xff00ff00,
	};
	Tr2SubresourceData textureData[3];
	textureData[0].m_sysMem = texturePixels0;
	textureData[0].m_sysMemPitch = 16;
	textureData[0].m_sysMemSlicePitch = sizeof( texturePixels0 );
	textureData[1].m_sysMem = texturePixels1;
	textureData[1].m_sysMemPitch = 8;
	textureData[1].m_sysMemSlicePitch = sizeof( texturePixels1 );
	textureData[2].m_sysMem = texturePixels2;
	textureData[2].m_sysMemPitch = 4;
	textureData[2].m_sysMemSlicePitch = sizeof( texturePixels2 );

	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 4, 4, 3, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, textureData, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {

		Tr2SamplerStateAL sampl;
		ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
			Tr2SamplerDescription( 
				Tr2RenderContextEnum::TF_LINEAR,
				Tr2RenderContextEnum::TA_WRAP,
				1,
				float( ( g & 0xff ) / 128 ),
				std::numeric_limits<float>::max() ),
			*renderContext ) );

		Tr2ResourceSetDescriptionAL desc( sp );
		desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex );
		desc.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

		Tr2ResourceSetAL resourceSet;
		ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );


		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

TEST_F( Rendering, CanPassConstantBufferToRendering )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreatePositionOnlyWithPerObjectDataVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};
	const uint32_t vbStride = 3 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	Tr2ConstantBufferAL cb;
	ASSERT_HRESULT_SUCCEEDED( cb.Create( sizeof( PerObjectData ), *renderContext ) );


	uint32_t g = 127;

	auto frame = [&] {
		PerObjectData* data;
		ASSERT_HRESULT_SUCCEEDED( cb.Lock( (void**)&data, *renderContext ) );
		data->x = float( g % 100 ) / 100.0f - 0.5f;
		data->y = 0;
		ASSERT_HRESULT_SUCCEEDED( cb.Unlock( *renderContext ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff00ff00 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( cb, Tr2RenderContextEnum::VERTEX_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( Tr2ConstantBufferAL(), Tr2RenderContextEnum::VERTEX_SHADER, 0 ) );
}

TEST_F( Rendering, CanDoInstancedRendering )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateInstancedRenderingVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.25f, -0.25f, 0.0f,
		-0.25f, 0.25f, 0.0f,
		0.25f, -0.25f, 0.0f,
		0.25f, 0.25f, 0.0f,
	};
	const uint32_t vbStride = 3 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	uint16_t indices[] = { 0, 1, 2, 1, 2, 3 };
	Tr2BufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT, sizeof( indices ) / sizeof( indices[0] ), Tr2GpuUsage::INDEX_BUFFER, Tr2CpuUsage::NONE, indices, *renderContext ) );

	float instances[] = {
		-0.5f, 0.2f,
		0.5f, -0.2f,
	};
	const uint32_t instanceVbStride = 2 * sizeof( float );
	Tr2BufferAL instanceVb;
	ASSERT_HRESULT_SUCCEEDED( instanceVb.Create( instanceVbStride, sizeof( instances ) / instanceVbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, instances, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::POSITION, 8, 1, 1 );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff00ff00 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 1, instanceVb, 0, instanceVbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetIndices( ib ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawIndexedInstanced( 4, 0, 2, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 1, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

TEST_F( Rendering, CanClearRenderTarget )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleTextureFromTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 

		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );

	Tr2TextureAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( Tr2BitmapDimensions( 128, 64, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE, *renderContext ) );

	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, rt );
	desc.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );


	uint32_t g = 127;

	auto frame = [&] {

		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushRenderTarget() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( rt ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetDepthStencil( Tr2TextureAL() ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 16 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PopDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PopRenderTarget() );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

TEST_F( Rendering, CanRenderToRenderTarget )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleTextureFromTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	Tr2ShaderAL psFill;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( psFill, *renderContext ) );

	Tr2ShaderAL shadersFill[] = { vs, psFill };
	Tr2ShaderProgramAL spFill;
	ASSERT_HRESULT_SUCCEEDED( spFill.Create( shadersFill, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 

		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );

	Tr2TextureAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( Tr2BitmapDimensions( 128, 64, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE, *renderContext ) );


	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, rt );
	desc.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {

		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushRenderTarget() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( rt ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetDepthStencil( Tr2TextureAL() ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 0 ), 1.0f ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( spFill ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->PopDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PopRenderTarget() );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

TEST_F( Rendering, CanRenderToMsaaRenderTarget )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleTextureFromTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	Tr2ShaderAL psFill;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( psFill, *renderContext ) );

	Tr2ShaderAL shadersFill[] = { vs, psFill };
	Tr2ShaderProgramAL spFill;
	ASSERT_HRESULT_SUCCEEDED( spFill.Create( shadersFill, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 

		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );

	Tr2TextureAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( Tr2BitmapDimensions( 128, 64, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2MsaaDesc( 4 ), Tr2GpuUsage::RENDER_TARGET, *renderContext ) );

	Tr2TextureAL readableRt;
	ASSERT_HRESULT_SUCCEEDED( readableRt.Create( Tr2BitmapDimensions( 128, 64, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE, *renderContext ) );


	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, readableRt );
	desc.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {

		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushRenderTarget() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( rt ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetDepthStencil( Tr2TextureAL() ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 0 ), 1.0f ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( spFill ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->PopDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PopRenderTarget() );

		ASSERT_HRESULT_SUCCEEDED( rt.Resolve( readableRt, *renderContext ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}


TEST_F( Rendering, CanClearDepthBuffer )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreatePositionOnlyVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 1.0f,
	};
	const uint32_t vbStride = 3 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	auto backBuffer = renderContext->GetDefaultBackBuffer();

	Tr2TextureAL depthBuffer;
	ASSERT_HRESULT_SUCCEEDED( depthBuffer.Create( Tr2BitmapDimensions( backBuffer.GetWidth(), backBuffer.GetHeight(), 1, Tr2RenderContextEnum::PIXEL_FORMAT_D24_UNORM_S8_UINT ), Tr2GpuUsage::DEPTH_STENCIL, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetDepthStencil( depthBuffer ) );
		float depth = float( g & 0xff ) / 255.f;
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET | Tr2RenderContextEnum::CLEARFLAGS_ZBUFFER, 0xff000000 | ( g & 0xff ), depth ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZFUNC, Tr2RenderContextEnum::CMP_LESSEQUAL ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PopDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

TEST_F( Rendering, CanRenderIntoDepthBuffer )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateOutputTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
		0.5f, -0.5f, 0.0f, 1.0f, 0.0f,

		0.5f, -0.5f, 0.5f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
		-0.5f, -0.5f, 0.5f, 1.0f, 1.0f,
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	auto backBuffer = renderContext->GetDefaultBackBuffer();

	Tr2TextureAL depthBuffer;
	ASSERT_HRESULT_SUCCEEDED( depthBuffer.Create( Tr2BitmapDimensions( backBuffer.GetWidth(), backBuffer.GetHeight(), 1, Tr2RenderContextEnum::PIXEL_FORMAT_D24_UNORM_S8_UINT ), Tr2GpuUsage::DEPTH_STENCIL, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetDepthStencil( depthBuffer ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET | Tr2RenderContextEnum::CLEARFLAGS_ZBUFFER, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZWRITEENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZFUNC, Tr2RenderContextEnum::CMP_LESSEQUAL ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PopDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

TEST_F( Rendering, CanSampleDepthBuffer )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateOutputTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL ps2;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleTextureFromTexCoordPS( ps2, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	Tr2ShaderAL shaders2[] = { vs, ps2 };
	Tr2ShaderProgramAL sp2;
	ASSERT_HRESULT_SUCCEEDED( sp2.Create( shaders2, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
		0.5f, -0.5f, 0.0f, 1.0f, 0.0f,

		0.5f, -0.5f, 0.5f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
		-0.5f, -0.5f, 0.5f, 1.0f, 1.0f,
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	float quad[] = {
		0, 0, 0, 0, 1,
		0, 1, 0, 0, 0,
		1, 0, 0, 1, 1,

		1, 0, 0, 1, 1,
		0, 1, 0, 0, 0,
		1, 1, 0, 1, 0,
	};
	Tr2BufferAL quadVb;
	ASSERT_HRESULT_SUCCEEDED( quadVb.Create( vbStride, sizeof( quad ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, quad, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create(
		Tr2SamplerDescription(
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	auto backBuffer = renderContext->GetDefaultBackBuffer();

	Tr2TextureAL depthBuffer;
	ASSERT_HRESULT_SUCCEEDED( depthBuffer.Create( 
		Tr2BitmapDimensions( backBuffer.GetWidth(), backBuffer.GetHeight(), 1, Tr2RenderContextEnum::PIXEL_FORMAT_D24_UNORM_S8_UINT ), 
		Tr2GpuUsage::DEPTH_STENCIL | Tr2GpuUsage::SHADER_RESOURCE, 
		*renderContext ) );

	Tr2ResourceSetDescriptionAL resourceSetDescription( sp2 );
	resourceSetDescription.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, depthBuffer );
	resourceSetDescription.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( resourceSetDescription, sp2, *renderContext ) );


	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetDepthStencil( depthBuffer ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET | Tr2RenderContextEnum::CLEARFLAGS_ZBUFFER, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZWRITEENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZFUNC, Tr2RenderContextEnum::CMP_LESSEQUAL ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );

		renderContext->SetReadOnlyDepth( true );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZWRITEENABLE, 0 ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, quadVb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );

		renderContext->SetReadOnlyDepth( false );

		ASSERT_HRESULT_SUCCEEDED( renderContext->PopDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	renderContext->SetReadOnlyDepth( false );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

TEST_F( Rendering, CanRenderASingleTriangleWithDrawPrimitiveUP )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreatePositionOnlyVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitiveUP( 1, vertices, sizeof( float ) * 3 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

TEST_F( Rendering, CanRenderIndexedTrianglesWith16BitDrawIndexedPrimitiveUP )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreatePositionOnlyVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.5f, 0.5f, 0.0f,
	};
	uint16_t indices[] = { 0, 1, 2, 1, 2, 3 };

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff550000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawIndexedPrimitiveUP( 4, 2, indices, vertices, sizeof( float ) * 3 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetIndices( Tr2BufferAL() ) );
}

TEST_F( Rendering, CanRenderIndexedTrianglesWith32BitDrawIndexedPrimitiveUP )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreatePositionOnlyVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.5f, 0.5f, 0.0f,
	};
	uint32_t indices[] = { 0, 1, 2, 1, 2, 3 };

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff550000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawIndexedPrimitiveUP( 4, 2, indices, vertices, sizeof( float ) * 3 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetIndices( Tr2BufferAL() ) );
}

TEST_F( Rendering, CanUseOcclusionQueries )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreatePositionOnlyWithPerObjectDataVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};
	float verticesBg[] = {
		-0.5f, -0.5f, 0.5f,
		-0.5f, 0.5f, 0.5f,
		0.5f, -0.5f, 0.5f,
	};
	const uint32_t vbStride = 3 * sizeof( float );

	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2BufferAL vbBg;
	ASSERT_HRESULT_SUCCEEDED( vbBg.Create( vbStride, sizeof( verticesBg ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, verticesBg, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	Tr2ConstantBufferAL cb;
	ASSERT_HRESULT_SUCCEEDED( cb.Create( sizeof( PerObjectData ), *renderContext ) );

	Tr2OcclusionQueryAL queryOccluded;
	ASSERT_HRESULT_SUCCEEDED( queryOccluded.Create( *renderContext ) );
	Tr2OcclusionQueryAL queryTotal;
	ASSERT_HRESULT_SUCCEEDED( queryTotal.Create( *renderContext ) );

	uint32_t g = 127;
	uint32_t occludedPixelCount = 0;
	uint32_t totalPixelCount = 0;

	bool issueQueries = true;

	auto backBuffer = renderContext->GetDefaultBackBuffer();

	Tr2TextureAL depthBuffer;
	ASSERT_HRESULT_SUCCEEDED( depthBuffer.Create( Tr2BitmapDimensions( backBuffer.GetWidth(), backBuffer.GetHeight(), 1, Tr2RenderContextEnum::PIXEL_FORMAT_D24_UNORM_S8_UINT ), Tr2GpuUsage::DEPTH_STENCIL, *renderContext ) );

	auto frame = [&] {
		PerObjectData* data;
		ASSERT_HRESULT_SUCCEEDED( cb.Lock( (void**)&data, *renderContext ) );
		data->x = float( g % 100 ) / 100.0f - 0.5f;
		data->y = 0;
		ASSERT_HRESULT_SUCCEEDED( cb.Unlock( *renderContext ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetDepthStencil( depthBuffer ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET | Tr2RenderContextEnum::CLEARFLAGS_ZBUFFER, 0xff00ff00 | ( g & 0xff ) << 16, 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( cb, Tr2RenderContextEnum::VERTEX_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZWRITEENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZFUNC, Tr2RenderContextEnum::CMP_LESSEQUAL ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );

		ASSERT_HRESULT_SUCCEEDED( cb.Lock( (void**)&data, *renderContext ) );
		data->x = 0;
		data->y = 0;
		ASSERT_HRESULT_SUCCEEDED( cb.Unlock( *renderContext ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( cb, Tr2RenderContextEnum::VERTEX_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vbBg, 0, vbStride ) );
		
		if( issueQueries )
		{
			ASSERT_HRESULT_SUCCEEDED( queryOccluded.Begin( *renderContext ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
			ASSERT_HRESULT_SUCCEEDED( queryOccluded.End( *renderContext ) );

			ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_COLORWRITEENABLE, 0 ) );
			ASSERT_HRESULT_SUCCEEDED( queryTotal.Begin( *renderContext ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
			ASSERT_HRESULT_SUCCEEDED( queryTotal.End( *renderContext ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 1 ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_COLORWRITEENABLE, 0xf ) );
		}
		else
		{
			ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		}
		ALResult resultOccluded = queryOccluded.GetPixelCount( *renderContext, occludedPixelCount );
		ASSERT_HRESULT_SUCCEEDED( resultOccluded );
		ALResult resultTotal = queryTotal.GetPixelCount( *renderContext, totalPixelCount );
		ASSERT_HRESULT_SUCCEEDED( resultTotal );
		issueQueries = resultOccluded == S_OK && resultTotal == S_OK;

		ASSERT_HRESULT_SUCCEEDED( cb.Lock( (void**)&data, *renderContext ) );
		data->x = 0.5f;
		data->y = totalPixelCount ? float( occludedPixelCount ) / float( totalPixelCount ) : 0;
		ASSERT_HRESULT_SUCCEEDED( cb.Unlock( *renderContext ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( cb, Tr2RenderContextEnum::VERTEX_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PopDepthStencil() );

		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( Tr2ConstantBufferAL(), Tr2RenderContextEnum::VERTEX_SHADER, 0 ) );
}

TEST_F( Rendering, CanUseViewport )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreatePositionOnlyVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t g = 127;

	Tr2Viewport viewport( 250, 250 );

	uint32_t renderStates[] = {
		Tr2RenderContextEnum::RS_ZENABLE, 0,
		Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE,
	};

	Tr2Viewport bkViewport;
	ASSERT_HRESULT_SUCCEEDED( renderContext->GetViewport( bkViewport ) );

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff00ff00 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderStates( renderStates, 2 ) );
		viewport.m_x = float( g );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetViewport( viewport ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitiveUP( 1, vertices, sizeof( float ) * 3 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetViewport( bkViewport ) );
}

TEST_F( Rendering, CanPerformAlphaTestGreaterEqual )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	uint32_t vsBytecodePatched[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs.patched )
	};

	auto vsInput = Tr2ShaderSignatureAL()
		.Add( Tr2VertexDefinition::TEXCOORD, 0, 0 )
		.Add( Tr2VertexDefinition::POSITION, 0, 1 );

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		VERTEX_SHADER,
		vsBytecode,
		vsBytecodePatched,
		vsInput,
		*renderContext ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	uint32_t psBytecodePatched[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps.patched )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		PIXEL_SHADER,
		psBytecode,
		psBytecodePatched,
		Tr2ShaderSignatureAL().Add( Tr2ShaderRegisterAL::RESOURCE, 0 ).Add( Tr2ShaderRegisterAL::SAMPLER, 0 ),
		*renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t texturePixels0[] = {
		0xff00ff00, 0xff0000ff, 0xff00ff00, 0xff0000ff,
		0xffff0000, 0x00ffffff, 0xffff0000, 0x00ffffff,
		0xff00ff00, 0xff0000ff, 0xff00ff00, 0xff0000ff,
		0xffff0000, 0x00ffffff, 0xffff0000, 0x00ffffff,
	};
	Tr2SubresourceData textureData[1];
	textureData[0].m_sysMem = texturePixels0;
	textureData[0].m_sysMemPitch = 16;
	textureData[0].m_sysMemSlicePitch = sizeof( texturePixels0 );

	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 4, 4, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, textureData, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );

	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex );
	desc.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHATESTENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHAFUNC, Tr2RenderContextEnum::CMP_GREATEREQUAL ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHAREF, 127 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHATESTENABLE, 0 ) );
}

TEST_F( Rendering, CanPerformAlphaTestLessEqual )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	uint32_t vsBytecodePatched[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs.patched )
	};

	auto vsInput = Tr2ShaderSignatureAL()
		.Add( Tr2VertexDefinition::TEXCOORD, 0, 0 )
		.Add( Tr2VertexDefinition::POSITION, 0, 1 );

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create(
		VERTEX_SHADER,
		vsBytecode,
		vsBytecodePatched,
		vsInput,
		*renderContext ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	uint32_t psBytecodePatched[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps.patched )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		PIXEL_SHADER,
		psBytecode,
		psBytecodePatched,
		Tr2ShaderSignatureAL().Add( Tr2ShaderRegisterAL::RESOURCE, 0 ).Add( Tr2ShaderRegisterAL::SAMPLER, 0 ),
		*renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t texturePixels0[] = {
		0xff00ff00, 0xff0000ff, 0xff00ff00, 0xff0000ff,
		0xffff0000, 0x00ffffff, 0xffff0000, 0x00ffffff,
		0xff00ff00, 0xff0000ff, 0xff00ff00, 0xff0000ff,
		0xffff0000, 0x00ffffff, 0xffff0000, 0x00ffffff,
	};
	Tr2SubresourceData textureData[1];
	textureData[0].m_sysMem = texturePixels0;
	textureData[0].m_sysMemPitch = 16;
	textureData[0].m_sysMemSlicePitch = sizeof( texturePixels0 );

	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 4, 4, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, textureData, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );

	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex );
	desc.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHATESTENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHAFUNC, Tr2RenderContextEnum::CMP_LESSEQUAL ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHAREF, 127 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHATESTENABLE, 0 ) );
}

TEST_F( Rendering, CanPerformAlphaTestEqual )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	uint32_t vsBytecodePatched[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs.patched )
	};

	auto vsInput = Tr2ShaderSignatureAL()
		.Add( Tr2VertexDefinition::TEXCOORD, 0, 0 )
		.Add( Tr2VertexDefinition::POSITION, 0, 1 );

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create(
		VERTEX_SHADER,
		vsBytecode,
		vsBytecodePatched,
		vsInput,
		*renderContext ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	uint32_t psBytecodePatched[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps.patched )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create(
		PIXEL_SHADER,
		psBytecode,
		psBytecodePatched,
		Tr2ShaderSignatureAL().Add( Tr2ShaderRegisterAL::RESOURCE, 0 ).Add( Tr2ShaderRegisterAL::SAMPLER, 0 ),
		*renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t texturePixels0[] = {
		0xff00ff00, 0xff0000ff, 0xff00ff00, 0xff0000ff,
		0xffff0000, 0x00ffffff, 0xffff0000, 0x11ffffff,
		0xff00ff00, 0xff0000ff, 0xff00ff00, 0xff0000ff,
		0xffff0000, 0x11ffffff, 0xffff0000, 0x00ffffff,
	};
	Tr2SubresourceData textureData[1];
	textureData[0].m_sysMem = texturePixels0;
	textureData[0].m_sysMemPitch = 16;
	textureData[0].m_sysMemSlicePitch = sizeof( texturePixels0 );

	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 4, 4, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, textureData, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );


	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex );
	desc.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHATESTENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHAFUNC, Tr2RenderContextEnum::CMP_EQUAL ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHAREF, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHATESTENABLE, 0 ) );
}

TEST_F( Rendering, CanPerformAlphaBlend )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleTextureFromTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	const uint32_t textureSize = 128;
	uint32_t texturePixels0[textureSize * textureSize];

	auto toBytes = []( float x ) { return 0xff & uint32_t( x * 255.f ); };
	for( uint32_t j = 0; j < textureSize; ++j )
	{
		float y = float( j ) / textureSize;
		for( uint32_t i = 0; i < textureSize; ++i )
		{
			float x = float( i ) / textureSize;
			texturePixels0[i + j * textureSize] = ( toBytes( sqrt( 1 - x * x - y * y ) ) << 24 ) |
				( toBytes( 1 - x ) << 16 ) |
				( toBytes( 1 - y ) << 8 );
		}
	}
	Tr2SubresourceData textureData[1];
	textureData[0].m_sysMem = texturePixels0;
	textureData[0].m_sysMemPitch = textureSize * 4;
	textureData[0].m_sysMemSlicePitch = sizeof( texturePixels0 );

	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( textureSize, textureSize, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, textureData, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_LINEAR,
			Tr2RenderContextEnum::TF_LINEAR,
			Tr2RenderContextEnum::TF_POINT,
			false,
			Tr2RenderContextEnum::TA_CLAMP,
			Tr2RenderContextEnum::TA_CLAMP,
			Tr2RenderContextEnum::TA_WRAP,
			0.0f,
			1,
			Tr2RenderContextEnum::CMP_ALWAYS,
			border,
			0.0f,
			0.0f ),
		*renderContext ) );


	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex );
	desc.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_SRCBLEND, Tr2RenderContextEnum::BM_SRCALPHA ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_DESTBLEND, Tr2RenderContextEnum::BM_INVSRCALPHA ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
}

TEST_F( Rendering, CanGenerateRenderTargetMips )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleTextureMipFromTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	Tr2TextureAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( Tr2BitmapDimensions( 128, 64, 0, Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_FLOAT ), Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE, *renderContext ) );

	Tr2ShaderAL psFill;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( psFill, *renderContext ) );

	Tr2ShaderAL shadersFill[] = { vs, psFill };
	Tr2ShaderProgramAL spFill;
	ASSERT_HRESULT_SUCCEEDED( spFill.Create( shadersFill, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 

		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2BufferAL quads[8];
	float quadVertices[] = {
		-0.85f, -0.1f, 0.0f, 0.0f, 1.0f, 
		-0.85f, 0.1f, 0.0f, 0.0f, 0.0f, 
		-0.65f, -0.1f, 0.0f, 1.0f, 1.0f, 
		-0.65f, 0.1f, 0.0f, 1.0f, 0.0f, };

	for( uint32_t i = 0; i < 8; ++i )
	{
		ASSERT_HRESULT_SUCCEEDED( quads[i].Create( vbStride, sizeof( quadVertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, quadVertices, *renderContext ) );
		for( uint32_t j = 0; j < 4; ++j )
		{
			quadVertices[j * 5] += 0.21f;
		}
	}

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2ConstantBufferAL cb;
	ASSERT_HRESULT_SUCCEEDED( cb.Create( sizeof( PerObjectData ), *renderContext ) );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampler;
	Tr2SamplerDescription samplerDesc( 
		Tr2RenderContextEnum::TF_POINT,
		Tr2RenderContextEnum::TF_POINT,
		Tr2RenderContextEnum::TF_POINT,
		false,
		Tr2RenderContextEnum::TA_CLAMP,
		Tr2RenderContextEnum::TA_CLAMP,
		Tr2RenderContextEnum::TA_WRAP,
		0.0f,
		1,
		Tr2RenderContextEnum::CMP_ALWAYS,
		border,
		0.0f,
		std::numeric_limits<float>::max() );
	ASSERT_HRESULT_SUCCEEDED( sampler.Create( samplerDesc, *renderContext ) );


	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, rt );
	desc.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampler );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {

		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushRenderTarget() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( rt ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetDepthStencil( Tr2TextureAL() ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 0 ), 1.0f ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( spFill ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->PopDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PopRenderTarget() );

		ASSERT_HRESULT_SUCCEEDED( rt.GenerateMipMaps( *renderContext ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		for( uint32_t i = 0; i < 8; ++i )
		{
			void* data;
			ASSERT_HRESULT_SUCCEEDED( cb.Lock( &data, *renderContext ) );
			static_cast<PerObjectData*>( data )->x = float( i );
			ASSERT_HRESULT_SUCCEEDED( cb.Unlock( *renderContext ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( cb, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, quads[i], 0, vbStride ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		}
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

TEST_F( Rendering, CanCopyRenderTargetRegion )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleTextureFromTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	Tr2ShaderAL psFill;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( psFill, *renderContext ) );

	Tr2ShaderAL shadersFill[] = { vs, psFill };
	Tr2ShaderProgramAL spFill;
	ASSERT_HRESULT_SUCCEEDED( spFill.Create( shadersFill, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 

		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );

	Tr2TextureAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( Tr2BitmapDimensions( 128, 64, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::RENDER_TARGET, *renderContext ) );

	Tr2TextureAL rt2;
	ASSERT_HRESULT_SUCCEEDED( rt2.Create( Tr2BitmapDimensions( 256, 256, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE, *renderContext ) );


	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, rt2 );
	desc.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {

		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 16 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushRenderTarget() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( rt ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetDepthStencil( Tr2TextureAL() ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 0 ), 1.0f ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( spFill ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( rt2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->PopDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PopRenderTarget() );

		Tr2TextureSubresource src( 0 );
		src.SetRect( 20, 15, 64, 64 );
		Tr2TextureSubresource dst( 0 );
		dst.SetRect( 64, 32, 64 + src.m_right - src.m_left, 32 + src.m_bottom - src.m_top );
		ASSERT_HRESULT_SUCCEEDED( rt2.CopySubresourceRegion( dst, rt, src, *renderContext ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}


TEST_F( Rendering, CanSampleBc1Texture )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleTextureFromTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	const uint32_t width = 128;
	const uint32_t height = 128;

	uint32_t texturePixels0[] = {
#include "Dxt1Image.h"
	};
	Tr2SubresourceData textureData[1];
	textureData[0].m_sysMem = texturePixels0;
	textureData[0].m_sysMemPitch = sizeof( texturePixels0 ) / height * 4; // *4 because it's a compressed format
	textureData[0].m_sysMemSlicePitch = sizeof( texturePixels0 );

	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( width, height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_BC1_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, textureData, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );


	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex );
	desc.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
}

TEST_F( Rendering, CanPassDynamicConstantBufferToRendering )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreatePositionOnlyWithPerObjectDataVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};
	const uint32_t vbStride = 3 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	Tr2ConstantBufferAL cb;
	ASSERT_HRESULT_SUCCEEDED( cb.Create( sizeof( PerObjectData ), Tr2ConstantUsageAL::ONE_SHOT, nullptr, *renderContext ) );


	uint32_t g = 127;

	auto frame = [&] {
		PerObjectData* data;
		ASSERT_HRESULT_SUCCEEDED( cb.Lock( (void**)&data, *renderContext ) );
		data->x = float( g % 100 ) / 100.0f - 0.5f;
		data->y = 0;
		ASSERT_HRESULT_SUCCEEDED( cb.Unlock( *renderContext ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff00ff00 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( cb, Tr2RenderContextEnum::VERTEX_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( Tr2ConstantBufferAL(), Tr2RenderContextEnum::VERTEX_SHADER, 0 ) );
}

TEST_F( Rendering, CanSampleBc2Texture )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleTextureFromTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	const uint32_t width = 128;
	const uint32_t height = 128;

	uint32_t texturePixels0[] = {
#include "Dxt3Image.h"
	};
	Tr2SubresourceData textureData[1];
	textureData[0].m_sysMem = texturePixels0;
	textureData[0].m_sysMemPitch = sizeof( texturePixels0 ) / height * 4; // *4 because it's a compressed format
	textureData[0].m_sysMemSlicePitch = sizeof( texturePixels0 );

	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( width, height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_BC2_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, textureData, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );


	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex );
	desc.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
}


TEST_F( Rendering, CanSampleBc3Texture )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleTextureFromTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	const uint32_t width = 128;
	const uint32_t height = 128;

	uint32_t texturePixels0[] = {
#include "Dxt5Image.h"
	};
	Tr2SubresourceData textureData[1];
	textureData[0].m_sysMem = texturePixels0;
	textureData[0].m_sysMemPitch = sizeof( texturePixels0 ) / height * 4; // *4 because it's a compressed format
	textureData[0].m_sysMemSlicePitch = sizeof( texturePixels0 );

	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( width, height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_BC3_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, textureData, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );


	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex );
	desc.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
}


TEST_F( Rendering, CanSampleVolumeTexture )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleVolumeTexturePS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	const uint32_t width = 32;
	const uint32_t height = 32;
	const uint32_t depth = 32;

	static uint32_t texturePixels0[] = {
#include "XrgbVolume.h"
	};
	Tr2SubresourceData textureData[1];
	textureData[0].m_sysMem = texturePixels0;
	textureData[0].m_sysMemPitch = sizeof( texturePixels0 ) / depth / height;
	textureData[0].m_sysMemSlicePitch = sizeof( texturePixels0 ) / depth;

	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( Tr2RenderContextEnum::TEX_TYPE_3D, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM, width, height, depth, 1 ), Tr2GpuUsage::SHADER_RESOURCE, textureData, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_LINEAR,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );


	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex );
	desc.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );

	uint32_t g = 127;


	Tr2ConstantBufferAL cb;
	ASSERT_HRESULT_SUCCEEDED( cb.Create( sizeof( PerObjectData ), *renderContext ) );

	auto frame = [&] {
		PerObjectData* data;
		ASSERT_HRESULT_SUCCEEDED( cb.Lock( (void**)&data, *renderContext ) );
		data->x = float( g % 100 ) / 100.0f - 0.5f;
		data->y = 0;
		ASSERT_HRESULT_SUCCEEDED( cb.Unlock( *renderContext ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( cb, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
}


TEST_F( Rendering, CanSampleBc3VolumeTexture )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleVolumeTexturePS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	const uint32_t width = 32;
	const uint32_t height = 32;
	const uint32_t depth = 32;

	uint32_t texturePixels0[] = {
#include "Dxt5Volume.h"
	};
	Tr2SubresourceData textureData[1];
	textureData[0].m_sysMem = texturePixels0;
	textureData[0].m_sysMemPitch = sizeof( texturePixels0 ) / depth / height * 4; // *4 because it's a compressed format
	textureData[0].m_sysMemSlicePitch = sizeof( texturePixels0 ) / depth;

	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( Tr2RenderContextEnum::TEX_TYPE_3D, Tr2RenderContextEnum::PIXEL_FORMAT_BC3_UNORM, width, height, depth, 1 ), Tr2GpuUsage::SHADER_RESOURCE, textureData, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_LINEAR,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );


	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex );
	desc.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );

	uint32_t g = 127;


	Tr2ConstantBufferAL cb;
	ASSERT_HRESULT_SUCCEEDED( cb.Create( sizeof( PerObjectData ), *renderContext ) );

	auto frame = [&] {
		PerObjectData* data;
		ASSERT_HRESULT_SUCCEEDED( cb.Lock( (void**)&data, *renderContext ) );
		data->x = float( g % 100 ) / 100.0f - 0.5f;
		data->y = 0;
		ASSERT_HRESULT_SUCCEEDED( cb.Unlock( *renderContext ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( cb, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
}


TEST_F( Rendering, CanSampleUnassignedTexture )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleTextureFromTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
}

TEST_F( Rendering, CanLockTextureTwice )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleTextureFromTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices1[] = {
		-0.75f, -0.25f, 0.0f, 1.0f, 1.0f, 
		-0.75f, 0.25f, 0.0f, 1.0f, 0.0f, 
		-0.25f, -0.25f, 0.0f, 0.0f, 1.0f, 
		-0.25f, 0.25f, 0.0f, 0.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb1;
	ASSERT_HRESULT_SUCCEEDED( vb1.Create( vbStride, sizeof( vertices1 ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices1, *renderContext ) );


	float vertices2[] = {
		0.25f, -0.25f, 0.0f, 1.0f, 1.0f, 
		0.25f, 0.25f, 0.0f, 1.0f, 0.0f, 
		0.75f, -0.25f, 0.0f, 0.0f, 1.0f, 
		0.75f, 0.25f, 0.0f, 0.0f, 0.0f, 
	};
	Tr2BufferAL vb2;
	ASSERT_HRESULT_SUCCEEDED( vb2.Create( vbStride, sizeof( vertices2 ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices2, *renderContext ) );



	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t texturePixels0[] = {
		0xff00ff00, 
	};
	Tr2SubresourceData textureData[1];
	textureData[0].m_sysMem = texturePixels0;
	textureData[0].m_sysMemPitch = 4;
	textureData[0].m_sysMemSlicePitch = sizeof( texturePixels0 );

	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 1, 1, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, textureData, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );


	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex );
	desc.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );

		void* data;
		uint32_t pitch;
		ASSERT_HRESULT_SUCCEEDED( tex.MapForWriting( Tr2TextureSubresource( 0 ), data, pitch, *renderContext ) );
		*reinterpret_cast<uint32_t*>( data ) = 0xffff0000;
		tex.UnmapForWriting( *renderContext );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb1, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );

		ASSERT_HRESULT_SUCCEEDED( tex.MapForWriting( Tr2TextureSubresource( 0 ), data, pitch, *renderContext ) );
		*reinterpret_cast<uint32_t*>( data ) = 0xff00ff00;
		tex.UnmapForWriting( *renderContext );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb2, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );


		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

TEST_F( Rendering, CanSampleSrgbTexture )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleTextureFromTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t texturePixels0[] = {
		0xffffffff, 0xff7f7f7f, 0xff7f7f7f, 0xffffffff,
		0xff000000, 0xff7f7f7f, 0xff7f7f7f, 0xff000000,
		0xffffffff, 0xff7f7f7f, 0xff7f7f7f, 0xffffffff,
		0xff000000, 0xff7f7f7f, 0xff7f7f7f, 0xff000000,
	};
	Tr2SubresourceData textureData[1];
	textureData[0].m_sysMem = texturePixels0;
	textureData[0].m_sysMemPitch = 16;
	textureData[0].m_sysMemSlicePitch = sizeof( texturePixels0 );

	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 4, 4, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, textureData, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );


	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex, Tr2RenderContextEnum::COLOR_SPACE_SRGB );
	desc.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

TEST_F( Rendering, CanOutputToSrgbTarget )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleTextureFromTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t texturePixels0[] = {
		0xffffffff, 0xff7f7f7f, 0xff7f7f7f, 0xffffffff,
		0xff000000, 0xff7f7f7f, 0xff7f7f7f, 0xff000000,
		0xffffffff, 0xff7f7f7f, 0xff7f7f7f, 0xffffffff,
		0xff000000, 0xff7f7f7f, 0xff7f7f7f, 0xff000000,
	};
	Tr2SubresourceData textureData[1];
	textureData[0].m_sysMem = texturePixels0;
	textureData[0].m_sysMemPitch = 16;
	textureData[0].m_sysMemSlicePitch = sizeof( texturePixels0 );

	Tr2TextureAL tex;
	ASSERT_HRESULT_SUCCEEDED( tex.Create( Tr2BitmapDimensions( 4, 4, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, textureData, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );


	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex, Tr2RenderContextEnum::COLOR_SPACE_SRGB );
	desc.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_SRGBWRITEENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_SRGBWRITEENABLE, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

#if TRINITY_PLATFORM_SUPPORTS_UNORDERED_ACCESS

TEST_F( Rendering, CanUsePsUavs )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateWriteToUavPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );


	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	Tr2TextureAL rwTexture;
	ASSERT_HRESULT_SUCCEEDED( rwTexture.Create( Tr2BitmapDimensions( 64, 64, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::UNORDERED_ACCESS, *renderContext ) );

	Tr2ResourceSetAL uavResourceSet;
	{
		Tr2ResourceSetDescriptionAL resourceSetDescription( sp );
		resourceSetDescription.SetUav( Tr2RenderContextEnum::PIXEL_SHADER, 1, rwTexture );
		ASSERT_HRESULT_SUCCEEDED( uavResourceSet.Create( resourceSetDescription, sp, *renderContext ) );
	}


	float quad[] = {
		0, 0, 0, 0, 1,
		0, 1, 0, 0, 0,
		1, 0, 0, 1, 1,

		1, 0, 0, 1, 1,
		0, 1, 0, 0, 0,
		1, 1, 0, 1, 0,
	};
	Tr2BufferAL quadVb;
	ASSERT_HRESULT_SUCCEEDED( quadVb.Create( vbStride, sizeof( quad ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, quad, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create(
		Tr2SamplerDescription(
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );


	Tr2ShaderAL ps2;
	ASSERT_HRESULT_SUCCEEDED( CreateSampleTextureFromTexCoordPS( ps2, *renderContext ) );

	Tr2ShaderAL shaders2[] = { vs, ps2 };
	Tr2ShaderProgramAL sp2;
	ASSERT_HRESULT_SUCCEEDED( sp2.Create( shaders2, 2, *renderContext ) );


	Tr2ResourceSetDescriptionAL resourceSetDescription( sp2 );
	resourceSetDescription.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, rwTexture );
	resourceSetDescription.SetSampler( Tr2RenderContextEnum::PIXEL_SHADER, 0, sampl );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( resourceSetDescription, sp2, *renderContext ) );


	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );

		float clearColor[] = { 0, float( g & 0xff ) / 255.f, 0, 0 };
		ASSERT_HRESULT_SUCCEEDED( renderContext->ClearUav( rwTexture, 0, clearColor ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( uavResourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, quadVb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );

}


TEST_F( Rendering, CanDrawIndirect )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreatePositionOnlyVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};
	const uint32_t vbStride = 3 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	struct 
	{
		uint32_t vertexCountPerInstance;
		uint32_t instanceCount;
		uint32_t startVertexLocation;
		uint32_t startInstanceLocation;
	} indirectArgs = { 3, 1, 0, 0 };

	Tr2BufferAL indirectArgsBuffer;
	ASSERT_HRESULT_SUCCEEDED( indirectArgsBuffer.Create( Tr2BufferDescriptionAL( PIXEL_FORMAT_R32_UINT, sizeof( indirectArgs ) / 4, Tr2GpuUsage::UNORDERED_ACCESS | Tr2GpuUsage::DRAW_INDIRECT_ARGS, Tr2CpuUsage::NONE ), &indirectArgs, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawInstancedIndirect( indirectArgsBuffer, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

#endif


TEST_F( Rendering, CanWriteToVB )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreatePositionOnlyVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};
	const uint32_t vbStride = 3 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::WRITE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );

		void* vbData;
		ASSERT_HRESULT_SUCCEEDED( vb.MapForWriting( vbData, *renderContext ) );
		vertices[3] = -0.5f + float( g & 0xff ) / 255;
		memcpy( vbData, vertices, sizeof( vertices ) );
		vb.UnmapForWriting( *renderContext );

		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

TEST_F( Rendering, CanWriteToDynamicVB )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreatePositionOnlyVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};
	const uint32_t vbStride = 3 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::WRITE_OFTEN, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );

		void* vbData;
		ASSERT_HRESULT_SUCCEEDED( vb.MapForWriting( vbData, *renderContext ) );
		vertices[3] = -0.5f + float( g & 0xff ) / 255;
		memcpy( vbData, vertices, sizeof( vertices ) );
		vb.UnmapForWriting( *renderContext );

		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

TEST_F( Rendering, CanWriteToDynamicVBWithoutSynchronization )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreatePositionOnlyVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};

	const uint32_t vbStride = 3 * sizeof( float );
	Tr2BufferAL vb;

	uint32_t capacity = 30;

	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride * capacity, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::WRITE_OFTEN, nullptr, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t g = 127;

	uint32_t offset = 0;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );

		offset += uint32_t( sizeof( vertices ) );
		if( offset >= vb.GetSize() )
		{
			offset = 0;
		}

		void* vbData;
		if( g % 5 == 0 )
		{
			// just to make it harder
			ASSERT_HRESULT_SUCCEEDED( vb.MapForWriting( vbData, Tr2LockType::SYNCHRONIZED, *renderContext ) );
		}
		else
		{
			ASSERT_HRESULT_SUCCEEDED( vb.MapForWriting( vbData, Tr2LockType::NON_SYNCHRONIZED, *renderContext ) );
		}
		vertices[3] = -0.5f + float( g & 0xff ) / 255;
		memcpy( static_cast<uint8_t*>( vbData ) + offset, vertices, sizeof( vertices ) );
		vb.UnmapForWriting( *renderContext );

		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, offset, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}


TEST_F( Rendering, CanHaveMissingIAElements )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateOutputTexCoordPS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};
	const uint32_t vbStride = 3 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

#if TRINITY_PLATFORM_SUPPORTS_MSAA_SAMPLE

TEST_F( Rendering, CanLoadMsaaRenderTarget )
{
	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( CreateTexCoordAndPositionVS( vs, *renderContext ) );

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( CreateLoadMsaaTexturePS( ps, *renderContext ) );

	Tr2ShaderAL shaders[] = { vs, ps };
	Tr2ShaderProgramAL sp;
	ASSERT_HRESULT_SUCCEEDED( sp.Create( shaders, 2, *renderContext ) );

	Tr2ShaderAL psFill;
	ASSERT_HRESULT_SUCCEEDED( CreateConstantColorPS( psFill, *renderContext ) );

	Tr2ShaderAL shadersFill[] = { vs, psFill };
	Tr2ShaderProgramAL spFill;
	ASSERT_HRESULT_SUCCEEDED( spFill.Create( shadersFill, 2, *renderContext ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f,

		0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2BufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( vbStride, sizeof( vertices ) / vbStride, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create(
		Tr2SamplerDescription(
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TA_WRAP,
			1,
			0.0f,
			0.0f ),
		*renderContext ) );

	Tr2TextureAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( Tr2BitmapDimensions( 128, 64, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2MsaaDesc( 4 ), Tr2GpuUsage::RENDER_TARGET | Tr2GpuUsage::SHADER_RESOURCE, *renderContext ) );


	Tr2ResourceSetDescriptionAL desc( sp );
	desc.SetSrv( Tr2RenderContextEnum::PIXEL_SHADER, 0, rt );

	Tr2ResourceSetAL resourceSet;
	ASSERT_HRESULT_SUCCEEDED( resourceSet.Create( desc, sp, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {

		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushRenderTarget() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( rt ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetDepthStencil( Tr2TextureAL() ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 0 ), 1.0f ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( spFill ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->PopDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PopRenderTarget() );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetResourceSet( resourceSet ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
}

#endif