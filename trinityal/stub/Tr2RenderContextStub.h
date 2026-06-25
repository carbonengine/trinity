// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2RenderContextStub_h_
#define Tr2RenderContextStub_h_

#if ( TRINITY_PLATFORM == TRINITY_STUB )

#include "../Tr2RenderContextEnum.h"
#include "../include/Tr2TextureAL.h"
#include "../include/Tr2CapsAL.h"
#include "../include/Tr2SamplerStateAL.h"
#include "../include/Tr2RenderPassAL.h"
#include "../include/Tr2RtTopLevelAccelerationStructureAL.h"
#include "../Tr2HalHelperStructures.h"
#include "../include/upscaling/Tr2UpscalingAL.h"

class Tr2ConstantBufferAL;
class Tr2VertexLayoutAL;
class Tr2ShaderAL;
class Tr2SamplerStateAL;
class Tr2TextureAL;
class Tr2ResourceSetAL;
class Tr2BufferAL;
class Tr2RtShaderTableAL;
class Tr2RtPipelineStateAL;
struct ITr2RenderContextEvents;
struct Tr2PresentParametersAL;



class Tr2BindlessResourcesAL
{
public:
	void Add( const Tr2TextureAL& )
	{
	}
	void Add( const Tr2BufferAL& )
	{
	}
	void Add( const Tr2BindlessResourcesAL& )
	{
	}
	void Clear()
	{
	}
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
		Tr2WindowHandle hFocusWindow,
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
		const Tr2BufferAL& buffer,
		uint32_t offset,
		uint32_t stride ) throw();
	ALResult SetIndices( const Tr2BufferAL& buffer ) throw();
	ALResult SetIndices( const Tr2BufferAL& buffer, uint32_t stride ) throw();
	ALResult ClearUav( const Tr2BufferAL& buffer, const float values[4] ) throw();
	ALResult ClearUav( const Tr2BufferAL& buffer, const uint32_t values[4] ) throw();

	ALResult CopySubBuffer(
		Tr2BufferAL& dest,
		uint32_t destOffset,
		Tr2BufferAL& src,
		uint32_t offset,
		uint32_t length );

	ALResult SetTopology( long topology );
	ALResult SetShaderProgram( const Tr2ShaderProgramAL& shaderProgram );


	ALResult ClearUav( const Tr2TextureAL&, uint32_t, const float[4] ) throw()
	{
		return E_FAIL;
	}

	ALResult ClearUav( const Tr2TextureAL&, uint32_t, const uint32_t[4] ) throw()
	{
		return E_FAIL;
	}

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
		uint32_t vertexStreamZeroStride );

	ALResult DrawIndexedPrimitiveUP(
		uint32_t numVertices,
		uint32_t primitiveCount,
		const uint16_t* indexData,
		const void* vertexStreamZeroData,
		uint32_t vertexStreamZeroStride );

	ALResult DrawPrimitiveUP(
		uint32_t primitiveCount,
		const void* vertexStreamZeroData,
		uint32_t vertexStreamZeroStride );

	ALResult DrawIndexedInstancedIndirect( Tr2BufferAL&, uint32_t )
	{
		return E_FAIL;
	}

	ALResult DrawInstancedIndirect( Tr2BufferAL&, uint32_t )
	{
		return E_FAIL;
	}

	ALResult RunComputeShader( unsigned, unsigned, unsigned )
	{
		return E_FAIL;
	}
	ALResult RunComputeShaderIndirect( Tr2BufferAL&, unsigned )
	{
		return E_FAIL;
	}

	ALResult DispatchRays( Tr2RtPipelineStateAL& pipeline, Tr2RtShaderTableAL& shaderTable, const wchar_t* rayGenShader, uint32_t width, uint32_t height, uint32_t depth )
	{
		return E_FAIL;
	}

	ALResult SetVertexLayout( const Tr2VertexLayoutAL& layout );

	ALResult SetRenderState( Tr2RenderContextEnum::RenderState state, uint32_t value );
	ALResult SetRenderStates( const uint32_t* stateValuePairs, uint32_t count );

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

	long GetTotalVideoMemory();

	Tr2RenderContextEnum::PixelFormat GetBackBufferFormat() const;

	static const uint32_t SHADER_TYPE_MASK =
		( 1 << Tr2RenderContextEnum::VERTEX_SHADER ) |
		( 1 << Tr2RenderContextEnum::PIXEL_SHADER );

	// Debug helpers
	size_t GetStackSizeRT( uint32_t = 0 ) const
	{
		return 0;
	}
	size_t GetStackSizeDS() const
	{
		return 0;
	}

	Tr2CapsAL m_caps;

	ITr2RenderContextEvents* m_events;
	Tr2TextureAL& GetDefaultBackBuffer()
	{
		return m_defaultBackBuffer;
	}

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

	ALResult UseResources( Tr2UseResourceDestination dest, Tr2GpuUsage::Type usage, const Tr2BindlessResourcesAL& resources );
	ALResult UseAccelerationStructure( Tr2RtTopLevelAccelerationStructureAL tlas );

	bool SupportsBindlessTextures() const;

	uint64_t GetRecordingFrameNumber() const;
	uint64_t GetRenderedFrameNumber() const;


	Tr2UpscalingAL::Result EnableUpscaling( Tr2UpscalingAL::Technique tech, Tr2UpscalingAL::Setting setting, bool framegeneration, uint32_t adapter );
	Tr2UpscalingContextAL* GetUpscalingContext( uint32_t upscalingContextID );
	Tr2UpscalingContextAL* CreateUpscalingContext( Tr2UpscalingAL::UpscalingContextParams params, uint32_t existingContext = Tr2UpscalingAL::INVALID_CONTEXT_ID );
	void DeleteUpscalingContext( uint32_t contextID );
	Tr2UpscalingAL::UpscalingInfo GetUpscalingInfo( uint32_t upscalingContextID );
	std::vector<std::tuple<Tr2UpscalingAL::Technique, uint32_t, bool>> GetSupportedUpscalingTechniques( uint32_t adapter );
	void GetUpscalingSetup( Tr2UpscalingAL::Technique& technique, Tr2UpscalingAL::Setting& setting, bool& framegeneration, bool& temporal );
	void MarkFrameEvent( Tr2RenderContextEnum::FrameEvent frameEvent );

private:
	enum
	{
		MAX_RENDER_TARGET = 8
	};
	Tr2TextureAL m_boundRenderTarget[MAX_RENDER_TARGET];
	bool m_isValid;
	Tr2TextureAL m_defaultBackBuffer;
	Tr2Viewport m_viewport;
	TrackableStdStack<Tr2TextureAL> m_stackRT[MAX_RENDER_TARGET];
	uint64_t m_frameNumber;

public:
	TrinityALImpl::Tr2SamplerStateALFactory m_samplerStateFactory;
};

#endif // #if( TRINITY_PLATFORM==TRINITY_STUB )

#endif //Tr2RenderContextStub_h_
