// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if ( TRINITY_PLATFORM == TRINITY_STUB )

#include "Tr2RenderContextStub.h"
#include "ITr2RenderContextEvents.h"
#include "ALLog.h"
#include "Tr2AdapterStructures.h"


CCP_STATS_DECLARE( vertexCount, "Trinity/AL/vertexCount", true, CST_COUNTER_HIGH, "Vertex count in DrawPrimitive calls." );


using namespace Tr2RenderContextEnum;
#pragma warning( disable : 4189 ) // Scopeguard

bool g_gatherPipelineStatistics = false;

namespace
{

Tr2PrimaryRenderContextAL*& GetPrimaryRenderContextPointer()
{
	static Tr2PrimaryRenderContextAL* primaryRenderContext = nullptr;
	return primaryRenderContext;
}

}



Tr2RenderContextAL::Tr2RenderContextAL() :
	m_isValid( false ),
	m_events( nullptr ),
	m_frameNumber( 0 )
{
	::GetPrimaryRenderContextPointer() = this;
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
		m_boundRenderTarget[i] = Tr2TextureAL();
	}
	m_isValid = false;
}



ALResult Tr2RenderContextAL::SetStreamSource( uint32_t, const Tr2BufferAL&, uint32_t, uint32_t ) throw()
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetIndices( const Tr2BufferAL& ) throw()
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetIndices( const Tr2BufferAL&, uint32_t ) throw()
{
	return S_OK;
}

ALResult Tr2RenderContextAL::ClearUav( const Tr2BufferAL&, const float[4] ) throw()
{
	return E_FAIL;
}

ALResult Tr2RenderContextAL::ClearUav( const Tr2BufferAL&, const uint32_t[4] ) throw()
{
	return E_FAIL;
}

ALResult Tr2RenderContextAL::CopySubBuffer( Tr2BufferAL&, uint32_t, Tr2BufferAL&, uint32_t, uint32_t )
{
	return E_FAIL;
}

ALResult Tr2RenderContextAL::Clear(
	uint32_t,
	uint32_t,
	float,
	uint32_t,
	uint32_t )
{
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
	uint32_t,
	uint32_t,
	uint32_t,
	uint32_t )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedInstanced(
	uint32_t,
	uint32_t,
	uint32_t,
	uint32_t )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedInstanced(
	uint32_t,
	uint32_t,
	uint32_t,
	int32_t,
	uint32_t )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawInstanced(
	uint32_t,
	uint32_t,
	uint32_t,
	uint32_t )
{
	return S_OK;
}


