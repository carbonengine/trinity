#include "StdAfx.h"
#include "RenderWindow.h"
#include "WithValidRenderContextFixture.h"


struct SwapChainResizing : public WithValidRenderContext 
{
	static ALResult ResizeWindow( uint32_t width, uint32_t height, bool windowed = true )
	{
		auto window = GetWindow();
		window->Resize( width, height );
		presentParameters.windowed = windowed;
		presentParameters.mode.width = window->GetClientWidth();
		presentParameters.mode.height = window->GetClientHeight();
		return renderContext->SetPresentParameters( 0, presentParameters );
	}
};

using namespace Tr2RenderContextEnum;

namespace
{

	ALResult CreatePositionOnlyVS( Tr2ShaderAL& shader, Tr2PrimaryRenderContextAL& renderContext )
	{
		uint8_t bytecode[] = {
	#include INCLUDE_SHADER_CODE( PositionOnly.vs )
		};

		auto input = Tr2ShaderSignatureAL().Add( Tr2VertexDefinition::POSITION, 0, 0, Tr2ShaderPipelineInputAL::FLOAT, 3 );

		return shader.Create( VERTEX_SHADER, bytecode, input, renderContext );
	}

	ALResult CreateConstantColorPS( Tr2ShaderAL& shader, Tr2PrimaryRenderContextAL& renderContext )
	{
		uint8_t bytecode[] = {
	#include INCLUDE_SHADER_CODE( ConstantColor.ps )
		};

		return shader.Create( PIXEL_SHADER, bytecode, Tr2ShaderSignatureAL(), renderContext );
	}

}

TEST_F( SwapChainResizing, CanResizeSwapChainWhileRendering )
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
		if( g % 200 == 0 )
		{
			if( g % 400 == 0 )
			{
				ASSERT_HRESULT_SUCCEEDED( ResizeWindow( 400, 400 ) );
			}
			else
			{
				ASSERT_HRESULT_SUCCEEDED( ResizeWindow( 640, 480 ) );
			}
		}


		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( RS_CULLMODE, CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( ResizeWindow( 640, 480 ) );
}

TEST_F( SwapChainResizing, CanSwitchToFullScreen )
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
		if( g == 200 )
		{
			ASSERT_HRESULT_SUCCEEDED( ResizeWindow( 1024, 768, false ) );
		}
		else if( g == 400 )
		{
			ASSERT_HRESULT_SUCCEEDED( ResizeWindow( 640, 480 ) );
		}


		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( RS_CULLMODE, CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( ResizeWindow( 640, 480 ) );
}


TEST_F( SwapChainResizing, CanChangeFullscreenResolution )
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
		if( g == 200 )
		{
			ASSERT_HRESULT_SUCCEEDED( ResizeWindow( 1024, 768, false ) );
		}
		else if( g == 400 )
		{
			ASSERT_HRESULT_SUCCEEDED( ResizeWindow( 1280, 1024, false ) );
		}
		else if( g == 600 )
		{
			ASSERT_HRESULT_SUCCEEDED( ResizeWindow( 640, 480 ) );
		}


		ASSERT_HRESULT_SUCCEEDED( renderContext->BeginScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Clear( CLEARFLAGS_TARGET, 0xff000000 | ( g & 0xff ), 1.0f ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, vb, 0, vbStride ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetVertexLayout( vertexLayout ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( sp ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetTopology( TOP_TRIANGLES ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( RS_ZENABLE, 0 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->SetRenderState( RS_CULLMODE, CULLMODE_NONE ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->DrawPrimitive( 0, 1 ) );
		ASSERT_HRESULT_SUCCEEDED( renderContext->EndScene() );
		ASSERT_HRESULT_SUCCEEDED( renderContext->Present() );
		g++;
	};

	RunLoop( frame );

	ASSERT_HRESULT_SUCCEEDED( renderContext->SetStreamSource( 0, Tr2BufferAL(), 0, 0 ) );
	ASSERT_HRESULT_SUCCEEDED( renderContext->SetShaderProgram( Tr2ShaderProgramAL() ) );
	ASSERT_HRESULT_SUCCEEDED( ResizeWindow( 640, 480 ) );
}
