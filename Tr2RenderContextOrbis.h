#pragma once
#ifndef Tr2RenderContextOrbis_h_
#define Tr2RenderContextOrbis_h_

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2RenderContextEnum.h"
#include "Tr2DrawUPHelper.h"
#include "Tr2FragmentOpSettings.h"
#include "Tr2CapsALOrbis.h"
#include <gnmx/gfxcontext.h>

struct ITr2RenderContextEvents;

// -------------------------------------------------------------
// Description:
//   See http://carbon/wiki/Tr2RenderContext
// -------------------------------------------------------------
class Tr2RenderContextAL
{
public:
    Tr2RenderContextAL();
	~Tr2RenderContextAL();
	void Destroy();

	static void SetPrimaryRenderContext( Tr2PrimaryRenderContextAL* );
	static Tr2PrimaryRenderContextAL& GetPrimaryRenderContext();

	ALResult CreateDevice(			
		uint32_t Adapter, 
		Tr2WindowHandle hFocusWindow, 
		const Tr2PresentParametersAL& presentationParameters );
	ALResult CreateSecondaryContext() { return E_FAIL; }
	ALResult SetPresentParameters( unsigned adapter, const Tr2PresentParametersAL& presentationParameters );

	const Tr2CapsAL& GetCaps() const;
	
	bool IsValid() const;

	ALResult Clear(						
		uint32_t clearFlags, 
		uint32_t color, 
		float depth, 
		uint32_t stencil = 0 );

	ALResult BeginScene();
	ALResult EndScene();
	ALResult Present();
	ALResult FinishCommandList() { return E_FAIL; }
	ALResult ExecuteCommandList() { return E_FAIL; }
	void ReleaseDeviceResources() {}

	ALResult SetStreamSource(	
		uint32_t stream, 
		const Tr2VertexBufferAL & buffer, 
		uint32_t offset, 
		uint32_t stride );
	ALResult SetVertexLayout( Tr2VertexLayoutAL& layout );
	ALResult SetShader( const Tr2ShaderAL& shader );
	ALResult SetTopology( long topology );
	ALResult SetIndices( const Tr2IndexBufferAL& buffer );
	ALResult SetConstants(			
		const Tr2ConstantBufferAL& buffer, 
		Tr2RenderContextEnum::ShaderType constantType, 
		uint32_t registerIndex, 
		uint32_t unusedArgument = 0 );
	ALResult SetTexture(				
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t slot, 
		const Tr2TextureAL& texture, 
		Tr2RenderContextEnum::ColorSpace colorSpace = Tr2RenderContextEnum::COLOR_SPACE_LINEAR );
	ALResult SetSamplerState(		
		const Tr2SamplerStateAL& samplerState, 
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t registerNumber );
	ALResult SetRenderState( Tr2RenderContextEnum::RenderState state, uint32_t value );
	ALResult SetRenderStates( const uint32_t* stateValuePairs, uint32_t count );
	ALResult GetRenderState( Tr2RenderContextEnum::RenderState state, uint32_t* value );

	ALResult SetClipPlane( uint32_t planeIndex, const float* planeEq );

	ALResult SetScissorRect(
		uint32_t left, 
		uint32_t top, 
		uint32_t right, 
		uint32_t bottom );

	ALResult SetViewport( const Tr2Viewport& viewport );
	ALResult GetViewport( Tr2Viewport& viewport );

	Tr2RenderContextEnum::PixelFormat GetBackBufferFormat() const;

	ALResult GetAFRGroupCount( uint32_t& count );
	
	static const uint32_t SHADER_TYPE_MASK = 
		( 1 << Tr2RenderContextEnum::VERTEX_SHADER ) |
		( 1 << Tr2RenderContextEnum::PIXEL_SHADER );

	// Debug helpers
	size_t GetStackSizeRT( uint32_t RT = 0 )	const { return m_stackRT[RT].size(); }
	size_t GetStackSizeDS()						const { return m_stackDS    .size(); }

	ALResult PushRenderTarget( uint32_t slot = 0 );
	ALResult PopRenderTarget( uint32_t slot = 0 );
	ALResult SetRenderTarget( const Tr2RenderTargetAL& renderTarget, uint32_t slot = 0 );

	ALResult PushDepthStencil();
	ALResult PopDepthStencil();
	ALResult SetDepthStencil( const Tr2DepthStencilAL& depthStencil );
	void SetReadOnlyDepth( bool enable ) { }

	ALResult SetNumberOfLights( uint32_t numLights );

	Tr2RenderTargetAL& GetDefaultBackBuffer();

	ALResult GetRenderTargetSize(	
		uint32_t& width, 
		uint32_t& height, 
		uint32_t slot = 0 );

