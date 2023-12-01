#include "StdAfx.h"

#if( TRINITY_PLATFORM == TRINITY_METAL )

#include "Tr2RenderContextMetal.h"
#include "ITr2RenderContextEvents.h"
#include "ALLog.h"
#include "Tr2AdapterStructures.h"
#include "Tr2BufferALMetal.h"
#include "Tr2ConstantBufferALMetal.h"
#include "Tr2RenderContextMetal.h"
#include "Tr2VertexLayoutALMetal.h"
#include "Tr2ResourceSetALMetal.h"
#include "Tr2ShaderProgramALMetal.h"
#include "Tr2TextureALMetal.h"
#include "Tr2SwapChainALMetal.h"


#include "MetalContext.h"

CCP_STATS_DECLARE( primitiveCount		, "Trinity/AL/primitiveCount"		, true, CST_COUNTER_HIGH, "Primitive count in DrawPrimitive calls." );
CCP_STATS_DECLARE( vertexCount			, "Trinity/AL/vertexCount"			, true, CST_COUNTER_HIGH, "Vertex count in DrawPrimitive calls." );
CCP_STATS_DECLARE( sceneDrawcallCount	, "Trinity/AL/sceneDrawcallCount"	, true, CST_COUNTER_LOW,  "Number of DrawPrimitive calls." );


using namespace Tr2RenderContextEnum;
#pragma warning( disable: 4189 )	// Scopeguard

bool g_gatherPipelineStatistics = false;
bool g_enableMetalCounters = false;

namespace
{
	Tr2PrimaryRenderContextAL*& GetPrimaryRenderContextPointer()
	{
		static Tr2PrimaryRenderContextAL* primaryRenderContext = nullptr;
		return primaryRenderContext;
	}
	
	MTLClearColor MakeClearColor( uint32_t color )
	{
		MTLClearColor clearColor;
		clearColor.alpha = float( ( color >> 24 ) & 0xff ) / 255.f;
		clearColor.red   = float( ( color >> 16 ) & 0xff ) / 255.f;
		clearColor.green = float( ( color >> 8  ) & 0xff ) / 255.f;
		clearColor.blue  = float( ( color >> 0  ) & 0xff ) / 255.f;
		return clearColor;
	}
}

Tr2RenderContextAL::Tr2RenderContextAL()
	: m_isValid(false)
	, m_events( nullptr )
	, m_metalContext( nil )
	, m_workQueue( nil )
	, m_depthWriteEnabled( true )
	, m_depthCompareEnabled( false )
	, m_readOnlyDepth( false )
	, m_alphaBlendEnable( false )
	, m_separateAlphaBlendEnabled( false )
	, m_srgbWriteEnable( false )
    , m_isPrimary( false )
{
}

Tr2RenderContextAL::~Tr2RenderContextAL()
{
	m_vertexLayout = Tr2VertexLayoutAL();
	m_resourceSet = Tr2ResourceSetAL();
	m_shaderProgram = Tr2ShaderProgramAL();

	if( m_isPrimary )
	{
		delete m_metalContext;
	}
	m_metalContext = nil;
	m_workQueue = nil;
}

void Tr2RenderContextAL::SetAsPrimary()
{
	m_isPrimary = true;
	m_metalContext = new TrinityALImpl::MetalContext;
	m_depthCompareFunction = MTLCompareFunctionAlways;

	m_metalIndexBuffer = nil;
	m_caMetalLayer     = nil;
	m_defaultBackBuffer.m_texture = std::make_shared<TrinityALImpl::Tr2TextureAL>();
	m_boundDepthStencil.m_texture = nullptr;
	m_needsDrawResourceCheck = true;
	
	m_swapChain.m_swapChain = std::make_shared<TrinityALImpl::Tr2SwapChainAL>();
}

void Tr2RenderContextAL::SetPrimaryRenderContext( Tr2PrimaryRenderContextAL* renderContext )
{
	if( renderContext )
	{
		renderContext->SetAsPrimary();
	}
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
		m_boundRenderTargets[i] = {};
	}
	m_boundDepthStencil = Tr2TextureAL();
	m_isValid = false;
}

ALResult Tr2RenderContextAL::SetStreamSource(uint32_t stream,
                                             const Tr2BufferAL & buffer,
                                             uint32_t offset,
                                             uint32_t stride ) throw( )
{
	if( !IsValid() )
	{
		return E_FAIL;
	}
	if( stream >= VERTEX_STREAM_COUNT || stream == METAL_VERTEX_STREAM_DUMMY )
	{
		return E_INVALIDARG;
	}

	if( !buffer.IsValid() )
	{
		m_workQueue->SetVertexStream( stream, nil, 0, 0 );
		return S_OK;
	}

	m_workQueue->SetVertexStream( stream, buffer.m_buffer->GetMetalBuffer(), stride, offset );

	m_boundVertexStreams[stream].buffer = buffer.m_buffer->GetMetalBuffer();
	m_boundVertexStreams[stream].offset = offset;
	m_boundVertexStreams[stream].stride = stride;

	return S_OK;
}

