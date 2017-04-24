#include "StdAfx.h"
#include "Tr2RenderContextStub.h"
#include "ITr2RenderContextEvents.h"

#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "ALLog.h"


using namespace Tr2RenderContextEnum;
#pragma warning( disable: 4189 )	// Scopeguard

namespace
{

Tr2PrimaryRenderContextAL*& GetPrimaryRenderContextPointer()
{
	static Tr2PrimaryRenderContextAL* primaryRenderContext = nullptr;
	return primaryRenderContext;
}

}



Tr2RenderContextAL::Tr2RenderContextAL()
	: m_isValid(false),
	m_events( nullptr )
{
	::GetPrimaryRenderContextPointer() = this;
	for( unsigned i = 0; i != MAX_RENDER_TARGET; ++i )
	{
		m_boundRenderTarget[i] = nullptr;
	}
}

Tr2RenderContextAL::~Tr2RenderContextAL()
{
}

void Tr2RenderContextAL::SetPrimaryRenderContext( Tr2PrimaryRenderContextAL* renderContext )
{
	::GetPrimaryRenderContextPointer() = renderContext;
}

Tr2PrimaryRenderContextAL& Tr2RenderContextAL::GetPrimaryRenderContext()
{
	CCP_ASSERT( GetPrimaryRenderContextPointer() );
	return *GetPrimaryRenderContextPointer();
}

Tr2PrimaryRenderContextAL* Tr2RenderContextAL::GetPrimaryRenderContextPointer()
{
	return ::GetPrimaryRenderContextPointer();
}

void Tr2RenderContextAL::Destroy()
{
	for( unsigned i = 0; i != MAX_RENDER_TARGET; ++i )
	{
		m_boundRenderTarget[i] = nullptr;
	}
	m_isValid = false;
}

