#include "StdAfx.h"
#include "Tr2ConstantBufferFormats.h"
#include "Tr2Renderer.h"
#include "TriDevice.h"

// Check the sizes of the objects. These must fill float4 registers.
static_assert( sizeof( Tr2PerFrameVSData ) % ( 4*sizeof( float ) ) == 0, "Size of per-frame data needs to be a multiple of Vector4" );
static_assert( sizeof( Tr2PerFramePSData ) % ( 4*sizeof( float ) ) == 0, "Size of per-frame data needs to be a multiple of Vector4" );
static_assert( sizeof( Tr2PerObjectPerPixelPointLightData ) % ( 4*sizeof( float ) ) == 0, "Size of per-object light data needs to be a multiple of Vector4" );

void Tr2PopulatePerFrameVSDataTransformations( Tr2PerFrameVSData &data )
{
    // 0
    memset( &data, 0, sizeof( Tr2PerFrameVSData ) );

    // column_major for shaders
    data.ViewMat = XMMatrixTranspose( Tr2Renderer::GetViewTransform() );
    data.ProjectionMat = XMMatrixTranspose( Tr2Renderer::GetProjectionTransform() );

    data.ViewProjectionMat = XMMatrixTranspose(
        XMMatrixMultiply(
        Tr2Renderer::GetViewTransform(),
        Tr2Renderer::GetProjectionTransform() ) );

    // attention: need the transposed, but shader also needs column_major, so it is transpose(transpose(m)) == m
    data.ViewInverseTransposeMat = Tr2Renderer::GetInverseViewTransform();
}

namespace {

	Tr2ConstantBufferAL	s_perFrameVSData;
	Tr2ConstantBufferAL	s_perFramePSData;

	struct Destroyer : Tr2DeviceResource
	{
		virtual void ReleaseResources( TriStorage s )
		{
			s_perFrameVSData = Tr2ConstantBufferAL();
			s_perFramePSData = Tr2ConstantBufferAL();
		}

		virtual bool OnPrepareResources() { return true; }
	};

	Destroyer s_destroyer;
}

void Tr2BindPerFrameVSData( Tr2PerFrameVSData& data, Tr2RenderContext& renderContext )
{
	FillAndSetConstants( s_perFrameVSData, &data, sizeof( data ), Tr2RenderContextEnum::VERTEX_SHADER, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );
}

void Tr2BindPerFramePSData( Tr2PerFramePSData& data, Tr2RenderContext& renderContext )
{
	FillAndSetConstants( s_perFramePSData, &data, sizeof( data ), Tr2RenderContextEnum::PIXEL_SHADER, Tr2Renderer::GetPerFramePSStartRegister(), renderContext );
}