	bool IsSupportedRenderTargetFormat( 
		Tr2RenderContextEnum::PixelFormat format, 
		bool withAutoGenMipmap = false );

	ALResult DrawPrimitive(	uint32_t startVertex, uint32_t primitiveCount );
	ALResult DrawIndexedPrimitive(	
		uint32_t numVertices, 
		uint32_t startIndex, 
		uint32_t primitiveCount, 
		uint32_t minimumIndex = 0 );
	ALResult DrawIndexedInstanced(	
		uint32_t numVertices, 
		uint32_t startIndex, 
		uint32_t primitiveCount, 
		uint32_t numInstances );

	ALResult DrawIndexedPrimitiveUP( 
		uint32_t numVertices, 
		uint32_t primitiveCount, 
		const uint32_t* indexData, 
		const void* vertexStreamZeroData, 
		uint32_t vertexStreamZeroStride);

	ALResult DrawIndexedPrimitiveUP( 
		uint32_t numVertices, 
		uint32_t primitiveCount, 
		const uint16_t* indexData, 
		const void* vertexStreamZeroData, 
		uint32_t vertexStreamZeroStride);

	ALResult DrawPrimitiveUP(		
		uint32_t primitiveCount, 
		const void* vertexStreamZeroData, 
		uint32_t vertexStreamZeroStride );

	ALResult DrawIndexedInstancedIndirect( Tr2GpuBufferAL& params, uint32_t offset )
	{
		return E_FAIL;
	}
	
	ALResult RunComputeShader( unsigned groupDimX, unsigned groupDimY, unsigned groupDimZ )
	{
		return E_FAIL;
	}
	ALResult RunComputeShaderIndirect( Tr2GpuBufferAL& indirectParams, unsigned offset )
	{
		return E_FAIL;
	}

	ALResult SetShaderBuffer(		
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t slot, 
		const Tr2GpuBufferAL& buffer )
	{ 
		return E_FAIL; 
	}

	ALResult SetUav(
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t slot, 
		const Tr2GpuBufferAL& buffer ) throw()
	{ 
		return E_FAIL; 
	}

	ALResult SetUav(				
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t slot, 
		Tr2TextureAL& texture ) throw()
	{ 
		return E_FAIL; 
	}

	ALResult ClearUav( Tr2GpuBufferAL& buffer, const float values[4] ) throw()
	{
		return E_FAIL;
	}

	ALResult ClearUav( Tr2GpuBufferAL& buffer, const uint32_t values[4] ) throw()
	{
		return E_FAIL;
	}

	static bool ConvertPixelFormatToDataFormat( Tr2RenderContextEnum::PixelFormat format, sce::Gnm::DataFormat& result );
public:
	uint32_t InternalGetCurentFrameIndex() const;
	uint32_t InternalGetMaxFrameLatency() const;
	static void InternalDelayDelete( uint32_t frameUsed, void* pointer );
	void InternalSyncToGpu();
	uint32_t InternalGetBorderColorIndex( const float* color );
	ALResult InternalResolveRT( Tr2RenderTargetAL* destination, Tr2RenderTargetAL* source );
	sce::Gnmx::GfxContext& InternalGetGfxContext();
	uint32_t ComputeVertexCount( uint32_t primitiveCount ) const;
	void InternalSyncRenderTarget( Tr2RenderTargetAL& rt );
	ALResult InternalGenerateMips( Tr2TextureAL& rt );
	ALResult InternalCopySubresourceRegion( 
		Tr2RenderTargetAL& destination, 
		uint32_t destX, 
		uint32_t destY, 
		Tr2RenderTargetAL& source, 
		uint32_t* ltrb );

	ITr2RenderContextEvents* m_events;

private:
	struct DisplayBuffer
	{
		sce::Gnmx::GfxContext context;
		void *cpRamShadow;
		void *cueHeap;
		void *dcbBuffer;
		void *ccbBuffer;
		sce::Gnm::RenderTarget renderTarget;
		sce::Gnm::DepthRenderTarget depthTarget;
		uint32_t m_expectedLabel;
	};
	struct Stream
	{
		const Tr2VertexBufferAL* m_buffer;
		uint32_t m_stride;
	};
	struct DelayDeleteResource
	{
		uint32_t frame;
		void* pointer;
	};
	struct SharedConstantBuffer
	{
		Tr2ConstantBufferAL constantBuffer;
		CcpMallocBuffer mirror;
		size_t size;
	};

