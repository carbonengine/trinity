#pragma once
#ifndef Tr2RenderContextDx11_h_
#define Tr2RenderContextDx11_h_

#include "Tr2RenderContextEnum.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

struct ID3D11DeviceContext;
class Tr2VertexBufferAL;
class Tr2IndexBufferAL;
class Tr2ConstantBufferAL;
class Tr2HalDevice;
class Tr2VertexLayoutAL;
class Tr2TextureAL;
struct ITr2RenderContextEvents;

#include "Tr2RenderStateEmulationDx11.h"
#include "Tr2DrawUPHelper.h"

// -------------------------------------------------------------
// Description:
//   See http://carbon/wiki/Tr2RenderContext
// -------------------------------------------------------------
class Tr2RenderContextAL
{
public:
	Tr2RenderContextAL() throw();
	~Tr2RenderContextAL() throw();
	void Destroy() throw();
	ALResult CreateSecondaryContext() throw();

	static void SetPrimaryRenderContext( Tr2PrimaryRenderContextAL* );
	static Tr2PrimaryRenderContextAL& GetPrimaryRenderContext();
	static Tr2PrimaryRenderContextAL* GetPrimaryRenderContextPointer();

	ALResult BeginScene() throw();
	ALResult EndScene()   { return S_OK; }
	ALResult FinishCommandList() throw();
	ALResult ExecuteCommandList() throw();
	
	bool IsValid() const throw();

	void ReleaseDeviceResources() throw();

	ALResult SetStreamSource(
		uint32_t stream, 
		const Tr2VertexBufferAL & buffer, 
		uint32_t offset, 
		uint32_t stride ) throw();

	ALResult SetIndices( const Tr2IndexBufferAL & buffer ) throw();
	ALResult SetTopology( Tr2RenderContextEnum::Topology topology ) throw();
	ALResult SetVertexLayout( const Tr2VertexLayoutAL& layout ) throw();
	ALResult SetShader( const Tr2ShaderAL& shader ) throw();

	ALResult SetShaderBuffer( 
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t slot, 
		const Tr2GpuBufferAL& buffer ) throw();

	ALResult SetUav(
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t slot, 
		const Tr2GpuBufferAL& buffer,
		uint32_t initialCount = -1 ) throw();

	ALResult SetUav(
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t slot, 
		Tr2TextureAL& texture ) throw();

	ALResult ClearUav( Tr2GpuBufferAL& buffer, const float values[4] ) throw();
	ALResult ClearUav( Tr2GpuBufferAL& buffer, const uint32_t values[4] ) throw();

	ALResult SetTexture(				
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t slot, 
		const Tr2TextureAL& texture, 
		Tr2RenderContextEnum::ColorSpace colorSpace = Tr2RenderContextEnum::COLOR_SPACE_LINEAR ) throw();
	
	ALResult DrawIndexedPrimitive(	
		uint32_t numVertices, 
		uint32_t startIndex, 
		uint32_t primitiveCount, 
		uint32_t minimumIndex = 0 ) throw();

	ALResult DrawPrimitive(uint32_t startVertex, uint32_t primitiveCount ) throw();

	ALResult DrawIndexedInstanced(	
		uint32_t numVertices, 
		uint32_t startIndex, 
		uint32_t primitiveCount, 
		uint32_t numInstances ) throw();
	
	ALResult DrawIndexedInstancedIndirect( Tr2GpuBufferAL& params, uint32_t offset ) throw();
	ALResult DrawInstancedIndirect( Tr2GpuBufferAL& params, uint32_t offset ) throw();
	
	ALResult DrawIndexedPrimitiveUP( 
		uint32_t numVertices, 
		uint32_t primitiveCount, 
		const uint32_t* indexData, 
		const void* vertexStreamZeroData, 
		uint32_t vertexStreamZeroStride ) throw();

	ALResult DrawIndexedPrimitiveUP( 
		uint32_t numVertices, 
		uint32_t primitiveCount, 
		const uint16_t* indexData, 
		const void* vertexStreamZeroData, 
		uint32_t vertexStreamZeroStride) throw();

	ALResult DrawPrimitiveUP(		
		uint32_t primitiveCount, 
		const void* vertexStreamZeroData, 
		uint32_t VertexStreamZeroStride ) throw();