ALResult Tr2RenderContextAL::SetStreamSource( 
	uint32_t stream, 
	const Tr2VertexBufferAL & buffer, 
	uint32_t offset, 
	uint32_t stride )
{
	if( !buffer.IsValid() && &buffer != &nullVB )
	{
		return E_INVALIDARG;
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::Clear(	
	uint32_t clearFlags, 
	uint32_t color, 
	float depth, 
	uint32_t stencil,
	uint32_t slot )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetIndices( const Tr2IndexBufferAL& buffer )
{
	if( !buffer.IsValid() && ( &buffer != &nullIB ) )
	{
		return E_INVALIDARG;
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::SetTopology( long topology )
{
	if( topology >= TOP_MAX_TOPOLOGY )
	{
		return E_FAIL;
	}
	return S_OK;
}


ALResult Tr2RenderContextAL::DrawIndexedPrimitive(	
	uint32_t numVertices, 
	uint32_t startIndex, 
	uint32_t primitiveCount, 
	uint32_t minimumIndex )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedInstanced(	
	uint32_t numVertices, 
	uint32_t startIndex, 
	uint32_t primitiveCount, 
	uint32_t numInstances )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawPrimitive( uint32_t startVertex, uint32_t primitiveCount )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawPrimitiveUP( 
	uint32_t primitiveCount, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitiveUP(	
	uint32_t numVertices, 
	uint32_t primitiveCount, 
	const uint32_t* indexData, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride )
{
	if( !indexData || !vertexStreamZeroData )
	{
		return E_FAIL;
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitiveUP(	
	uint32_t numVertices, 
	uint32_t primitiveCount, 
	const uint16_t* indexData, 
	const void* vertexStreamZeroData, 
	uint32_t vertexStreamZeroStride )
{
	if( !indexData || !vertexStreamZeroData )
	{
		return E_FAIL;
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::SetConstants(
	Tr2ConstantBufferAL& buffer, 
	ShaderType constantType, 
	uint32_t registerIndex, 
	uint32_t maxRegisterCount )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetSamplerState( 
	const Tr2SamplerStateAL& samplerState, 
	ShaderType /*inputType*/, 
	uint32_t registerNumber )
{
	return S_OK;
}

void Tr2RenderContextAL::SetReadOnlyDepth( bool /*enable*/ ) 
{
}

bool Tr2RenderContextAL::GetReadOnlyDepth() const
{
	return false;
}
ALResult Tr2RenderContextAL::SetDepthStencil( const Tr2DepthStencilAL& depthStencil )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetRenderTarget( const Tr2RenderTargetAL& renderTarget, uint32_t slot )
{
	m_boundRenderTarget[slot] = &renderTarget;
	return S_OK;
}

ALResult Tr2RenderContextAL::CreateDevice(	
	uint32_t Adapter, 
	Tr2WindowHandle  hFocusWindow, 
	const Tr2PresentParametersAL& presentationParameters )
{
	m_isValid = true;
	if( m_events )
	{
		m_events->OnContextCreated( *this );
	}
	
	return S_OK;
}

PixelFormat Tr2RenderContextAL::GetBackBufferFormat() const
{
	return m_defaultBackBuffer.GetFormat();
}

ALResult Tr2RenderContextAL::GetAFRGroupCount( uint32_t& count )
{
	count = 1;
	return S_OK;
}

ALResult Tr2RenderContextAL::SetPresentParameters( unsigned adapter, const Tr2PresentParametersAL& presentationParameters )
{
	CR_RETURN_HR( m_defaultBackBuffer.Create(	
		presentationParameters.mode.width,
		presentationParameters.mode.height,
		1,
		PIXEL_FORMAT_B8G8R8A8_UNORM,
		1,
		0,
		*this ) );

	SetRenderTarget( m_defaultBackBuffer );

	return S_OK;
}

const Tr2CapsAL& Tr2RenderContextAL::GetCaps() const
{
	return m_caps;
}

ALResult Tr2RenderContextAL::BeginScene()
{
	return S_OK;
}

ALResult Tr2RenderContextAL::EndScene()
{
	return S_OK;
}

ALResult Tr2RenderContextAL::Present()
{
	return S_OK;
}

bool Tr2RenderContextAL::IsValid()
{
	return m_isValid;
}

ALResult Tr2RenderContextAL::SetVertexLayout( const Tr2VertexLayoutAL& layout )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetShader( const Tr2ShaderAL& shader )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetRenderState( RenderState state, uint32_t value )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetRenderStates( const uint32_t* stateValuePairs, uint32_t count )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::GetRenderState( RenderState state, uint32_t* value )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetClipPlane( uint32_t planeIndex, const float* planeEq )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetScissorRect(	
	uint32_t left, 
	uint32_t top, 
	uint32_t right, 
	uint32_t bottom )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetShaderBuffer(		
	ShaderType /* inputType */, 
	uint32_t /* slot */, 
	const Tr2GpuBufferAL& /* buffer */ )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetTexture(	
	ShaderType inputType, 
	uint32_t slot, 
	const Tr2TextureAL& texture, 
	Tr2RenderContextEnum::ColorSpace colorSpace )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetNumberOfLights( uint32_t numLights )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetViewport( const Tr2Viewport& viewport )
{
	m_viewport = viewport;
	return S_OK;
}

ALResult Tr2RenderContextAL::GetViewport( Tr2Viewport& viewport )
{
	viewport = m_viewport;
	return S_OK;
}

bool Tr2RenderContextAL::IsSupportedRenderTargetFormat( PixelFormat format, bool withAutoGenMipmap )
{
	return true;
}

long Tr2RenderContextAL::GetTotalVideoMemory()
{
	return 0;
}

ALResult Tr2RenderContextAL::PushRenderTarget( uint32_t slot )
{
	CCP_ASSERT( slot < MAX_RENDER_TARGET );
	if( slot >= MAX_RENDER_TARGET )
	{
		return E_INVALIDARG;
	}
	
	m_stackRT[slot].push( m_boundRenderTarget[slot] );
	return S_OK;
}

ALResult Tr2RenderContextAL::PopRenderTarget( uint32_t slot )
{
	CCP_ASSERT( slot < MAX_RENDER_TARGET );
	if( slot >= MAX_RENDER_TARGET )
	{
		return E_INVALIDARG;
	}
	CCP_ASSERT( !m_stackRT[slot].empty() );
	if( m_stackRT[slot].empty() )
	{
		return E_FAIL;
	}

	m_boundRenderTarget[slot] = m_stackRT[slot].top();
	m_stackRT[slot].pop();
	return S_OK;
}
	
ALResult Tr2RenderContextAL::PushDepthStencil()
{
	return S_OK;
}

ALResult Tr2RenderContextAL::PopDepthStencil()
{
	return S_OK;
}

ALResult Tr2RenderContextAL::GetRenderTargetSize( uint32_t& width, uint32_t& height, uint32_t slot )
{
	if( slot >= MAX_RENDER_TARGET )
	{
		return E_FAIL;
	}
	if( m_boundRenderTarget[slot] == &nullRT )
	{
		return E_INVALIDCALL;
	}
	else if(m_boundRenderTarget[slot] == nullptr)
	{
		width = height = 0;
	}
	else
	{
		width = m_boundRenderTarget[slot]->GetWidth();
		height = m_boundRenderTarget[slot]->GetHeight();
	}
	return S_OK;
}

void Tr2RenderContextAL::ReleaseDeviceResources()
{
	for( unsigned i = 0; i != MAX_RENDER_TARGET; ++i )
	{
		if( m_boundRenderTarget[i] )
		{
			m_boundRenderTarget[i] = nullptr;
		}
	}
	

	m_defaultBackBuffer.Destroy();
}

#endif
