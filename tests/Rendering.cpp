#include "StdAfx.h"
#include "WithValidRenderContextFixture.h"

struct Rendering: public WithValidRenderContext {};

namespace
{

struct PerObjectData
{
	float x;
	float y;
	float padding[2];
};

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

TEST_F( Rendering, CanBeginAndEndScene )
{
	ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
	ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
}

TEST_F( Rendering, CanRenderASingleTriangle )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 1 );
	vsInput.elements[0].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};
	const uint32_t vbStride = 3 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

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
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
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

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
}

TEST_F( Rendering, CanRenderTriangleStrip )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 1 );
	vsInput.elements[0].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.5f, 0.5f, 0.0f,
	};
	const uint32_t vbStride = 3 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

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
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
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

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
}

TEST_F( Rendering, CanRenderIndexedTriangles )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 1 );
	vsInput.elements[0].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.5f, 0.5f, 0.0f,
	};
	const uint32_t vbStride = 3 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

	uint16_t indices[] = { 0, 1, 2, 1, 2, 3 };
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( sizeof( indices ) / sizeof( indices[0] ), 0, Tr2RenderContextEnum::IB_16BIT, indices, *renderContext ) );

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
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
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

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetIndices( nullIB ) );
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

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( OutputTexCoord.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

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
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
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

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
}

TEST_F( Rendering, CanSampleTexture )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

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
	ASSERT_HRESULT_SUCCEEDED( tex.Create2D( 4, 4, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, 0, textureData, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		*renderContext,
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			false,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			0.0f,
			1,
			Tr2RenderContextEnum::CMP_ALWAYS,
			border,
			0.0f,
			0.0f ) ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampl, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
}


TEST_F( Rendering, CanSampleMipMappedTexture )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

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
	ASSERT_HRESULT_SUCCEEDED( tex.Create2D( 4, 4, 3, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, 0, textureData, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {

		float border[4] = { 0 };
		Tr2SamplerStateAL sampl;
		ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
			*renderContext,
			Tr2SamplerDescription( 
				Tr2RenderContextEnum::TF_LINEAR,
				Tr2RenderContextEnum::TF_LINEAR,
				Tr2RenderContextEnum::TF_LINEAR,
				false,
				Tr2RenderContextEnum::TA_WRAP,
				Tr2RenderContextEnum::TA_WRAP,
				Tr2RenderContextEnum::TA_WRAP,
				0.0f,
				1,
				Tr2RenderContextEnum::CMP_ALWAYS,
				border,
				float( ( g & 0xff ) / 128 ),
				std::numeric_limits<float>::max() ) ) );


		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampl, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
}

TEST_F( Rendering, CanPassConstantBufferToRendering )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnlyWithPerObjectData.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 1 );
	vsInput.elements[0].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};
	const uint32_t vbStride = 3 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	Tr2ConstantBufferAL cb;
	ASSERT_HRESULT_SUCCEEDED( cb.Create( sizeof( PerObjectData ), Tr2RenderContextEnum::USAGE_CPU_WRITE, nullptr, *renderContext ) );


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
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
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

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( const_cast<Tr2ConstantBufferAL&>( nullCB ), Tr2RenderContextEnum::VERTEX_SHADER, 0 ) );
}

TEST_F( Rendering, CanDoInstancedRendering )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( InstancedRendering.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 8;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.25f, -0.25f, 0.0f,
		-0.25f, 0.25f, 0.0f,
		0.25f, -0.25f, 0.0f,
		0.25f, 0.25f, 0.0f,
	};
	const uint32_t vbStride = 3 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

	uint16_t indices[] = { 0, 1, 2, 1, 2, 3 };
	Tr2IndexBufferAL ib;
	ASSERT_HRESULT_SUCCEEDED( ib.Create( sizeof( indices ) / sizeof( indices[0] ), 0, Tr2RenderContextEnum::IB_16BIT, indices, *renderContext ) );

	float instances[] = {
		-0.5f, 0.2f,
		0.5f, -0.2f,
	};
	const uint32_t instanceVbStride = 2 * sizeof( float );
	Tr2VertexBufferAL instanceVb;
	ASSERT_HRESULT_SUCCEEDED( instanceVb.Create( sizeof( instances ), Tr2RenderContextEnum::USAGE_IMMUTABLE, instances, *renderContext ) );

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
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
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

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 1, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
}