	ALResult RunComputeShader( unsigned groupDimX, unsigned groupDimY, unsigned groupDimZ ) throw();
	ALResult RunComputeShaderIndirect( Tr2GpuBufferAL& indirectParams, unsigned offset ) throw();

	ALResult CopyBufferCounter( Tr2GpuBufferAL& dest, uint32_t destOffset, Tr2GpuBufferAL& src ) throw();

	ALResult SetRenderState( Tr2RenderContextEnum::RenderState state, uint32_t value ) throw();

	ALResult SetRenderStates( const uint32_t* stateValuePairs, uint32_t count ) throw();
	ALResult GetRenderState( Tr2RenderContextEnum::RenderState state, uint32_t* value ) throw();

	ALResult SetClipPlane( uint32_t planeIndex, const float* planeEq ) throw();

	ALResult SetScissorRect(			
		uint32_t left, 
		uint32_t top, 
		uint32_t right, 
		uint32_t bottom ) throw();

	ALResult SetConstants(			
		const Tr2ConstantBufferAL& buffer, 
		Tr2RenderContextEnum::ShaderType constantType, 
		uint32_t registerIndex, 
		uint32_t unusedArgument = 0 ) throw();

	ALResult SetSamplerState(		
		const Tr2SamplerStateAL& samplerState, 
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t registerNumber ) throw();

	// Set a depth stencil.  Ideally you'd set renderTarget and depthStencil at the same time.
	ALResult SetDepthStencil( const Tr2DepthStencilAL& depthStencil ) throw();
	void SetReadOnlyDepth( bool enable ) throw();
	bool GetReadOnlyDepth() const;
	ALResult SetRenderTarget( const Tr2RenderTargetAL& renderTarget, uint32_t slot = 0 ) throw();
	//Tr2RenderTargetAL GetRenderTarget( unsigned slot = 0 );

	static void DestroyMainThreadRenderContext();

	// Helper function to clear the current primary backbuffer, depth and/or stencil.
	ALResult Clear(						
		uint32_t clearFlags, 
		uint32_t color, 
		float depth, 
		uint32_t stencil = 0,
		uint32_t slot = 0 ) throw();

	// rather hacky call to make integer loops work across DX9 and DX11.
	ALResult SetNumberOfLights( uint32_t numLights ) throw();

	ALResult SetViewport( const Tr2Viewport& viewport ) throw();
	ALResult GetViewport( Tr2Viewport& viewport ) throw();

	ALResult PushRenderTarget( uint32_t slot = 0 ) throw();
	ALResult PopRenderTarget( uint32_t slot = 0 ) throw();
	ALResult PushDepthStencil() throw();
	ALResult PopDepthStencil() throw();
	ALResult GetRenderTargetSize( uint32_t& width, uint32_t& height, uint32_t slot = 0 ) throw();

	Tr2RenderContextEnum::PixelFormat GetBackBufferFormat() const throw();
	
	static const uint32_t SHADER_TYPE_MASK = 
		( 1 << Tr2RenderContextEnum::VERTEX_SHADER ) |
		( 1 << Tr2RenderContextEnum::PIXEL_SHADER ) |
		( 1 << Tr2RenderContextEnum::COMPUTE_SHADER ) |
		( 1 << Tr2RenderContextEnum::GEOMETRY_SHADER ) |
		( 1 << Tr2RenderContextEnum::HULL_SHADER ) |
		( 1 << Tr2RenderContextEnum::DOMAIN_SHADER );

	// Debug helpers
	size_t GetStackSizeRT( uint32_t RT = 0 )	const { return m_stackRT[RT].size(); }
	size_t GetStackSizeDS()						const { return m_stackDS    .size(); }

	void	ResetCapturePlayback();

	// Set of variables that are the first thing we need in ApplyShadowState, keep them
	// close for the cache -- don't put renderstates, renderStateEmulation or m_esm
	// in between.
private:
	uint32_t						m_dirtyFlag;
public:
	CComPtr<ID3D11DeviceContext>	m_context;
private:
	// Current vertex layout (for creating vertex layout)
	const Tr2VertexLayoutAL* m_vertexLayout;
	const Tr2VertexLayoutAL* m_lastSetVertexLayout;
	uint32_t m_lastSetVertexLayoutVSHash;