ALResult Tr2RenderContextAL::SetIndices( const Tr2BufferAL& buffer) throw( )
{
	if( !buffer.IsValid() )
	{
		m_metalIndexBuffer = nil;
		return S_OK;
	}
	
	if( buffer.GetDesc().stride == 2 )
	{
		m_metalIndexType = MTLIndexTypeUInt16;
	}
	else if( buffer.GetDesc().stride == 4 )
	{
		m_metalIndexType = MTLIndexTypeUInt32;
	}
	else
	{
		m_metalIndexType = MTLIndexTypeUInt32;
		CCP_AL_LOGWARN( "Unsupported index buffer stride: %d. Defaulting to 4.", (int)buffer.GetDesc().stride );
	}

	m_metalIndexBuffer = buffer.m_buffer->GetMetalBuffer();

	return S_OK;
}

void Tr2RenderContextAL::BufferRewritten( id<MTLBuffer> from, id<MTLBuffer> to )
{
	if( m_metalIndexBuffer == from )
	{
		m_metalIndexBuffer = to;
	}
	for( uint32_t i = 0; i < VERTEX_STREAM_COUNT; ++i )
	{
		if( m_boundVertexStreams[i].buffer == from )
		{
			TrinityALImpl::MetalContext* metalContext = GetMetalContext();
			m_workQueue->SetVertexStream( i, to, m_boundVertexStreams[i].stride, m_boundVertexStreams[i].offset );
			m_boundVertexStreams[i].buffer = to;
		}
	}
}

ALResult Tr2RenderContextAL::ClearUav( Tr2BufferAL& buffer, const float values[4] ) throw( )
{
    if( !buffer.IsValid() )
    {
        return E_INVALIDARG;
    }
	m_workQueue->ClearBuffer( buffer.m_buffer->GetMetalBuffer(), values );

	return S_OK;
}

ALResult Tr2RenderContextAL::ClearUav( Tr2BufferAL& buffer, const uint32_t values[4] ) throw( )
{
    if( !buffer.IsValid() )
    {
        return E_INVALIDARG;
    }
	m_workQueue->ClearBuffer( buffer.m_buffer->GetMetalBuffer(), values );

	return S_OK;
}

ALResult Tr2RenderContextAL::ClearUav( Tr2TextureAL& texture, uint32_t mipLevel, const float values[4] ) throw( )
{
    if( !texture.IsValid() )
    {
        return E_INVALIDARG;
    }
	m_workQueue->ClearTexture( texture.m_texture->GetMetalTexture(), mipLevel, values );

	return S_OK;
}

ALResult Tr2RenderContextAL::ClearUav( Tr2TextureAL& texture, uint32_t mipLevel, const uint32_t values[4] ) throw( )
{
    if( !texture.IsValid() )
    {
        return E_INVALIDARG;
    }
	m_workQueue->ClearTexture( texture.m_texture->GetMetalTexture(), mipLevel, values );

	return S_OK;
}

ALResult Tr2RenderContextAL::CopySubBuffer( Tr2BufferAL&, uint32_t, Tr2BufferAL&, uint32_t, uint32_t )
{
	return E_FAIL;
}

ALResult Tr2RenderContextAL::Clear(
	uint32_t clearFlags,
	uint32_t color,
	float depth,
	uint32_t stencil,
	uint32_t slot )
{
	if( !IsValid() )
	{
		return E_FAIL;
	}

	MTLClearColor *clearColor   = nullptr;
	float    *clearDepth        = nullptr;
	uint32_t *clearStencil      = nullptr;
	MTLClearColor metalClearColor;

	if( clearFlags & Tr2RenderContextEnum::CLEARFLAGS_TARGET )
	{
		if( m_boundRenderTargets[slot].texture.IsValid() )
		{
			metalClearColor = MakeClearColor( color );
			clearColor = &metalClearColor;
		}
		else
		{
			return E_INVALIDCALL;
		}
	}

	if( clearFlags & Tr2RenderContextEnum::CLEARFLAGS_ZBUFFER )
	{
		if( m_boundDepthStencil.IsValid() )
		{
			clearDepth = &depth;
		}
		else
		{
			return E_INVALIDCALL;
		}
	}

	if( clearFlags & Tr2RenderContextEnum::CLEARFLAGS_STENCIL )
	{
		if( m_boundDepthStencil.IsValid() )
		{
			clearStencil = &stencil;
		}
		else
		{
			return E_INVALIDCALL;
		}
	}

	if( clearColor || clearDepth || clearStencil )
	{
		m_workQueue->ClearAttachment( clearColor, clearDepth, clearStencil, slot );
	}

	return S_OK;
}

ALResult Tr2RenderContextAL::SetTopology( Tr2RenderContextEnum::Topology topology )
{
	if( topology >= TOP_MAX_TOPOLOGY )
	{
		return E_INVALIDARG;
	}
	if (topology == TOP_TRIANGLE_FAN)
	{
		CCP_AL_LOGERR( "Fan topology is not supported in Metal." );
		return E_FAIL;
	}

	static const MetalPrimitiveInfo topologyMapping[ TOP_MAX_TOPOLOGY ] = {
		{MTLPrimitiveTypeTriangle,      3, 0, false}, // TOP_INVALID
		{MTLPrimitiveTypeTriangle,      3, 0, true},  // TOP_TRIANGLES,
		{MTLPrimitiveTypeTriangleStrip, 1, 2, true},  // TOP_TRIANGLE_STRIP
		{MTLPrimitiveTypeTriangle,      3, 0, false}, // TOP_TRIANGLE_FAN - NOT SUPPORTED!
		{MTLPrimitiveTypeLine,          2, 0, true},  // TOP_LINES
		{MTLPrimitiveTypeLineStrip,     1, 1, true},  // TOP_LINE_STRIP
		{MTLPrimitiveTypePoint,         1, 0, false}  // TOP_POINTS
	};

	m_metalPrimitiveInfo = topologyMapping[topology];
	m_topology = topology;

	return S_OK;
}

