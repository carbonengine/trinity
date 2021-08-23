#pragma once
#ifndef Tr2RenderContextDx9_h_
#define Tr2RenderContextDx9_h_


#include "../Tr2RenderContextEnum.h"
#include "../Tr2MemoryCounterAL.h"
#include "../include/Tr2CapsAL.h"
#include "../include/Tr2SamplerStateAL.h"
#include "../include/Tr2TextureAL.h"
#include "../include/Tr2RenderPassAL.h"


#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

class Tr2ConstantBufferAL;
class Tr2VertexLayoutAL;
class Tr2ShaderAL;
class Tr2SamplerStateAL;
class Tr2TextureAL;
class Tr2BufferAL;
struct ITr2RenderContextEvents;
struct Tr2PresentParametersAL;
struct Tr2Viewport;
class Tr2ResourceSetAL;

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
	static Tr2PrimaryRenderContextAL* GetPrimaryRenderContextPointer();

	ALResult CreateDevice( 
		uint32_t Adapter, 
		Tr2WindowHandle  hFocusWindow, 
		const Tr2PresentParametersAL& presentationParameters );
	ALResult SetPresentParameters( unsigned adapter, const Tr2PresentParametersAL& presentationParameters );

	const Tr2CapsAL& GetCaps() const;

	ALResult BeginScene();
	ALResult EndScene();
	ALResult Present();

	bool IsValid();

	// DX9 only method to handle lost devices
	bool IsLost() const;
	ALResult TestCooperativeLevel();

	void ReleaseDeviceResources();


	ALResult SetStreamSource(
		uint32_t stream,
		const Tr2BufferAL & buffer,
		uint32_t offset,
		uint32_t stride ) throw( );
	ALResult SetIndices( const Tr2BufferAL & buffer ) throw( );
	ALResult ClearUav( Tr2BufferAL& buffer, const float values[4] ) throw( );
	ALResult ClearUav( Tr2BufferAL& buffer, const uint32_t values[4] ) throw( );

	ALResult CopySubBuffer(
		Tr2BufferAL& dest,
		uint32_t destOffset,
		Tr2BufferAL& src,
		uint32_t offset,
		uint32_t length );

	ALResult SetTopology( long topology );
	ALResult SetShaderProgram( const Tr2ShaderProgramAL& shaderProgram );

	ALResult ClearUav( Tr2TextureAL& rt, uint32_t mipLevel, const float values[4] ) throw( )
	{
		CCP_UNUSED( rt );
		CCP_UNUSED( values );
		CCP_UNUSED( mipLevel );

		return E_FAIL;
	}

	ALResult ClearUav( Tr2TextureAL& rt, uint32_t mipLevel, const uint32_t values[4] ) throw( )
	{
		CCP_UNUSED( rt );
		CCP_UNUSED( values );
		CCP_UNUSED( mipLevel );

		return E_FAIL;
	}
	
	ALResult SetResourceSet( const Tr2ResourceSetAL& resourceSet );

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

	ALResult DrawIndexedInstancedIndirect( Tr2BufferAL& /*params*/, uint32_t offset )
	{
		CCP_UNUSED( offset );

		return E_FAIL;
	}

	ALResult DrawInstancedIndirect( Tr2BufferAL& /*params*/, uint32_t offset )
	{
		CCP_UNUSED( offset );

		return E_FAIL;
	}

	ALResult RunComputeShader( unsigned groupDimX, unsigned groupDimY, unsigned groupDimZ )
	{
		CCP_UNUSED( groupDimX );
		CCP_UNUSED( groupDimY );
		CCP_UNUSED( groupDimZ );

		return E_FAIL;
	}
	ALResult RunComputeShaderIndirect( Tr2BufferAL& /*indirectParams*/, unsigned offset )
	{
		CCP_UNUSED( offset );

		return E_FAIL;
	}

	ALResult CopyBufferCounter( Tr2BufferAL& /*dest*/, uint32_t destOffset, Tr2BufferAL& /*src*/ )
	{
		CCP_UNUSED( destOffset );

		return E_FAIL;
	}

	ALResult SetVertexLayout( const Tr2VertexLayoutAL& layout );

	ALResult SetRenderState( Tr2RenderContextEnum::RenderState state, uint32_t value );
	ALResult SetRenderStates( const uint32_t * stateValuePairs, uint32_t count );


	CComPtr<IDirect3DDevice9>	m_d3dDevice9;
	bool	m_usingEXDevice;
	std::vector<CComPtr<IUnknown> >	m_frameDelayedDX9Objects;


	long m_topology;	// in DX9, that's part of the DrawXyz calls, so remember this.

	ALResult SetConstants(			
		const Tr2ConstantBufferAL& buffer, 
		Tr2RenderContextEnum::ShaderType constantType, 
		uint32_t registerIndex, 
		uint32_t maxRegisterCount = 0 );

	// Helper function to clear the current primary backbuffer, depth and/or stencil.
	ALResult Clear(						
		uint32_t clearFlags, 
		uint32_t color, 
		float depth, 
		uint32_t stencil = 0,
		uint32_t slot = 0 );

	ALResult SetDepthStencil( const Tr2TextureAL& depthStencil );
	void SetReadOnlyDepth(			bool enable );
	bool GetReadOnlyDepth() const;
	ALResult SetRenderTarget( const Tr2TextureAL& renderTarget, uint32_t slot = 0 );

	void RenderPassHint( const Tr2ColorAttachment& rt0, const Tr2DepthAttachment& depth );
	void RenderPassHint( const Tr2ColorAttachment& rt0, const Tr2ColorAttachment& rt1, const Tr2DepthAttachment& depth );

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

	void TrashQuery( IDirect3DQuery9* query );

	Tr2RenderContextEnum::PixelFormat GetBackBufferFormat() const;
	
	static const uint32_t SHADER_TYPE_MASK = 
		( 1 << Tr2RenderContextEnum::VERTEX_SHADER ) |
		( 1 << Tr2RenderContextEnum::PIXEL_SHADER );

	// Debug helpers
	size_t GetStackSizeRT( uint32_t RT = 0 )	const { return m_stackRT[RT].size(); }
	size_t GetStackSizeDS()						const { return m_stackDS    .size(); }

	Tr2CapsAL m_caps;

	ALResult InternalBlit( IDirect3DSurface9* destination, IDirect3DBaseTexture9* source, uint32_t width, uint32_t height );

	ITr2RenderContextEvents* m_events;
	Tr2TextureAL& GetDefaultBackBuffer() { return m_defaultBackBuffer; }

	void AddGpuMarker( const char* marker );
	void PushGpuMarker( const char* marker );
	void PopGpuMarker();
	ALResult GetGpuStateMarker( Tr2RenderContextEnum::RenderContextStatus& status, std::string& marker ) const;
	ALResult GetGpuPageFaultResource(
		Tr2RenderContextEnum::PixelFormat& format,
		uint64_t& size,
		uint32_t& width,
		uint32_t& height,
		uint32_t& depth,
		uint32_t& mips ) const;