	// Current vertex shader (for creating vertex layout)
	const Tr2ShaderAL* m_vertexShader;
	// Current pixel shader (for alpha test patching)
	const Tr2ShaderAL* m_pixelShader;
	// Has active hull shader (requires changing topology)

	// UAVs for pixel shader (need to be set all at once)
	CComPtr<ID3D11UnorderedAccessView> m_pixelShaderUavs[16];
	uint32_t m_pixelShaderUavInitialCounts[16];

	// Indexes of dirty pixel shader UAVs
	uint32_t m_psUavsDirtyBegin;
	uint32_t m_psUavsDirtyEnd;

	Tr2RenderContextEnum::Topology	m_topology;
	Tr2RenderContextEnum::Topology	m_lastSetTopology;
	// If readonly depth buffer was requested
	bool m_useReadOnlyDepthView;
	// If readonly depth buffer needs to be set (i.e. if m_useReadOnlyDepthView and we are not writing to depth)
	bool m_isDepthReadOnly;
	bool m_isSrgbRenderTarget;

	ALResult SetRenderStatesImpl( const uint32_t* stateValuePairs, uint32_t count ) throw();

	struct SharedConstantBuffer
	{
		Tr2ConstantBufferAL constantBuffer;
		CcpMallocBuffer mirror;
		size_t size;
	};

	static const uint32_t CB_SLOT_COUNT = 16;
	SharedConstantBuffer m_sharedConstantBuffers[Tr2RenderContextEnum::SHADER_TYPE_COUNT * CB_SLOT_COUNT];

public:
	uint32_t ComputeVertexCount( uint32_t primitiveCount ) const throw();

	CComPtr<ID3D11Device>			m_secondaryDevice11;
	CComPtr<IDXGISwapChain>			m_secondarySwapChain;
	
	std::shared_ptr<Tr2RenderTargetAL> m_secondaryDefaultBackBuffer;		

	bool						IsBackBuffer( const Tr2RenderTargetAL& rt ) const throw();

	// If you need this, you're probably doing something wrong :P
	Tr2RenderTargetAL&			GetDefaultBackBuffer();

private:	
	// We can only set renderTarget and depthStencil together, so remember what's set in case code asks
	// to update only one.
	enum { MAX_RENDER_TARGET = 8 };
	const Tr2RenderTargetAL*	m_boundRenderTarget[MAX_RENDER_TARGET];
	uint32_t					m_renderTargetHighWaterMark;

	const Tr2DepthStencilAL*	m_boundDepthStencil;

	Tr2RenderStateEmulation		m_renderStateEmulation;
		
	bool	ApplyShadowRenderStates() throw();
	bool	ApplyBlendState() throw();
	bool	ApplyDepthStencilState() throw();
	bool	ApplyRasterizerState() throw();
	void ApplyReadOnlyDepth() throw();
	void ApplyUavs() throw();

	Tr2ConstantBufferAL	m_fragmentOpBuffer;

#if 1
	uint32_t			m_allRenderStates[ Tr2RenderContextEnum::RS_MAX_STATE ];	// for GetRenderState
#else
	struct TQueryableRenderState
	{
		uint32_t	m_colorWriteEnable;
		uint32_t	m_zEnable;
		uint32_t	m_zWriteEnable;
		uint32_t	m_cullMode;
	};
	TQueryableRenderState	m_queryableRenderState;
#endif

	
	bool m_hasHullShader;
	// Has active hull shader (requires changing topology)
	bool m_previouslyHadHullShader;	

	Tr2DrawUPHelper	m_drawUP;

	ALResult SetRtDsToDevice( uint32_t changedSlot ) throw();

private:

	friend class Tr2PrimaryRenderContextAL;

	TrackableStdStack<const Tr2RenderTargetAL*>	m_stackRT[MAX_RENDER_TARGET];
	TrackableStdStack<const Tr2DepthStencilAL*>	m_stackDS;

	Tr2RenderContextAL( const Tr2RenderContextAL& ) /* = delete */;
	Tr2RenderContextAL& operator=( const Tr2RenderContextAL& ) /* = delete */;

public:
	CComPtr<ID3D11CommandList>		m_commandList;
	ITr2RenderContextEvents* m_events;
};

#endif // ( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#endif //Tr2RenderContextDx11_h_
