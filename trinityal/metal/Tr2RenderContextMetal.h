#pragma once

#if( TRINITY_PLATFORM == TRINITY_METAL )

#include "../Tr2RenderContextEnum.h"
#include "../Tr2DrawUPHelper.h"
#include "../include/Tr2TextureAL.h"
#include "../include/Tr2SwapChainAL.h"
#include "../include/Tr2CapsAL.h"
#include "../include/Tr2ResourceSetAL.h"
#include "../include/Tr2SamplerStateAL.h"
#include "../include/Tr2ShaderProgramAL.h"
#include "../include/Tr2VertexLayoutAL.h"
#include "../include/Tr2ConstantBufferAL.h"
#include "../include/Tr2RenderPassAL.h"
#include "../Tr2HalHelperStructures.h"
#include "../Tr2AdapterStructures.h"
#include "../include/Tr2RtTopLevelAccelerationStructureAL.h"
#include "MetalContext.h"
#include <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

class Tr2ConstantBufferAL;
class Tr2ShaderAL;
class Tr2SamplerStateAL;
class Tr2TextureAL;
class Tr2ResourceSetAL;
class Tr2BufferAL;
class Tr2RtShaderTableAL;
class Tr2RtPipelineStateAL;
struct ITr2RenderContextEvents;


class Tr2BindlessResourcesAL
{
public:
	void Add( const Tr2TextureAL& texture );
	void Add( const Tr2BindlessResourcesAL& resources );
	void Clear();
	friend class Tr2RenderContextAL;

private:
	std::vector<TrinityALImpl::Tr2TextureAL*> m_textures;
};

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

	void ReleaseDeviceResources();

	ALResult SetStreamSource(
		uint32_t stream,
		const Tr2BufferAL & buffer,
		uint32_t offset,
		uint32_t stride ) throw( );

	ALResult SetIndices( const Tr2BufferAL& buffer ) throw( );
	ALResult SetIndices( const Tr2BufferAL& buffer, int stride ) throw();

	ALResult ClearUav( Tr2BufferAL& buffer, const float values[4] ) throw( );
	ALResult ClearUav( Tr2BufferAL& buffer, const uint32_t values[4] ) throw( );
	ALResult ClearUav( Tr2TextureAL& texture, uint32_t mipLevel, const float values[4] ) throw( );
	ALResult ClearUav( Tr2TextureAL& texture, uint32_t mipLevel, const uint32_t values[4] ) throw( );

	ALResult CopySubBuffer(
		Tr2BufferAL& dest,
		uint32_t destOffset,
		Tr2BufferAL& src,
		uint32_t offset,
		uint32_t length );

	ALResult SetTopology( Tr2RenderContextEnum::Topology topology );
	ALResult SetShaderProgram( const Tr2ShaderProgramAL& shaderProgram );

	ALResult SetResourceSet( const Tr2ResourceSetAL& resourceSet );
	
	ALResult DrawIndexedPrimitive(
		uint32_t numVertices,
		uint32_t startIndex,
		uint32_t primitiveCount,
		uint32_t minimumIndex = 0 );

	ALResult DrawPrimitive( uint32_t startVertex, uint32_t primitiveCount );

	ALResult DrawIndexedInstanced(
		uint32_t numVertices,
		uint32_t startIndex,
		uint32_t primitiveCount,
		uint32_t numInstances );

	ALResult DrawIndexedInstanced(
		uint32_t indexCountPerInstance,
		uint32_t instanceCount,
		uint32_t startIndexLocation,
		int32_t baseVertexLocation,
		uint32_t startInstanceLocation );
	ALResult DrawInstanced(
		uint32_t vertexCountPerInstance,
		uint32_t instanceCount,
		uint32_t startVertexLocation,
		uint32_t startInstanceLocation );

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

	ALResult DrawIndexedInstancedIndirect( Tr2BufferAL& params, uint32_t offset );
	ALResult DrawInstancedIndirect( Tr2BufferAL& params, uint32_t offset );

	ALResult RunComputeShader( unsigned groupDimX, unsigned groupDimY, unsigned groupDimZ );
	ALResult RunComputeShaderIndirect( Tr2BufferAL& indirectParams, unsigned offset );

    ALResult DispatchRays( Tr2RtPipelineStateAL& pipeline, Tr2RtShaderTableAL& shaderTable, const wchar_t* rayGenShader, uint32_t width, uint32_t height, uint32_t depth );
    
	ALResult SetVertexLayout( const Tr2VertexLayoutAL& layout );

	ALResult SetRenderState( Tr2RenderContextEnum::RenderState state, uint32_t value );
	ALResult SetRenderStates( const uint32_t * stateValuePairs, uint32_t count );

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
	void SetReadOnlyDepth( bool enable );
	bool GetReadOnlyDepth() const;
	ALResult SetRenderTarget( const Tr2TextureAL& renderTarget, uint32_t slot = 0, uint32_t slice = 0 );

	void RenderPassHint( const Tr2ColorAttachment& rt0, const Tr2DepthAttachment& depth );
	void RenderPassHint( const Tr2ColorAttachment& rt0, const Tr2ColorAttachment& rt1, const Tr2DepthAttachment& depth );
    void EndRenderPassHint();

	ALResult SetViewport( const Tr2Viewport& viewport );
	ALResult GetViewport( Tr2Viewport& viewport );

	ALResult PushRenderTarget( uint32_t slot = 0 );
	ALResult PopRenderTarget( uint32_t slot = 0 );
	ALResult PushDepthStencil();
	ALResult PopDepthStencil();
	ALResult GetRenderTargetSize(
		uint32_t& width,
		uint32_t& height,
		uint32_t slot = 0 );

	void ResetRenderTargets();

	long GetTotalVideoMemory();

	Tr2RenderContextEnum::PixelFormat GetBackBufferFormat() const;

	static const uint32_t SHADER_TYPE_MASK =
		( 1 << Tr2RenderContextEnum::VERTEX_SHADER ) |
		( 1 << Tr2RenderContextEnum::PIXEL_SHADER ) |
		( 1 << Tr2RenderContextEnum::COMPUTE_SHADER );

	// Debug helpers
	size_t GetStackSizeRT( uint32_t RT = 0 )	const { return 0; }
	size_t GetStackSizeDS()						const { return 0; }

	Tr2CapsAL m_caps;

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

	TrinityALImpl::MetalContext *GetMetalContext() { return m_metalContext; }
	TrinityALImpl::MetalWorkQueue *GetMetalWorkQueue() { return m_workQueue; }
	Tr2PresentParametersAL      *GetPresentParamaters() { return &m_presentParameters; }

	uint32_t ComputeVertexCount( uint32_t primitiveCount );
	
	void BufferRewritten( id<MTLBuffer> from, id<MTLBuffer> to );
	
	uint32_t BeginParallelEncoding( uint32_t requestedEncodersCount );
	void EndParallelEncoding();
	
	ALResult ForkContext( Tr2RenderContextAL* context, uint32_t index ) const;

	ALResult UseTextures( Tr2GpuUsage::Type usage, const Tr2BindlessResourcesAL& resources );
    ALResult UseAccelerationStructure( NSMutableArray *primitiveAccelerationStructures );

