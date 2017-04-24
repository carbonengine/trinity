#pragma once
#ifndef Tr2RenderContextDx9_h_
#define Tr2RenderContextDx9_h_

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "Tr2RenderContextEnum.h"
#include "Tr2RenderTargetALDx9.h"
#include "Tr2CapsALDx9.h"

class Tr2VertexBufferAL;
class Tr2IndexBufferAL;
class Tr2ConstantBufferAL;
class Tr2VertexLayoutAL;
class Tr2ShaderAL;
class Tr2SamplerStateAL;
class Tr2TextureAL;
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
	ALResult CreateSecondaryContext() { return E_FAIL; }

	static void SetPrimaryRenderContext( Tr2PrimaryRenderContextAL* );
	static Tr2PrimaryRenderContextAL& GetPrimaryRenderContext();
	static Tr2PrimaryRenderContextAL* GetPrimaryRenderContextPointer();

	ALResult CreateDevice( 
		uint32_t Adapter, 
		Tr2WindowHandle  hFocusWindow, 
		const Tr2PresentParametersAL& presentationParameters );
	ALResult SetPresentParameters( unsigned adapter, const Tr2PresentParametersAL& presentationParameters );

	const Tr2CapsAL& GetCaps() const;

	ALResult BeginScene();
	ALResult EndScene();
	ALResult FinishCommandList()	 { return E_FAIL; }
	ALResult ExecuteCommandList() { return E_FAIL; }
	ALResult Present();

	bool IsValid();

	void ReleaseDeviceResources();

	ALResult SetStreamSource(		uint32_t stream, 
									const Tr2VertexBufferAL & buffer, 
									uint32_t offset, 
									uint32_t stride );

	ALResult SetIndices( const Tr2IndexBufferAL & buffer );
	ALResult SetTopology( long topology );
	ALResult SetShader( const Tr2ShaderAL& shader );

	ALResult SetShaderBuffer(		
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t slot, 
		const Tr2GpuBufferAL& buffer );

	ALResult SetUav(
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t slot, 
		const Tr2GpuBufferAL& buffer,
		uint32_t initialCount = -1 ) throw()
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

	ALResult SetTexture(			
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t slot, 
		const Tr2TextureAL& texture, 
		Tr2RenderContextEnum::ColorSpace colorSpace = Tr2RenderContextEnum::COLOR_SPACE_LINEAR );
	
	ALResult DrawIndexedPrimitive(	
		uint32_t numVertices, 
		uint32_t startIndex, 
		uint32_t primitiveCount, 
		uint32_t minimumIndex = 0 );

	ALResult DrawPrimitive(	uint32_t startVertex, uint32_t primitiveCount );

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

	ALResult DrawInstancedIndirect( Tr2GpuBufferAL& params, uint32_t offset )
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

	ALResult CopyBufferCounter( Tr2GpuBufferAL& dest, uint32_t destOffset, Tr2GpuBufferAL& src )
	{
		return E_FAIL;
	}

	ALResult SetVertexLayout( const Tr2VertexLayoutAL& layout );

	ALResult SetRenderState( Tr2RenderContextEnum::RenderState state, uint32_t value );
	ALResult SetRenderStates( const uint32_t * stateValuePairs, uint32_t count );
	ALResult GetRenderState( Tr2RenderContextEnum::RenderState state, uint32_t* value );

	ALResult SetClipPlane( uint32_t planeIndex, const float* planeEq );

	ALResult SetScissorRect(
		uint32_t left, 
		uint32_t top, 
		uint32_t right, 
		uint32_t bottom );


	CComPtr<IDirect3DDevice9>	m_d3dDevice9;
	bool	m_usingEXDevice;
	std::vector<CComPtr<IUnknown> >	m_frameDelayedDX9Objects;


	long m_topology;	// in DX9, that's part of the DrawXyz calls, so remember this.

	ALResult SetConstants(			
		Tr2ConstantBufferAL& buffer, 
		Tr2RenderContextEnum::ShaderType constantType, 
		uint32_t registerIndex, 
		uint32_t maxRegisterCount = 0 );

	ALResult SetSamplerState(		
		const Tr2SamplerStateAL& samplerState, 
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t registerNumber );


	// Helper function to clear the current primary backbuffer, depth and/or stencil.
	ALResult Clear(						
		uint32_t clearFlags, 
		uint32_t color, 
		float depth, 
		uint32_t stencil = 0,
		uint32_t slot = 0 );

	ALResult SetDepthStencil( const Tr2DepthStencilAL& depthStencil );
	void SetReadOnlyDepth(			bool enable );
	bool GetReadOnlyDepth() const;
	ALResult SetRenderTarget( const Tr2RenderTargetAL& renderTarget, uint32_t slot = 0 );

	ALResult SetNumberOfLights(			uint32_t numLights );

	ALResult SetViewport( const Tr2Viewport& viewport );
	ALResult GetViewport( Tr2Viewport& viewport );
	
	// Debug helper while texture work is WIP
	static D3DFORMAT ConvertToD3D9Format( Tr2RenderContextEnum::PixelFormat format );
	static Tr2RenderContextEnum::PixelFormat ConvertFromD3D9Format( D3DFORMAT format );

	ALResult PushRenderTarget( uint32_t slot = 0 );
	ALResult PopRenderTarget( uint32_t slot = 0 );
	ALResult PushDepthStencil();
	ALResult PopDepthStencil();
	ALResult GetRenderTargetSize(	
		uint32_t& width, 
		uint32_t& height, 
		uint32_t slot = 0 );

	bool IsSupportedRenderTargetFormat( 
		Tr2RenderContextEnum::PixelFormat format, 
		bool withAutoGenMipmap = false );

	void TrashQuery( IDirect3DQuery9* query );

	Tr2RenderContextEnum::PixelFormat GetBackBufferFormat() const;

	ALResult GetAFRGroupCount( uint32_t& count );
	
	static const uint32_t SHADER_TYPE_MASK = 
		( 1 << Tr2RenderContextEnum::VERTEX_SHADER ) |
		( 1 << Tr2RenderContextEnum::PIXEL_SHADER );

	// Debug helpers
	size_t GetStackSizeRT( uint32_t RT = 0 )	const { return m_stackRT[RT].size(); }
	size_t GetStackSizeDS()						const { return m_stackDS    .size(); }

	Tr2CapsAL m_caps;
	Tr2RenderContextEnum::DepthStencilFormat m_depthStencilFormat;

	ALResult InternalBlit( IDirect3DSurface9* destination, IDirect3DBaseTexture9* source, uint32_t width, uint32_t height );

	ITr2RenderContextEvents* m_events;
	Tr2RenderTargetAL& GetDefaultBackBuffer() { return m_defaultBackBuffer; }
private:
	enum { MAX_RENDER_TARGET = 8 };
	const Tr2RenderTargetAL*				m_boundRenderTarget[MAX_RENDER_TARGET];
	TrackableStdStack<IDirect3DSurface9*>	m_stackRT[MAX_RENDER_TARGET];
	TrackableStdStack<IDirect3DSurface9*>	m_stackDS;

	struct Blitter;
	Blitter* m_blitter;
	//dustbin for orphaned queries
	TrackableStdVector<CComPtr<IDirect3DQuery9>> m_queryDustbin;
private:	
	Tr2RenderTargetAL m_defaultBackBuffer;
	CComPtr<IDirect3DSurface9> m_nullRT;
	uint32_t m_adapter;

	Tr2RenderContextAL( const Tr2RenderContextAL& ) /* = delete */;
	Tr2RenderContextAL& operator=( const Tr2RenderContextAL& ) /* = delete */;

	bool ProcessOrphanedQueries( bool flush );
	bool HasStencilBuffer();

	ALResult ReportIfFailure( long returnCode, const char * message );
};

#endif	// #if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#endif //Tr2RenderContextDx9_h_