uint32_t Tr2RenderContextAL::ComputeVertexCount( uint32_t primitiveCount )
{
	CCP_ASSERT( m_metalPrimitiveInfo.validType );

	return (m_metalPrimitiveInfo.primitiveToIndexMultiplier * primitiveCount) + m_metalPrimitiveInfo.primtiveToIndexAddition;
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitive(
	uint32_t numVertices,
	uint32_t startIndex,
	uint32_t primitiveCount,
	uint32_t minimumIndex )
{
    if( m_metalIndexBuffer == nil )
    {
        return E_INVALIDARG; 
    }
    
	auto vc = ComputeVertexCount( primitiveCount );

	CCP_STATS_ADD( primitiveCount, primitiveCount );
	CCP_STATS_ADD( vertexCount, vc );
	CCP_STATS_INC( sceneDrawcallCount );

	CheckDrawResources();

	m_workQueue->DrawIndexedPrimitives(m_metalPrimitiveInfo.metalPrimitiveType, vc, m_metalIndexType, m_metalIndexBuffer, startIndex, 1);

	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedInstanced(
	uint32_t numVertices,
	uint32_t startIndex,
	uint32_t primitiveCount,
	uint32_t numInstances )
{
    if( m_metalIndexBuffer == nil )
    {
        return E_INVALIDARG;
    }
    
	auto vc = ComputeVertexCount( primitiveCount );

	CCP_STATS_ADD( primitiveCount, primitiveCount * numInstances );
	CCP_STATS_ADD( vertexCount, vc * numInstances );
	CCP_STATS_INC( sceneDrawcallCount );

	CheckDrawResources();

	m_workQueue->DrawIndexedPrimitives(m_metalPrimitiveInfo.metalPrimitiveType, vc, m_metalIndexType, m_metalIndexBuffer, startIndex, numInstances);

	return S_OK;
}

ALResult Tr2RenderContextAL::DrawIndexedInstancedIndirect( Tr2BufferAL& params, uint32_t offset )
{
	if( !params.IsValid() )
	{
		return E_INVALIDARG;
	}

	CCP_STATS_INC( sceneDrawcallCount );

	CheckDrawResources();

	m_workQueue->DrawIndexedPrimitives( m_metalPrimitiveInfo.metalPrimitiveType, m_metalIndexType, m_metalIndexBuffer, 0, params.m_buffer->GetMetalBuffer(), offset );

	return S_OK;
}

ALResult Tr2RenderContextAL::DrawInstancedIndirect( Tr2BufferAL& params, uint32_t offset )
{
	if( !params.IsValid() )
	{
		return E_INVALIDARG;
	}

	CCP_STATS_INC( sceneDrawcallCount );

	CheckDrawResources();

	m_workQueue->DrawPrimitives( m_metalPrimitiveInfo.metalPrimitiveType, params.m_buffer->GetMetalBuffer(), offset );

	return S_OK;
}

void Tr2RenderContextAL::CheckDrawResources()
{
	// Only need to check resources if we don't have a resource set and the shader has changed since the last draw.
	if( m_needsDrawResourceCheck && !m_resourceSet.IsValid() )
    {
        m_shaderProgram.m_program->SetDummyResources( *m_workQueue );
        m_needsDrawResourceCheck = false;
    }

	if( m_vertexLayout.IsValid() )
	{
		m_vertexLayout.m_layout->SetVertexLayout( m_shaderProgram.m_program, *this );
	}
	else
	{
		m_workQueue->SetCurrentVertexDescriptor( nil, 0 );
	}
}

ALResult Tr2RenderContextAL::DrawPrimitive( uint32_t startVertex, uint32_t primitiveCount )
{
	auto vc = ComputeVertexCount( primitiveCount );

	CCP_STATS_ADD( primitiveCount, primitiveCount );
	CCP_STATS_ADD( vertexCount, vc );
	CCP_STATS_INC( sceneDrawcallCount );

	CheckDrawResources();

	m_workQueue->DrawPrimitives(m_metalPrimitiveInfo.metalPrimitiveType, vc, startVertex, 1);

	return S_OK;
}

ALResult Tr2RenderContextAL::DrawPrimitiveUP(
	uint32_t primitiveCount,
	const void* vertexStreamZeroData,
	uint32_t vertexStreamZeroStride )
{
	auto vc = ComputeVertexCount( primitiveCount );

	CCP_STATS_ADD( primitiveCount, primitiveCount );
	CCP_STATS_ADD( vertexCount, vc );
	CCP_STATS_INC( sceneDrawcallCount );

    CheckDrawResources();

	return m_drawUPHelper.DrawPrimitiveUP( m_topology, primitiveCount, vertexStreamZeroData, vertexStreamZeroStride, *this, GetPrimaryRenderContext() );
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitiveUP(
	uint32_t numVertices,
	uint32_t primitiveCount,
	const uint32_t* indexData,
	const void* vertexStreamZeroData,
	uint32_t vertexStreamZeroStride )
{
	CCP_STATS_ADD( primitiveCount, primitiveCount );
	CCP_STATS_ADD( vertexCount, numVertices );
	CCP_STATS_INC( sceneDrawcallCount );

    CheckDrawResources();

    return m_drawUPHelper.DrawIndexedPrimitiveUP( m_topology, numVertices, primitiveCount, indexData, vertexStreamZeroData, vertexStreamZeroStride, *this, GetPrimaryRenderContext() );
}

ALResult Tr2RenderContextAL::DrawIndexedPrimitiveUP(
	uint32_t numVertices,
	uint32_t primitiveCount,
	const uint16_t* indexData,
	const void* vertexStreamZeroData,
	uint32_t vertexStreamZeroStride )
{
	CCP_STATS_ADD( primitiveCount, primitiveCount );
	CCP_STATS_ADD( vertexCount, numVertices );
	CCP_STATS_INC( sceneDrawcallCount );

	CheckDrawResources();

    return m_drawUPHelper.DrawIndexedPrimitiveUP( m_topology, numVertices, primitiveCount, indexData, vertexStreamZeroData, vertexStreamZeroStride, *this, GetPrimaryRenderContext() );
}

ALResult Tr2RenderContextAL::RunComputeShader( unsigned groupDimX, unsigned groupDimY, unsigned groupDimZ )
{
	m_workQueue->Dispatch( groupDimX, groupDimY, groupDimZ );

	return S_OK;
}

ALResult Tr2RenderContextAL::RunComputeShaderIndirect( Tr2BufferAL& indirectParams, unsigned offset )
{
	if( !indirectParams.IsValid() )
	{
		return E_FAIL;
	}

	m_workQueue->Dispatch( indirectParams.m_buffer->GetMetalBuffer(), offset );

	return S_OK;
}

ALResult Tr2RenderContextAL::SetConstants(
	const Tr2ConstantBufferAL& buffer,
	ShaderType shaderType,
	uint32_t registerIndex,
	uint32_t maxRegisterCount )
{
	if( registerIndex >= METAL_CONST_BUFFER_COUNT )
	{
		return E_INVALIDARG;
	}

	buffer.m_buffer->SetConstants( shaderType, registerIndex, *this );

	return S_OK;
}

void Tr2RenderContextAL::SetReadOnlyDepth( bool enable )
{
	if( m_readOnlyDepth == enable )
	{
		return;
	}
	m_readOnlyDepth = enable;

	m_workQueue->SetDepthState( m_depthWriteEnabled && !enable );

	if( !enable )
	{
		// We don't need to do anything special for READ -> WRITE transition
		return;
	}
	
	if( !m_boundDepthStencil.IsValid() )
	{
		return;
	}
	
	// JM - Blunt approach for now - barrier on render targets. Added proper resource tracking later (maybe).
	m_workQueue->RenderTargetBarrier();
}

bool Tr2RenderContextAL::GetReadOnlyDepth() const
{
	return m_readOnlyDepth;
}

ALResult Tr2RenderContextAL::SetDepthStencil( const Tr2TextureAL& depthStencil )
{
	if( depthStencil == m_boundDepthStencil )
	{
		return S_OK;
	}

	id<MTLTexture> depthStencilTexture = depthStencil.m_texture->GetMetalTexture();

	if( depthStencil.IsValid() && depthStencilTexture == nullptr )
	{
		return E_INVALIDARG;
	}

	m_boundDepthStencil = depthStencil;

	m_workQueue->SetDepthStencilAttachment(depthStencilTexture);
	return S_OK;
}

ALResult Tr2RenderContextAL::SetRenderTarget( const Tr2TextureAL& renderTarget, uint32_t slot, uint32_t slice )
{
	id<MTLTexture> renderTargetTexture = nil;
	if( renderTarget.m_texture->IsValid() )
	{
		renderTargetTexture = !m_srgbWriteEnable ?
			renderTarget.m_texture->GetMetalTexture() :
			renderTarget.m_texture->GetSRGBViewMetalTexture();
	}

	m_workQueue->SetRenderAttachments( renderTargetTexture, slot, slice );

	m_boundRenderTargets[slot] = { renderTarget, slice };

	if( m_boundRenderTargets[0].texture.IsValid() )
	{
		SetViewport( Tr2Viewport( m_boundRenderTargets[0].texture.GetWidth(), m_boundRenderTargets[0].texture.GetHeight() ) );
	}

	return S_OK;
}

ALResult Tr2RenderContextAL::CreateDevice(
	uint32_t Adapter,
	Tr2WindowHandle  hFocusWindow,
	const Tr2PresentParametersAL& presentationParameters )
{
	m_isValid = true;
	m_workQueue = m_metalContext->GetPrimaryWorkQueue();
#if 0
	NSView *view = (NSView *)hFocusWindow;
#else
	NSView *view = (NSView *)presentationParameters.outputWindow;
#endif
	m_caMetalLayer = (CAMetalLayer *)view.layer;
	METAL_LOG(@"Creating device");
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

ALResult Tr2RenderContextAL::SetPresentParameters( unsigned adapter, const Tr2PresentParametersAL& presentationParameters )
{
	TrinityALImpl::MetalContext *metalContext = GetMetalContext();
#if 0
	m_caMetalLayer.device               = metalContext->GetDevice();
	m_caMetalLayer.pixelFormat          = metalContext->m_utils->GetMTLPixelFormat(PIXEL_FORMAT_B8G8R8A8_UNORM);
	// This will assert if the value is not 2 or 3
	m_caMetalLayer.maximumDrawableCount = presentationParameters.backBufferCount;
	m_caMetalLayer.drawableSize         = CGSizeMake(presentationParameters.mode.width, presentationParameters.mode.height);
	m_caMetalLayer.framebufferOnly      = false;
	m_caMetalLayer.wantsExtendedDynamicRangeContent = false;
	id<CAMetalDrawable> drawable = m_caMetalLayer.nextDrawable;
	id<MTLTexture> drawableTexture = drawable.texture;
#endif

	m_presentParameters = presentationParameters;
	if( m_presentParameters.mode.format == PIXEL_FORMAT_UNKNOWN )
	{
		m_presentParameters.mode.format = PIXEL_FORMAT_B8G8R8A8_UNORM;
	}
	m_swapChain.m_swapChain->Create( presentationParameters.outputWindow, *this );
	
	NSView* view = (NSView*)presentationParameters.outputWindow;
	if( view.layer && [view.layer isKindOfClass:CAMetalLayer.class] )
	{
		((CAMetalLayer*)view.layer).displaySyncEnabled = presentationParameters.presentInterval == Tr2RenderContextEnum::PRESENT_INTERVAL_IMMEDIATE ? NO : YES;
	}
	m_defaultBackBuffer = m_swapChain.GetBackBuffer();

	// Set a default viewport based around this 
	m_workQueue->SetViewport(
		0.0,
		0.0,
		m_defaultBackBuffer.GetWidth(),
		m_defaultBackBuffer.GetHeight(),
		0.0,
		1.0);

	ResetRenderTargets();
	SetRenderTarget( m_defaultBackBuffer ); 
	Clear( Tr2RenderContextEnum::CLEARFLAGS_TARGET, 0, 0, 0 );

	return S_OK;
}

const Tr2CapsAL& Tr2RenderContextAL::GetCaps() const
{
	return m_caps;
}

ALResult Tr2RenderContextAL::BeginScene()
{
	m_workQueue->BeginFrame();
	ResetRenderTargets();
	return S_OK;
}

ALResult Tr2RenderContextAL::EndScene()
{
	m_workQueue->EndFrame();

	m_vertexLayout = Tr2VertexLayoutAL();
	m_resourceSet = Tr2ResourceSetAL();
	m_shaderProgram = Tr2ShaderProgramAL();
	m_needsDrawResourceCheck = true;

	return S_OK;
}

ALResult Tr2RenderContextAL::Present()
{
	m_swapChain.Present( *this );
	m_defaultBackBuffer = m_swapChain.GetBackBuffer();

	ResetRenderTargets();

	return S_OK;
}

bool Tr2RenderContextAL::IsValid()
{
	return m_isValid;
}

ALResult Tr2RenderContextAL::SetVertexLayout( const Tr2VertexLayoutAL& layout )
{
	m_vertexLayout = layout;

	return S_OK;
}

ALResult Tr2RenderContextAL::SetShaderProgram( const Tr2ShaderProgramAL& shaderProgram )
{
	if( m_shaderProgram == shaderProgram )
	{
        return S_OK;
    }

	m_shaderProgram = shaderProgram;

	// Move this into shaderProgram itself to remove need for getters.
	m_workQueue->SetShaders(
		shaderProgram.m_program->GetVertexShader(),
		shaderProgram.m_program->GetFragmentShader(),
		shaderProgram.m_program->GetComputeKernel(),
		shaderProgram.m_program->GetThreadGroupSize(),
		shaderProgram.m_program->GetResourceMasks() );

    m_needsDrawResourceCheck = true;

	return S_OK;
}

//
// JM - This implementation may incur some overhead due to the amount of conditional checking
// the metal context functions do on the supplied pointers (see stencil setting). Eventually
// we'll remove the actual setting here and just store the values in some state and we'll do
// a wholesale setting of state in some combined function just before we draw.
//
ALResult Tr2RenderContextAL::SetRenderState( RenderState state, uint32_t value )
{
	TrinityALImpl::MetalContext *metalContext = GetMetalContext();

	switch( state )
	{
	case RS_ZENABLE:
	{
		// You don't enable/disable depth test explicitly in Metal you just set the compare mode.
		if( !value )
		{
			m_depthCompareEnabled = false;
			m_workQueue->SetDepthCompareFn( MTLCompareFunctionAlways );
		}
		else
		{
			m_depthCompareEnabled = true;
			m_workQueue->SetDepthCompareFn( m_depthCompareFunction );
		}
		return S_OK;
	}
	case RS_ZWRITEENABLE:
	{
		m_depthWriteEnabled = value ? true : false;
		m_workQueue->SetDepthState( m_depthWriteEnabled && !m_readOnlyDepth );
		return S_OK;
	}
	case RS_ZFUNC:
	{
		// Need to store this as depth test can be enabled/disabled (which Metal has no analog for - you just set the compare mode).
		m_depthCompareFunction = metalContext->m_utils->GetMTLCompareFunction( value );
		if( !m_depthCompareEnabled )
		{
			m_workQueue->SetDepthCompareFn( MTLCompareFunctionAlways );
		}
		else
		{
			m_workQueue->SetDepthCompareFn( m_depthCompareFunction );
		}

		return S_OK;
	}
	case RS_ALPHATESTENABLE:
	case RS_ALPHAREF:
	case RS_ALPHAFUNC:
        return S_OK;
	case RS_CLIPPING:
	case RS_CLIPPLANEENABLE:
		return S_OK;
	case RS_SRCBLEND:
	{
		MTLBlendFactor srcBlend = metalContext->m_utils->GetMTLBlendFactor( value );
		m_workQueue->SetSrcBlend( 0, srcBlend );
		return S_OK;
	}
	case RS_DESTBLEND:
	{
		MTLBlendFactor destBlend = metalContext->m_utils->GetMTLBlendFactor( value );
		m_workQueue->SetDestBlend( 0, destBlend );
		return S_OK;
	}
	case RS_SRCBLENDALPHA:
	{
		MTLBlendFactor srcBlend = metalContext->m_utils->GetMTLBlendFactor( value );
		m_workQueue->SetSrcBlendAlpha( 0, srcBlend );
		return S_OK;
	}
	case RS_DESTBLENDALPHA:
	{
		MTLBlendFactor destBlend = metalContext->m_utils->GetMTLBlendFactor( value );
		m_workQueue->SetDestBlendAlpha( 0, destBlend );
		return S_OK;
	}
	case RS_BLENDOP:
	{
		MTLBlendOperation blendOp = metalContext->m_utils->GetMTLBlendOperation( value );
		m_workQueue->SetBlendOp( 0, blendOp );
		return S_OK;
	}
	case RS_BLENDOPALPHA:
	{
		MTLBlendOperation blendOp = metalContext->m_utils->GetMTLBlendOperation( value );
		m_workQueue->SetBlendOpAlpha( 0, blendOp );
		return S_OK;
	}
	case RS_CULLMODE:
	{
		MTLCullMode cullMode = metalContext->m_utils->GetMTLCullMode( (Tr2RenderContextEnum::CullMode)value );
		m_workQueue->SetCullMode( cullMode );
		return S_OK;
	}
	case RS_ALPHABLENDENABLE:
	{
		TrinityALImpl::MetalBlendType blendType = value ? ( m_separateAlphaBlendEnabled ? TrinityALImpl::METAL_BLENDING_ENABLED_SEPARATE_ALPHA : TrinityALImpl::METAL_BLENDING_ENABLED ) : TrinityALImpl::METAL_BLENDING_DISABLED;
		m_alphaBlendEnable = value != 0;
		m_workQueue->SetBlendType( 0, blendType );
		return S_OK;
	}
	case RS_STENCILENABLE:
	{
		bool stencilEnable = value ? true : false;
		m_workQueue->SetStencilState( stencilEnable );
		return S_OK;
	}
	case RS_STENCILFAIL:
	{
		MTLStencilOperation stencilOp = metalContext->m_utils->GetMTLStencilOperation( value );
		m_workQueue->SetStencilFailOp( stencilOp );
		return S_OK;
	}
	case RS_STENCILZFAIL:
	{
		MTLStencilOperation stencilOp = metalContext->m_utils->GetMTLStencilOperation( value );
		m_workQueue->SetStencilDepthFailOp( stencilOp );
		return S_OK;
	}
	case RS_STENCILPASS:
	{
		MTLStencilOperation stencilOp = metalContext->m_utils->GetMTLStencilOperation( value );
		m_workQueue->SetStencilPassOp( stencilOp );
		return S_OK;
	}
	case RS_STENCILFUNC:
	{
		MTLCompareFunction stencilCompareFn = metalContext->m_utils->GetMTLCompareFunction( value );
		m_workQueue->SetStencilCompareFn( stencilCompareFn );
		return S_OK;
	}
	case RS_STENCILREF:
	{
		m_workQueue->SetStencilRefValue( value );
		return S_OK;
	}
	case RS_STENCILMASK:
	{
		m_workQueue->SetStencilMask( value );
		return S_OK;
	}
	case RS_COLORWRITEENABLE:
	{
		MTLColorWriteMask writeMask = metalContext->m_utils->GetMTLColorWriteMask( (ColorWriteEnable)value );
		m_workQueue->SetColorWriteMask( 0, writeMask );
		return S_OK;
	}
	case RS_SLOPESCALEDEPTHBIAS:
	{
		float slopeScale = *(float *)&value;
		m_workQueue->SetDepthBias( nil, &slopeScale, nil );
		return S_OK;
	}
	case RS_BLENDFACTOR:
	{
		m_workQueue->SetBlendColor( 0, value );
		return S_OK;
	}
	case RS_DEPTHBIAS:
	case RS_ZBIAS:
	{
		float depthBias = *(float *)&value;
		m_workQueue->SetDepthBias( &depthBias, nil, nil );
		return S_OK;
	}
	case RS_SEPARATEALPHABLENDENABLE:
	{
		m_separateAlphaBlendEnabled = value != 0;
		TrinityALImpl::MetalBlendType blendType = m_alphaBlendEnable ? ( value ? TrinityALImpl::METAL_BLENDING_ENABLED_SEPARATE_ALPHA : TrinityALImpl::METAL_BLENDING_ENABLED ) : TrinityALImpl::METAL_BLENDING_DISABLED;
		m_workQueue->SetBlendType( 0, blendType );
		return S_OK;
	}
	case RS_SRGBWRITEENABLE:
		m_srgbWriteEnable = value;

		if( value )
		{
			for( unsigned i = 0; i != MAX_RENDER_TARGET; ++i )
			{
				if( m_boundRenderTargets[i].texture.IsValid() )
				{
					id<MTLTexture> renderTargetTexture = m_boundRenderTargets[i].texture.m_texture->GetSRGBViewMetalTexture();
					m_workQueue->SetRenderAttachments( renderTargetTexture, i, m_boundRenderTargets[i].slice );
				}
			}
		}
		else
		{
			for( unsigned i = 0; i != MAX_RENDER_TARGET; ++i )
			{
				if( m_boundRenderTargets[i].texture.IsValid() )
				{
					id<MTLTexture> renderTargetTexture = m_boundRenderTargets[i].texture.m_texture->GetMetalTexture();
					m_workQueue->SetRenderAttachments( renderTargetTexture, i, m_boundRenderTargets[i].slice );
				}
			}
		}
		return S_OK;
    case RS_FILLMODE:
        switch( value )
        {
        case FM_SOLID:
            m_workQueue->SetFillMode( MTLTriangleFillModeFill );
            return S_OK;
        case FM_WIREFRAME:
            m_workQueue->SetFillMode( MTLTriangleFillModeLines );
            return S_OK;
        default:
            return E_INVALIDARG;
        }
	default:
		return E_INVALIDARG;
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::SetRenderStates( const uint32_t* stateValuePairs, uint32_t count )
{
	for( uint32_t i = 0;
		 ( count != 0 && i != count ) || ( count == 0 && *stateValuePairs );
		 ++i, stateValuePairs += 2 )
	{
		SetRenderState( static_cast<RenderState>( stateValuePairs[0] ), stateValuePairs[1] );
	}
	return S_OK;
}

ALResult Tr2RenderContextAL::SetResourceSet( const Tr2ResourceSetAL& resourceSet )
{
#if 0
	if( m_resourceSet.m_resourceSet == resourceSet.m_resourceSet )
	{
        return S_OK;
    }
#endif

	TrinityALImpl::MetalContext* metalContext = GetMetalContext();

	m_resourceSet = resourceSet;
	auto& rs = *resourceSet.m_resourceSet;

	if( rs.IsValid() )
	{
		const ShaderType stages[] = { VERTEX_SHADER, PIXEL_SHADER, COMPUTE_SHADER };
		for( auto stage : stages )
		{
			m_workQueue->SetBuffers( stage, rs.m_buffers[stage], rs.m_buffersMask[stage] );
			m_workQueue->SetTextures( stage, rs.m_textures[stage], rs.m_texturesRange[stage] );
			m_workQueue->SetSamplers( stage, rs.m_samplers[stage], rs.m_samplersRange[stage] );
		}
	}
	else
	{
		const ShaderType stages[] = { VERTEX_SHADER, PIXEL_SHADER, COMPUTE_SHADER };
		for( auto stage : stages )
		{
			m_workQueue->ResetBuffers( stage );
			m_workQueue->ResetTextures( stage );
			m_workQueue->ResetSamplers( stage );
		}
	}

    m_needsDrawResourceCheck = true;
	return S_OK;
}

ALResult Tr2RenderContextAL::SetViewport( const Tr2Viewport& viewport )
{
	m_viewport = viewport;
	TrinityALImpl::MetalContext* metalContext = GetMetalContext();
	m_workQueue->SetViewport( viewport.m_x, viewport.m_y, viewport.m_width, viewport.m_height, viewport.m_minZ, viewport.m_maxZ );
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

	m_stackRT[slot].push( m_boundRenderTargets[slot] );
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

	auto rt = m_stackRT[slot].top();
	m_stackRT[slot].pop();

	return SetRenderTarget( rt.texture, slot, rt.slice );
}

ALResult Tr2RenderContextAL::PushDepthStencil()
{
	m_stackDS.push( m_boundDepthStencil );
	return S_OK;
}

ALResult Tr2RenderContextAL::PopDepthStencil()
{
	CCP_ASSERT( !m_stackDS.empty() );

	if( m_stackDS.empty() )
	{
		return E_FAIL;
	}

	auto ds = m_stackDS.top();
	m_stackDS.pop();

	// Popping implicitly sets the current depth stencil attachment attachment to this one
	return SetDepthStencil( ds );
}

ALResult Tr2RenderContextAL::GetRenderTargetSize( uint32_t& width, uint32_t& height, uint32_t slot )
{
	if( slot >= MAX_RENDER_TARGET )
	{
		return E_FAIL;
	}
	if( !m_boundRenderTargets[slot].texture.IsValid() )
	{
		return E_INVALIDCALL;
	}
	else
	{
		width  = m_boundRenderTargets[slot].texture.GetWidth();
		height = m_boundRenderTargets[slot].texture.GetHeight();
	}
	return S_OK;
}

void Tr2RenderContextAL::ResetRenderTargets()
{
	Tr2TextureAL nullTex;
	SetDepthStencil( nullTex );
	//ResourceBarrierDx12( TrinityALImpl::Transition( m_defaultBackBuffer.m_texture->GetResourceDx12(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET ) );

	// Setting a default render target here means we will get unoptimal depth only ed
	SetRenderTarget( m_defaultBackBuffer );
	for( uint32_t i = 1; i < METAL_MAX_RENDER_TARGETS; ++i )
	{
		SetRenderTarget( nullTex, i );
	}
}

void Tr2RenderContextAL::ReleaseDeviceResources()
{
	for( unsigned i = 0; i != MAX_RENDER_TARGET; ++i )
	{
		m_boundRenderTargets[i] = {};
	}
	m_boundDepthStencil = Tr2TextureAL();

	m_defaultBackBuffer = Tr2TextureAL();
}

void Tr2RenderContextAL::AddGpuMarker( const char* )
{
}

void Tr2RenderContextAL::PushGpuMarker( const char* name )
{
	m_workQueue->PushDebugGroup( name );
}

void Tr2RenderContextAL::PopGpuMarker()
{
	m_workQueue->PopDebugGroup();
}

ALResult Tr2RenderContextAL::GetGpuStateMarker( Tr2RenderContextEnum::RenderContextStatus&, std::string& ) const
{
	return E_FAIL;
}

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

void Tr2RenderContextAL::RenderPassHint( const Tr2ColorAttachment& rt0, const Tr2DepthAttachment& depth )
{
	TrinityALImpl::MetalRenderPassHint hint;
	hint.depth = { MTLLoadAction( depth.load ), MTLStoreAction( depth.store ), depth.clearValue };
	hint.color[0] = { MTLLoadAction( rt0.load ), MTLStoreAction( rt0.store ), MakeClearColor( rt0.clearColor ) };
	for( int i = 1; i < METAL_MAX_RENDER_TARGETS; ++i )
	{
		hint.color[i] = { MTLLoadActionDontCare, MTLStoreActionDontCare };
	}
	m_workQueue->RenderPassHint( hint );
}

void Tr2RenderContextAL::RenderPassHint( const Tr2ColorAttachment& rt0, const Tr2ColorAttachment& rt1, const Tr2DepthAttachment& depth )
{
	TrinityALImpl::MetalRenderPassHint hint;
	hint.depth = { MTLLoadAction( depth.load ), MTLStoreAction( depth.store ), depth.clearValue };
	hint.color[0] = { MTLLoadAction( rt0.load ), MTLStoreAction( rt0.store ), MakeClearColor( rt0.clearColor ) };
	hint.color[1] = { MTLLoadAction( rt1.load ), MTLStoreAction( rt1.store ), MakeClearColor( rt1.clearColor ) };
	for( int i = 2; i < METAL_MAX_RENDER_TARGETS; ++i )
	{
		hint.color[i] = { MTLLoadActionDontCare, MTLStoreActionDontCare };
	}
	m_workQueue->RenderPassHint( hint );

}

void Tr2RenderContextAL::EndRenderPassHint()
{
    m_workQueue->EndRenderPassHint();
}

uint32_t Tr2RenderContextAL::BeginParallelEncoding( uint32_t requestedEncodersCount )
{
	if( !m_isValid || !m_isPrimary )
	{
		CCP_ASSERT( false );
		return 0;
	}
	if( !m_workQueue->CanBeginParallelEncoding() )
	{
		return 0;
	}
	return GetMetalContext()->BeginParallelEncoding( requestedEncodersCount );
}

void Tr2RenderContextAL::EndParallelEncoding()
{
	GetMetalContext()->EndParallelEncoding();
}

ALResult Tr2RenderContextAL::ForkContext( Tr2RenderContextAL* context, uint32_t index ) const
{
	context->m_workQueue = m_metalContext->GetSecondaryWorkQueue( index );
	if( !context->m_workQueue )
	{
		return E_INVALIDCALL;
	}
	
	context->m_isValid = m_isValid;
	context->m_isPrimary = false;

	context->m_viewport = m_viewport;
	context->m_metalContext = m_metalContext;

	context->m_depthWriteEnabled = m_depthWriteEnabled;
	context->m_depthCompareEnabled = m_depthCompareEnabled;
	context->m_readOnlyDepth = m_readOnlyDepth;
	context->m_srgbWriteEnable = m_srgbWriteEnable;
	context->m_alphaBlendEnable = m_alphaBlendEnable;
	context->m_separateAlphaBlendEnabled = m_separateAlphaBlendEnabled;
	
	context->m_metalPrimitiveInfo = m_metalPrimitiveInfo;
	context->m_metalIndexType = m_metalIndexType;
	context->m_depthCompareFunction = m_depthCompareFunction;
	
	context->m_vertexLayout = Tr2VertexLayoutAL();
	context->m_resourceSet = Tr2ResourceSetAL();
	context->m_shaderProgram = Tr2ShaderProgramAL();
	context->m_needsDrawResourceCheck = true;

	return S_OK;
}

#endif