private:
	enum { MAX_RENDER_TARGET = 8 };
	Tr2TextureAL m_boundRenderTarget[MAX_RENDER_TARGET];
	TrackableStdStack<IDirect3DSurface9*>	m_stackRT[MAX_RENDER_TARGET];
	TrackableStdStack<IDirect3DSurface9*>	m_stackDS;

	struct Blitter;
	Blitter* m_blitter;
	//dustbin for orphaned queries
	TrackableStdVector<CComPtr<IDirect3DQuery9>> m_queryDustbin;
public:
	TrinityALImpl::Tr2SamplerStateALFactory m_samplerStateFactory;
private:	
	uint32_t m_samplerHash;
	Tr2TextureAL m_defaultBackBuffer;
	CComPtr<IDirect3DSurface9> m_nullRT;
	uint32_t m_adapter;
	Tr2MemoryCounterAL m_memory;
	bool m_isLost;

	Tr2RenderContextAL( const Tr2RenderContextAL& ) /* = delete */;
	Tr2RenderContextAL& operator=( const Tr2RenderContextAL& ) /* = delete */;

	bool ProcessOrphanedQueries( bool flush );
	bool HasStencilBuffer();

	ALResult ReportIfFailure( long returnCode, const char * message );
};

#endif	// #if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#endif //Tr2RenderContextDx9_h_