	ALResult DoBeginFrame();
	ALResult ApplyShadowState( Tr2VertexLayoutAL::FetchShaderType fetchType );
	void ResizeDrawPrimitiveIndexBuffer( uint32_t vertexCount );
	static Tr2VertexLayoutAL::FetchShader GetFetchShader( const Tr2VertexLayoutAL& layout, const Tr2ShaderAL& vertexShader, Tr2VertexLayoutAL::FetchShaderType type );
	static Tr2VertexLayoutAL::FetchShader CreateFetchShader( const Tr2VertexLayoutAL& layout, const Tr2ShaderAL& vertexShader, Tr2VertexLayoutAL::FetchShaderType type );

	void SyncRenderTarget( uint32_t slot );
	void SetRenderTarget( uint32_t slot );
	void SetDepthStencil();
	void ClearMemoryWithCompute( void* memory, size_t sizeInUint32s, uint32_t* source, size_t sourceCount );

	static const uint32_t BUFFER_COUNT = 2;
	DisplayBuffer m_buffers[BUFFER_COUNT];
	volatile uint64_t* m_labels;
	DisplayBuffer* m_backBuffer;
	int32_t m_registeredBuffers;
	uint32_t m_backBufferIndex;
	SceKernelEqueue m_flipEventQueue;
	uint32_t m_frameIndex;
	std::vector<DelayDeleteResource> m_delayDeletes;

	Tr2ShaderAL m_clearShader;

	static const unsigned STREAM_COUNT = 8;
	Stream m_streams[STREAM_COUNT];
	const Tr2VertexLayoutAL* m_layout;
	const Tr2ShaderAL* m_boundShaders[Tr2RenderContextEnum::SHADER_TYPE_COUNT];
	const Tr2IndexBufferAL* m_indices;
	static const uint32_t TEX_SLOT_COUNT = 16;
	struct BoundTexture
	{
		const Tr2TextureAL* texture;
		Tr2RenderContextEnum::ColorSpace colorSpace;
	};
	BoundTexture m_boundTextures[Tr2RenderContextEnum::SHADER_TYPE_COUNT * TEX_SLOT_COUNT];

	uint16_t* m_drawPrimitiveIndexBuffer;
	uint32_t m_drawPrimitiveIndexCount;

	Tr2RenderContextEnum::Topology m_topology;

	static const uint32_t CB_SLOT_COUNT = sce::Gnm::kSlotCountConstantBuffer;
	SharedConstantBuffer m_sharedConstantBuffers[Tr2RenderContextEnum::SHADER_TYPE_COUNT * CB_SLOT_COUNT];

	static const uint32_t MAX_BORDER_COLORS = 16;
	uint32_t m_borderColorCount;
	float* m_borderColorColors;

	uint32_t m_renderStates[Tr2RenderContextEnum::RS_MAX_STATE];

	sce::Gnm::DepthStencilControl m_depthStencilControl;
	bool m_depthStencilControlDirty;

	sce::Gnm::PrimitiveSetup m_primitiveSetup;
	bool m_primitiveSetupDirty;

	sce::Gnm::BlendControl m_blendControl;
	bool m_blendControlDirty;

	sce::Gnm::StencilOpControl m_stencilOpControl;
	bool m_stencilOpControlDirty;

	sce::Gnm::StencilControl m_stencilControl;
	bool m_stencilControlDirty;

	sce::Gnm::ClipControl m_clipControl;
	bool m_clipControlDirty;

	sce::Gnm::ScanModeControlAa m_msaaEnabled;
	sce::Gnm::ScanModeControlViewportScissor m_scissorEnabled;
	bool m_scanModeControlDirty;

	float m_polygonOffsetScale;
	float m_polygonOffset;
	bool m_polygonOffsetDirty;

	Tr2DrawUPHelper	m_drawUP;


	// TODO: use me
	bool m_linearRenderTarget;

	enum { MAX_RENDER_TARGET = 8 };
	const Tr2RenderTargetAL* m_boundRenderTarget[MAX_RENDER_TARGET];
	const Tr2DepthStencilAL* m_boundDepthStencil;
	TrackableStdStack<const Tr2RenderTargetAL*>	m_stackRT[MAX_RENDER_TARGET];
	TrackableStdStack<const Tr2DepthStencilAL*>	m_stackDS;

	Tr2ShaderAL m_quadVS;
	Tr2ShaderAL m_dummyPS;
	Tr2ShaderAL m_downsampleCS;
	Tr2ShaderAL m_copyCS;

	Tr2Viewport m_viewport;
	Tr2RenderTargetAL m_backBufferRT;

	Tr2FragmentOpSettings::TAlphaTestParameters	m_alphaTestParameters;
	Tr2FragmentOpSettings m_fragmentOpSettings;
	Tr2ConstantBufferAL	m_fragmentOpBuffer;
	uint32_t m_dirtyFlag;

	Tr2TextureAL m_emptyTexture;
	Tr2CapsAL m_caps;
};

#endif

#endif