ALResult Tr2RenderContextAL::DrawPrimitive( uint32_t, uint32_t )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawPrimitiveUP(
	uint32_t,
	const void*,
	uint32_t )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitiveUP(
	uint32_t,
	uint32_t,
	const uint32_t* indexData,
	const void* vertexStreamZeroData,
	uint32_t )
{
	if( !indexData || !vertexStreamZeroData )
	{
		return E_FAIL;
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitiveUP(
	uint32_t,
	uint32_t,
	const uint16_t* indexData,
	const void* vertexStreamZeroData,
	uint32_t )
{
	if( !indexData || !vertexStreamZeroData )
	{
		return E_FAIL;
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::SetConstants(
	const Tr2ConstantBufferAL&,
	ShaderType,
	uint32_t,
	uint32_t )
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
ALResult Tr2RenderContextAL::SetDepthStencil( const Tr2TextureAL& )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetRenderTarget( const Tr2TextureAL& renderTarget, uint32_t slot, uint32_t )
{
	m_boundRenderTarget[slot] = renderTarget;
	return S_OK;
}

ALResult Tr2RenderContextAL::CreateDevice(
	uint32_t Adapter,
	Tr2WindowHandle,
	const Tr2PresentParametersAL& presentationParameters )
{
	m_isValid = true;
	SetPresentParameters( Adapter, presentationParameters );
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

ALResult Tr2RenderContextAL::SetPresentParameters( unsigned, const Tr2PresentParametersAL& presentationParameters )
{
	CR_RETURN_HR( m_defaultBackBuffer.Create(
		Tr2BitmapDimensions( presentationParameters.mode.width, presentationParameters.mode.height, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ),
		Tr2GpuUsage::RENDER_TARGET,
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
	++m_frameNumber;
	return S_OK;
}

bool Tr2RenderContextAL::IsValid()
{
	return m_isValid;
}

ALResult Tr2RenderContextAL::SetVertexLayout( const Tr2VertexLayoutAL& )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetShaderProgram( const Tr2ShaderProgramAL& )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetRenderState( RenderState, uint32_t )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetRenderStates( const uint32_t*, uint32_t )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::SetResourceSet( const Tr2ResourceSetAL& )
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
	if( !m_boundRenderTarget[slot].IsValid() )
	{
		return E_INVALIDCALL;
	}
	else
	{
		width = m_boundRenderTarget[slot].GetWidth();
		height = m_boundRenderTarget[slot].GetHeight();
	}
	return S_OK;
}

void Tr2RenderContextAL::ReleaseDeviceResources()
{
	for( unsigned i = 0; i != MAX_RENDER_TARGET; ++i )
	{
		m_boundRenderTarget[i] = Tr2TextureAL();
	}


	m_defaultBackBuffer = Tr2TextureAL();
}

// --------------------------------------------------------------------------------------
void Tr2RenderContextAL::AddGpuMarker( const char* )
{
}

void Tr2RenderContextAL::PushGpuMarker( const char* )
{
}

void Tr2RenderContextAL::PopGpuMarker()
{
}

// --------------------------------------------------------------------------------------
ALResult Tr2RenderContextAL::GetGpuStateMarker( Tr2RenderContextEnum::RenderContextStatus&, std::string& ) const
{
	return E_FAIL;
}

// --------------------------------------------------------------------------------------
ALResult Tr2RenderContextAL::GetGpuPageFaultResource(
	Tr2RenderContextEnum::PixelFormat&,
	uint64_t&,
	uint32_t&,
	uint32_t&,
	uint32_t&,
	uint32_t& ) const
{
	return E_FAIL;
}

void Tr2RenderContextAL::RenderPassHint( const Tr2ColorAttachment&, const Tr2DepthAttachment& )
{
}

void Tr2RenderContextAL::RenderPassHint( const Tr2ColorAttachment&, const Tr2ColorAttachment&, const Tr2DepthAttachment& )
{
}

ALResult Tr2RenderContextAL::UseResources( Tr2UseResourceDestination, Tr2GpuUsage::Type, const Tr2BindlessResourcesAL& )
{
	return S_OK;
}

ALResult Tr2RenderContextAL::UseAccelerationStructure( Tr2RtTopLevelAccelerationStructureAL tlas )
{
	return S_OK;
}

bool Tr2RenderContextAL::SupportsBindlessTextures() const
{
	return false;
}

uint64_t Tr2RenderContextAL::GetRecordingFrameNumber() const
{
	return m_frameNumber + 1;
}


Tr2UpscalingAL::Result Tr2RenderContextAL::EnableUpscaling( Tr2UpscalingAL::Technique tech, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter )
{
	return Tr2UpscalingAL::Result::OK;
}

Tr2UpscalingContextAL* Tr2RenderContextAL::GetUpscalingContext( uint32_t upscalingContextID )
{
	return nullptr;
}

Tr2UpscalingContextAL* Tr2RenderContextAL::CreateUpscalingContext( Tr2UpscalingAL::UpscalingContextParams params, uint32_t existingContext )
{
	return nullptr;
}

void Tr2RenderContextAL::DeleteUpscalingContext( uint32_t contextID )
{
}

Tr2UpscalingAL::UpscalingInfo Tr2RenderContextAL::GetUpscalingInfo( uint32_t upscalingContextID )
{
	return Tr2UpscalingAL::UpscalingInfo();
}

void Tr2PrimaryRenderContextAL::GetUpscalingSetup( Tr2UpscalingAL::Technique& technique, Tr2UpscalingAL::Setting& setting, bool& framegeneration, bool& temporal )
{
	technique = Tr2UpscalingAL::Technique::NONE;
	setting = Tr2UpscalingAL::Setting::NATIVE;
	framegeneration = false;
	temporal = false;
}

std::vector<std::tuple<Tr2UpscalingAL::Technique, uint32_t, bool>> Tr2RenderContextAL::GetSupportedUpscalingTechniques( uint32_t adapter )
{
	return std::vector<std::tuple<Tr2UpscalingAL::Technique, uint32_t, bool>>();
}


void Tr2RenderContextAL::MarkFrameEvent( Tr2RenderContextEnum::FrameEvent frameEvent )
{
}
uint64_t Tr2RenderContextAL::GetRenderedFrameNumber() const
{
	return m_frameNumber;
}


#endif