TEST_F( Rendering, CanClearRenderTarget )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 

		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		*renderContext,
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			false,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			0.0f,
			1,
			Tr2RenderContextEnum::CMP_ALWAYS,
			border,
			0.0f,
			0.0f ) ) );

	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 64, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, Tr2RenderContextEnum::EX_NONE, *renderContext ) );


	uint32_t g = 127;

	auto frame = [&] {

		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushRenderTarget() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( rt ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetDepthStencil( nullDS ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 16 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PopDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PopRenderTarget() );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, rt.GetTexture() ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampl, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
}

TEST_F( Rendering, CanRenderToRenderTarget )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	uint32_t psFillBytecode[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
	};

	Tr2ShaderAL psFill;
	ASSERT_HRESULT_SUCCEEDED( psFill.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psFillBytecode,
		sizeof( psFillBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 

		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		*renderContext,
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			false,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			0.0f,
			1,
			Tr2RenderContextEnum::CMP_ALWAYS,
			border,
			0.0f,
			0.0f ) ) );

	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 64, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, Tr2RenderContextEnum::EX_NONE, *renderContext ) );


	uint32_t g = 127;

	auto frame = [&] {

		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushRenderTarget() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( rt ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetDepthStencil( nullDS ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 0 ), 1.0f ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( psFill ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->PopDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PopRenderTarget() );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, rt.GetTexture() ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampl, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
}

TEST_F( Rendering, CanRenderToMsaaRenderTarget )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	uint32_t psFillBytecode[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
	};

	Tr2ShaderAL psFill;
	ASSERT_HRESULT_SUCCEEDED( psFill.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psFillBytecode,
		sizeof( psFillBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 

		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		*renderContext,
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			false,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			0.0f,
			1,
			Tr2RenderContextEnum::CMP_ALWAYS,
			border,
			0.0f,
			0.0f ) ) );

	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 64, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc( 4 ), 0, Tr2RenderContextEnum::EX_NONE, *renderContext ) );

	Tr2RenderTargetAL readableRt;
	ASSERT_HRESULT_SUCCEEDED( readableRt.Create( 128, 64, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, Tr2RenderContextEnum::EX_NONE, *renderContext ) );


	uint32_t g = 127;

	auto frame = [&] {

		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushRenderTarget() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( rt ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetDepthStencil( nullDS ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 0 ), 1.0f ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( psFill ) );
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
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, readableRt.GetTexture() ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampl, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
}


TEST_F( Rendering, CanClearDepthBuffer )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 1 );
	vsInput.elements[0].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 1.0f,
	};
	const uint32_t vbStride = 3 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		float depth = float( g & 0xff ) / 255.f;
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET | Tr2RenderContextEnum::CLEARFLAGS_ZBUFFER, 0xff000000 | ( g & 0xff ), depth ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZFUNC, Tr2RenderContextEnum::CMP_LESSEQUAL ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
}

TEST_F( Rendering, CanRenderIntoDepthBuffer )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( OutputTexCoord.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
		0.5f, -0.5f, 0.0f, 1.0f, 0.0f,

		0.5f, -0.5f, 0.5f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
		-0.5f, -0.5f, 0.5f, 1.0f, 1.0f,
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET | Tr2RenderContextEnum::CLEARFLAGS_ZBUFFER, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZWRITEENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZFUNC, Tr2RenderContextEnum::CMP_LESSEQUAL ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
}

TEST_F( Rendering, CanRenderASingleTriangleWithDrawPrimitiveUP )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 1 );
	vsInput.elements[0].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

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
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
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

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
}

