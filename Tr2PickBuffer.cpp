#include "StdAfx.h"
#include "Tr2PickBuffer.h"
#include "Tr2Renderer.h"

using namespace Tr2RenderContextEnum;

// ------------------------------------------------------------------------------------------------------
Tr2PickBuffer::Tr2PickBuffer( IRoot* lockobj, Tr2RenderContextEnum::PixelFormat format, int size ) :
	m_size( size ),
	m_format( format ),
	m_clearColor( 0xFFFFFFFF )
{
}

// ------------------------------------------------------------------------------------------------------
Tr2PickBuffer::~Tr2PickBuffer()
{
	ReleaseResources( TRISTORAGE_ALL );
}

// ------------------------------------------------------------------------------------------------------
void Tr2PickBuffer::SetSize( int size )
{
	ReleaseResources( TRISTORAGE_ALL );
	m_size = size;
}

// ------------------------------------------------------------------------------------------------------
void Tr2PickBuffer::ReleaseResources( TriStorage s )
{
	m_pickTarget = Tr2TextureAL();
	m_depthBuffer = Tr2TextureAL();
}

// ------------------------------------------------------------------------------------------------------
bool Tr2PickBuffer::OnPrepareResources()
{
    int const bufferWidth( ( m_size > 0 )? m_size : Tr2Renderer::GetRenderTargetWidth() );
    int const bufferHeight( ( m_size > 0 )? m_size : Tr2Renderer::GetRenderTargetHeight() );

	// create the pixel buffer as a rendertarget
	USE_MAIN_THREAD_RENDER_CONTEXT();
	CR( m_pickTarget.Create( Tr2BitmapDimensions( bufferWidth, bufferHeight, 1, m_format ), Tr2GpuUsage::RENDER_TARGET, Tr2CpuUsage::READ_OFTEN, renderContext ) );
	CR( m_depthBuffer.Create( Tr2BitmapDimensions( bufferWidth, bufferHeight, 1, PIXEL_FORMAT_D24_UNORM_S8_UINT ), Tr2GpuUsage::DEPTH_STENCIL, renderContext ) );

	return true;
}

// ------------------------------------------------------------------------------------------------------
bool Tr2PickBuffer::BeginRendering( float initialDepth, Tr2RenderContext& renderContext )
{
	if( !m_pickTarget.IsValid() )
	{
		// This could happen if device is lost
		return false;
	}

	Tr2Renderer::PushRenderTarget( m_pickTarget, renderContext );
	Tr2Renderer::PushDepthStencilBuffer( m_depthBuffer, renderContext );

	CR( renderContext.Clear( CLEARFLAGS_TARGET | CLEARFLAGS_ZBUFFER, m_clearColor, initialDepth ) );

    Tr2Renderer::SetFullScreenViewport();

	return true;
}

// ------------------------------------------------------------------------------------------------------
bool Tr2PickBuffer::EndRendering( Tr2RenderContext& renderContext )
{
	Tr2Renderer::PopDepthStencilBuffer( renderContext );
	Tr2Renderer::PopRenderTarget( renderContext );

	return true;
}

// ------------------------------------------------------------------------------------------------------
bool Tr2PickBuffer::PrepareGetResults( const void*& data, uint32_t& pitch, Tr2RenderContext& renderContext )
{
	if( !m_pickTarget.IsValid() )
	{
		// This could happen if device is lost
		return false;
	}

	HRESULT hr = m_pickTarget.MapForReading( Tr2TextureSubresource( 0 ), data, pitch, renderContext );

	return SUCCEEDED( hr );
}

void Tr2PickBuffer::UnlockBuffer( Tr2RenderContext& renderContext )
{
	if( !m_pickTarget.IsValid() )
	{
		// This could happen if device is lost
		return;
	}
	m_pickTarget.UnmapForReading( renderContext );
}