protected:
	bool                               m_isValid;

	enum { MAX_RENDER_TARGET = 8 };
	Tr2SwapChainAL                     m_swapChain;

	struct BoundRT
	{
		Tr2TextureAL texture;
		uint32_t slice;
	};
	BoundRT m_boundRenderTargets[MAX_RENDER_TARGET];
	Tr2TextureAL                       m_boundDepthStencil;
	TrackableStdStack<BoundRT> m_stackRT[MAX_RENDER_TARGET];
	TrackableStdStack<Tr2TextureAL>    m_stackDS;
	Tr2TextureAL                       m_defaultBackBuffer;

	Tr2Viewport                        m_viewport;
	TrinityALImpl::MetalContext       *m_metalContext;
	TrinityALImpl::MetalWorkQueue *m_workQueue;

	Tr2PresentParametersAL             m_presentParameters;

	Tr2ShaderProgramAL                 m_shaderProgram;
	Tr2ResourceSetAL                   m_resourceSet;
	Tr2VertexLayoutAL                  m_vertexLayout;

	bool                               m_needsDrawResourceCheck;

	struct MetalPrimitiveInfo
	{
		MTLPrimitiveType metalPrimitiveType;
		uint32_t         primitiveToIndexMultiplier;
		uint32_t         primtiveToIndexAddition;
		bool             validType;
	};

	MetalPrimitiveInfo                 m_metalPrimitiveInfo;
	MTLIndexType                       m_metalIndexType;
	id<MTLBuffer>                      m_metalIndexBuffer;
	MTLCompareFunction                 m_depthCompareFunction;
	CAMetalLayer                      *m_caMetalLayer;
	
	static const uint32_t VERTEX_STREAM_COUNT = METAL_VERTEX_STREAM_BUFFER_COUNT;
	
	struct
	{
		id<MTLBuffer> buffer;
		uint32_t offset;
		uint32_t stride;
	} m_boundVertexStreams[VERTEX_STREAM_COUNT];

	Tr2RenderContextEnum::Topology m_topology;
	TrinityALImpl::Tr2DrawUPHelper m_drawUPHelper;

	bool m_depthWriteEnabled;
	bool m_depthCompareEnabled;
	bool m_readOnlyDepth;
	bool m_srgbWriteEnable;
	bool m_alphaBlendEnable;
	bool m_separateAlphaBlendEnabled;
	bool m_isPrimary;
	
	uint32_t m_queueIndex;

	void CheckDrawResources();
	void SetAsPrimary();

public:
	TrinityALImpl::Tr2SamplerStateALFactory m_samplerStateFactory;
};

#endif