TEST_F( Rendering, CanRenderIndexedTrianglesWith16BitDrawIndexedPrimitiveUP )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 1 );
	vsInput.elements[0].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

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
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
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

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetIndices( nullIB ) );
}

TEST_F( Rendering, CanRenderIndexedTrianglesWith32BitDrawIndexedPrimitiveUP )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 1 );
	vsInput.elements[0].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

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
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
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

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetIndices( nullIB ) );
}

TEST_F( Rendering, CanUseClippingPlaneAndScissor )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
	};

	uint32_t vsBytecodePatched[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs.patched )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 1 );
	vsInput.elements[0].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		vsBytecodePatched,
		sizeof( vsBytecodePatched ),
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

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

	float clipPlane[] = { 0, 1, 0, 0 };

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff00ff00 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetClipPlane( 0, clipPlane ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CLIPPLANEENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_SCISSORTESTENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetScissorRect( 0, 0, presentParameters.mode.width / 2, presentParameters.mode.height ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitiveUP( 1, vertices, sizeof( float ) * 3 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_SCISSORTESTENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetScissorRect( 0, 0, presentParameters.mode.width, presentParameters.mode.height ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_SCISSORTESTENABLE, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CLIPPLANEENABLE, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
}

TEST_F( Rendering, CanUseOcclusionQueries )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnlyWithPerObjectData.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 1 );
	vsInput.elements[0].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

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

	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

	Tr2VertexBufferAL vbBg;
	ASSERT_HRESULT_SUCCEEDED( vbBg.Create( sizeof( verticesBg ), Tr2RenderContextEnum::USAGE_IMMUTABLE, verticesBg, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	Tr2ConstantBufferAL cb;
	ASSERT_HRESULT_SUCCEEDED( cb.Create( sizeof( PerObjectData ), Tr2RenderContextEnum::USAGE_CPU_WRITE, nullptr, *renderContext ) );

	Tr2OcclusionQueryAL queryOccluded;
	ASSERT_HRESULT_SUCCEEDED( queryOccluded.Create( *renderContext ) );
	Tr2OcclusionQueryAL queryTotal;
	ASSERT_HRESULT_SUCCEEDED( queryTotal.Create( *renderContext ) );

	uint32_t g = 127;
	uint32_t occludedPixelCount = 0;
	uint32_t totalPixelCount = 0;

	bool issueQueries = true;

	auto frame = [&] {
		PerObjectData* data;
		ASSERT_HRESULT_SUCCEEDED( cb.Lock( (void**)&data, *renderContext ) );
		data->x = float( g % 100 ) / 100.0f - 0.5f;
		data->y = 0;
		ASSERT_HRESULT_SUCCEEDED( cb.Unlock( *renderContext ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET | Tr2RenderContextEnum::CLEARFLAGS_ZBUFFER, 0xff00ff00 | ( g & 0xff ) << 16, 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( cb, Tr2RenderContextEnum::VERTEX_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
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

		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( const_cast<Tr2ConstantBufferAL&>( nullCB ), Tr2RenderContextEnum::VERTEX_SHADER, 0 ) );
}

TEST_F( Rendering, CanUseViewport )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnly.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 1 );
	vsInput.elements[0].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

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
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
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

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
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

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		vsBytecodePatched,
		sizeof( vsBytecodePatched ),
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	uint32_t psBytecodePatched[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps.patched )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		psBytecodePatched,
		sizeof( psBytecodePatched ),
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

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
	ASSERT_HRESULT_SUCCEEDED( tex.Create2D( 4, 4, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, 0, textureData, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		*renderContext,
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			false,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			0.0f,
			1,
			Tr2RenderContextEnum::CMP_ALWAYS,
			border,
			0.0f,
			0.0f ) ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHATESTENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHAFUNC, Tr2RenderContextEnum::CMP_GREATEREQUAL ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHAREF, 127 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampl, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
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

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		vsBytecodePatched,
		sizeof( vsBytecodePatched ),
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	uint32_t psBytecodePatched[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps.patched )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		psBytecodePatched,
		sizeof( psBytecodePatched ),
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

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
	ASSERT_HRESULT_SUCCEEDED( tex.Create2D( 4, 4, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, 0, textureData, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		*renderContext,
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			false,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			0.0f,
			1,
			Tr2RenderContextEnum::CMP_ALWAYS,
			border,
			0.0f,
			0.0f ) ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHATESTENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHAFUNC, Tr2RenderContextEnum::CMP_LESSEQUAL ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHAREF, 127 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampl, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
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

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		vsBytecodePatched,
		sizeof( vsBytecodePatched ),
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	uint32_t psBytecodePatched[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps.patched )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		psBytecodePatched,
		sizeof( psBytecodePatched ),
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

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
	ASSERT_HRESULT_SUCCEEDED( tex.Create2D( 4, 4, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, 0, textureData, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		*renderContext,
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			false,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			0.0f,
			1,
			Tr2RenderContextEnum::CMP_ALWAYS,
			border,
			0.0f,
			0.0f ) ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHATESTENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHAFUNC, Tr2RenderContextEnum::CMP_EQUAL ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHAREF, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampl, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHATESTENABLE, 0 ) );
}

TEST_F( Rendering, CanPerformAlphaBlend )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	uint32_t vsBytecodePatched[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs.patched )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		vsBytecodePatched,
		sizeof( vsBytecodePatched ),
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	uint32_t psBytecodePatched[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps.patched )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		psBytecodePatched,
		sizeof( psBytecodePatched ),
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

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
	ASSERT_HRESULT_SUCCEEDED( tex.Create2D( textureSize, textureSize, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, 0, textureData, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		*renderContext,
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
			0.0f ) ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_SRCBLEND, Tr2RenderContextEnum::BM_SRCALPHA ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_DESTBLEND, Tr2RenderContextEnum::BM_INVSRCALPHA ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampl, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
}

TEST_F( Rendering, CanGenerateRenderTargetMips )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureMipFromTexCoord.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	uint32_t psFillBytecode[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
	};

	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 64, 0, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, Tr2RenderContextEnum::EX_NONE, *renderContext ) );

	Tr2ShaderAL psFill;
	ASSERT_HRESULT_SUCCEEDED( psFill.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psFillBytecode,
		sizeof( psFillBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 

		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

	Tr2VertexBufferAL quads[8];
	float quadVertices[] = {
		-0.85f, -0.1f, 0.0f, 0.0f, 1.0f, 
		-0.85f, 0.1f, 0.0f, 0.0f, 0.0f, 
		-0.65f, -0.1f, 0.0f, 1.0f, 1.0f, 
		-0.65f, 0.1f, 0.0f, 1.0f, 0.0f, };

	for( uint32_t i = 0; i < 8; ++i )
	{
		ASSERT_HRESULT_SUCCEEDED( quads[i].Create( sizeof( quadVertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, quadVertices, *renderContext ) );
		for( uint32_t j = 0; j < 4; ++j )
		{
			quadVertices[j * 5] += 0.21f;
		}
	}

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2ConstantBufferAL cb;
	ASSERT_HRESULT_SUCCEEDED( cb.Create( sizeof( PerObjectData ), Tr2RenderContextEnum::USAGE_CPU_WRITE, nullptr, *renderContext ) );

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
	ASSERT_HRESULT_SUCCEEDED( sampler.Create( *renderContext, samplerDesc ) );

	uint32_t g = 127;

	auto frame = [&] {

		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushRenderTarget() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( rt ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetDepthStencil( nullDS ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 0 ), 1.0f ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( psFill ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->PopDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PopRenderTarget() );

		ASSERT_HRESULT_SUCCEEDED( rt.GenerateMipMaps( *renderContext ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, rt.GetTexture() ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampler, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		for( uint32_t i = 0; i < 8; ++i )
		{
			static_cast<PerObjectData*>( cb.GetBufferMirror( *renderContext ) )->x = float( i );
			ASSERT_HRESULT_SUCCEEDED( cb.UpdateFromMirror( *renderContext ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( cb, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, quads[i], 0, vbStride ) );
			ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		}
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
}

TEST_F( Rendering, CanCopyRenderTargetRegion )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	uint32_t psFillBytecode[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
	};

	Tr2ShaderAL psFill;
	ASSERT_HRESULT_SUCCEEDED( psFill.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psFillBytecode,
		sizeof( psFillBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 

		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );
	definition.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::TEXCOORD );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		*renderContext,
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			false,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			0.0f,
			1,
			Tr2RenderContextEnum::CMP_ALWAYS,
			border,
			0.0f,
			0.0f ) ) );

	Tr2RenderTargetAL rt;
	ASSERT_HRESULT_SUCCEEDED( rt.Create( 128, 64, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, Tr2RenderContextEnum::EX_NONE, *renderContext ) );

	Tr2RenderTargetAL rt2;
	ASSERT_HRESULT_SUCCEEDED( rt2.Create( 256, 256, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, Tr2MsaaDesc(), 0, Tr2RenderContextEnum::EX_NONE, *renderContext ) );


	uint32_t g = 127;

	auto frame = [&] {

		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 16 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushRenderTarget() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( rt ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PushDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetDepthStencil( nullDS ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 0 ), 1.0f ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( psFill ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderTarget( rt2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->PopDepthStencil() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->PopRenderTarget() );

		uint32_t ltrb[] = { 20, 15, 64, 64 };
		ASSERT_HRESULT_SUCCEEDED( rt2.CopySubresourceRegion( 64, 32, rt, ltrb, *renderContext ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, rt2.GetTexture() ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampl, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
}


TEST_F( Rendering, CanSampleBc1Texture )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

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
	ASSERT_HRESULT_SUCCEEDED( tex.Create2D( width, height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_BC1_UNORM, 0, textureData, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		*renderContext,
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			false,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			0.0f,
			1,
			Tr2RenderContextEnum::CMP_ALWAYS,
			border,
			0.0f,
			0.0f ) ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampl, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
}

TEST_F( Rendering, CanPassDynamicConstantBufferToRendering )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( PositionOnlyWithPerObjectData.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 1 );
	vsInput.elements[0].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( ConstantColor.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,
		-0.5f, 0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
	};
	const uint32_t vbStride = 3 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

	Tr2VertexDefinition definition;
	definition.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION );

	Tr2VertexLayoutAL vertexLayout;
	ASSERT_HRESULT_SUCCEEDED( vertexLayout.Create( definition, *renderContext ) );

	Tr2ConstantBufferAL cb;
	ASSERT_HRESULT_SUCCEEDED( cb.Create( sizeof( PerObjectData ), Tr2RenderContextEnum::USAGE_CPU_WRITE | Tr2RenderContextEnum::USAGE_LOCK_FREQUENTLY, nullptr, *renderContext ) );


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
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
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

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( const_cast<Tr2ConstantBufferAL&>( nullCB ), Tr2RenderContextEnum::VERTEX_SHADER, 0 ) );
}

TEST_F( Rendering, CanSampleBc2Texture )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

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
	ASSERT_HRESULT_SUCCEEDED( tex.Create2D( width, height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_BC2_UNORM, 0, textureData, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		*renderContext,
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			false,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			0.0f,
			1,
			Tr2RenderContextEnum::CMP_ALWAYS,
			border,
			0.0f,
			0.0f ) ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampl, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
}


TEST_F( Rendering, CanSampleBc3Texture )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

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
	ASSERT_HRESULT_SUCCEEDED( tex.Create2D( width, height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_BC3_UNORM, 0, textureData, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		*renderContext,
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			false,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			0.0f,
			1,
			Tr2RenderContextEnum::CMP_ALWAYS,
			border,
			0.0f,
			0.0f ) ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampl, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
}


TEST_F( Rendering, CanSampleVolumeTexture )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleVolumeTexture.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

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
	ASSERT_HRESULT_SUCCEEDED( tex.CreateVolume( width, height, depth, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM, 0, textureData, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		*renderContext,
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_LINEAR,
			Tr2RenderContextEnum::TF_LINEAR,
			Tr2RenderContextEnum::TF_LINEAR,
			false,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			0.0f,
			1,
			Tr2RenderContextEnum::CMP_ALWAYS,
			border,
			0.0f,
			0.0f ) ) );

	uint32_t g = 127;


	Tr2ConstantBufferAL cb;
	ASSERT_HRESULT_SUCCEEDED( cb.Create( sizeof( PerObjectData ), Tr2RenderContextEnum::USAGE_CPU_WRITE, nullptr, *renderContext ) );

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
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampl, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( cb, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
}


TEST_F( Rendering, CanSampleBc3VolumeTexture )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleVolumeTexture.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

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
	ASSERT_HRESULT_SUCCEEDED( tex.CreateVolume( width, height, depth, 1, Tr2RenderContextEnum::PIXEL_FORMAT_BC3_UNORM, 0, textureData, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		*renderContext,
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_LINEAR,
			Tr2RenderContextEnum::TF_LINEAR,
			Tr2RenderContextEnum::TF_LINEAR,
			false,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			0.0f,
			1,
			Tr2RenderContextEnum::CMP_ALWAYS,
			border,
			0.0f,
			0.0f ) ) );

	uint32_t g = 127;


	Tr2ConstantBufferAL cb;
	ASSERT_HRESULT_SUCCEEDED( cb.Create( sizeof( PerObjectData ), Tr2RenderContextEnum::USAGE_CPU_WRITE, nullptr, *renderContext ) );

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
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampl, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetConstants( cb, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
}


TEST_F( Rendering, CanSampleUnassignedTexture )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

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
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
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

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
}

TEST_F( Rendering, CanLockTextureTwice )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices1[] = {
		-0.75f, -0.25f, 0.0f, 1.0f, 1.0f, 
		-0.75f, 0.25f, 0.0f, 1.0f, 0.0f, 
		-0.25f, -0.25f, 0.0f, 0.0f, 1.0f, 
		-0.25f, 0.25f, 0.0f, 0.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb1;
	ASSERT_HRESULT_SUCCEEDED( vb1.Create( sizeof( vertices1 ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices1, *renderContext ) );


	float vertices2[] = {
		0.25f, -0.25f, 0.0f, 1.0f, 1.0f, 
		0.25f, 0.25f, 0.0f, 1.0f, 0.0f, 
		0.75f, -0.25f, 0.0f, 0.0f, 1.0f, 
		0.75f, 0.25f, 0.0f, 0.0f, 0.0f, 
	};
	Tr2VertexBufferAL vb2;
	ASSERT_HRESULT_SUCCEEDED( vb2.Create( sizeof( vertices2 ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices2, *renderContext ) );



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
	ASSERT_HRESULT_SUCCEEDED( tex.Create2D( 1, 1, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, 0, textureData, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		*renderContext,
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			false,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			0.0f,
			1,
			Tr2RenderContextEnum::CMP_ALWAYS,
			border,
			0.0f,
			0.0f ) ) );

	uint32_t g = 127;

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampl, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );

		void* data;
		uint32_t pitch;
		ASSERT_HRESULT_SUCCEEDED( tex.Lock( 0, data, pitch, Tr2RenderContextEnum::LOCK_WRITEONLY, *renderContext ) );
		*reinterpret_cast<uint32_t*>( data ) = 0xffff0000;
		ASSERT_HRESULT_SUCCEEDED( tex.Unlock( *renderContext ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb1, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );

		ASSERT_HRESULT_SUCCEEDED( tex.Lock( 0, data, pitch, Tr2RenderContextEnum::LOCK_WRITEONLY, *renderContext ) );
		*reinterpret_cast<uint32_t*>( data ) = 0xff00ff00;
		ASSERT_HRESULT_SUCCEEDED( tex.Unlock( *renderContext ) );

		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb2, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );


		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
}

TEST_F( Rendering, CanSampleSrgbTexture )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

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
	ASSERT_HRESULT_SUCCEEDED( tex.Create2D( 4, 4, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, 0, textureData, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		*renderContext,
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			false,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			0.0f,
			1,
			Tr2RenderContextEnum::CMP_ALWAYS,
			border,
			0.0f,
			0.0f ) ) );

	uint32_t g = 127; 

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex, Tr2RenderContextEnum::COLOR_SPACE_SRGB ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampl, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
}

TEST_F( Rendering, CanOutputToSrgbTarget )
{
	uint32_t vsBytecode[] = {
#include INCLUDE_SHADER_CODE( TexCoordAndPosition.vs )
	};

	Tr2ShaderInputDefinition vsInput;
	vsInput.elements.resize( 2 );
	vsInput.elements[0].usage = Tr2VertexDefinition::TEXCOORD;
	vsInput.elements[0].usageIndex = 0;
	vsInput.elements[0].registerIndex = 0;
	vsInput.elements[1].usage = Tr2VertexDefinition::POSITION;
	vsInput.elements[1].usageIndex = 0;
	vsInput.elements[1].registerIndex = 1;
	vsInput.ComputeHash();

	Tr2ShaderAL vs;
	ASSERT_HRESULT_SUCCEEDED( vs.Create( 
		*renderContext, 
		Tr2RenderContextEnum::VERTEX_SHADER,
		vsBytecode,
		sizeof( vsBytecode ),
		nullptr,
		0,
		vsInput ) );

	uint32_t psBytecode[] = {
#include INCLUDE_SHADER_CODE( SampleTextureFromTexCoord.ps )
	};

	Tr2ShaderAL ps;
	ASSERT_HRESULT_SUCCEEDED( ps.Create( 
		*renderContext,  
		Tr2RenderContextEnum::PIXEL_SHADER,
		psBytecode,
		sizeof( psBytecode ),
		nullptr,
		0,
		Tr2ShaderInputDefinition() ) );

	float vertices[] = {
		-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 
		-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 
		0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 
		0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 
	};
	const uint32_t vbStride = 5 * sizeof( float );
	Tr2VertexBufferAL vb;
	ASSERT_HRESULT_SUCCEEDED( vb.Create( sizeof( vertices ), Tr2RenderContextEnum::USAGE_IMMUTABLE, vertices, *renderContext ) );

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
	ASSERT_HRESULT_SUCCEEDED( tex.Create2D( 4, 4, 1, Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM, 0, textureData, *renderContext ) );

	float border[4] = { 0 };
	Tr2SamplerStateAL sampl;
	ASSERT_HRESULT_SUCCEEDED( sampl.Create( 
		*renderContext,
		Tr2SamplerDescription( 
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			Tr2RenderContextEnum::TF_POINT,
			false,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			Tr2RenderContextEnum::TA_WRAP,
			0.0f,
			1,
			Tr2RenderContextEnum::CMP_ALWAYS,
			border,
			0.0f,
			0.0f ) ) );

	uint32_t g = 127; 

	auto frame = [&] {
		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0xff000000 | ( ( g & 0xff ) << 8 ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( vs ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( ps ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( Tr2RenderContextEnum::TOP_TRIANGLE_STRIP ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_SRGBWRITEENABLE, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_ALPHABLENDENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_CULLMODE, Tr2RenderContextEnum::CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, tex, Tr2RenderContextEnum::COLOR_SPACE_SRGB ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetSamplerState( sampl, Tr2RenderContextEnum::PIXEL_SHADER, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 2 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		MakeTestScreenShot();
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( Tr2RenderContextEnum::RS_SRGBWRITEENABLE, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, nullVB, 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::VERTEX_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShader( nullShader[Tr2RenderContextEnum::PIXEL_SHADER] ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX ) );
}
