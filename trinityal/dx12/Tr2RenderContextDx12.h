////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../Tr2RenderContextEnum.h"
#include "../Tr2DrawUPHelper.h"
#include "../include/Tr2ConstantBufferAL.h"
#include "../include/Tr2ResourceSetAL.h"
#include "../include/Tr2TextureAL.h"
#include "../include/Tr2ShaderProgramAL.h"
#include "../include/Tr2VertexLayoutAL.h"
#include "../include/Tr2RenderPassAL.h"

#include "./util/DescriptorStateCacheDx12.h"
#include "./util/PsoDescription.h"
#include "../Tr2HalHelperStructures.h"

class Tr2ConstantBufferAL;
struct ITr2RenderContextEvents;

class Tr2ShaderAL;
class Tr2SamplerStateAL;
class Tr2BufferAL;
struct Tr2Viewport;



class Tr2RenderContextAL
{
public:
	Tr2RenderContextAL() throw( );
	~Tr2RenderContextAL() throw( );

	void Destroy() throw( );

	bool IsValid() const throw( );

	ALResult CreateDx12( ID3D12CommandAllocator* commandAllocator, Tr2PrimaryRenderContextAL& renderContext );

	static void SetPrimaryRenderContext( Tr2PrimaryRenderContextAL* )
	{

	}
	Tr2PrimaryRenderContextAL& GetPrimaryRenderContext();
	Tr2PrimaryRenderContextAL* GetPrimaryRenderContextPointer();

	ALResult BeginScene() throw( );
	ALResult EndScene();

	void ReleaseDeviceResources() throw( )
	{

	}


	ALResult SetStreamSource( uint32_t stream, const Tr2BufferAL& buffer, uint32_t offset, uint32_t stride ) throw( );

	ALResult SetIndices( const Tr2BufferAL & buffer ) throw( );

	ALResult ClearUav( Tr2BufferAL& buffer, const float values[4] ) throw( );
	ALResult ClearUav( Tr2BufferAL& buffer, const uint32_t values[4] ) throw( );
	ALResult ClearUav( Tr2TextureAL& rt, uint32_t mip, const float values[4] ) throw( );
	ALResult ClearUav( Tr2TextureAL& rt, uint32_t mip, const uint32_t values[4] ) throw( );

	ALResult CopySubBuffer(
		Tr2BufferAL& dest,
		uint32_t destOffset,
		Tr2BufferAL& src,
		uint32_t offset,
		uint32_t length );

	ALResult SetTopology( Tr2RenderContextEnum::Topology topology ) throw( );
	ALResult SetVertexLayout( const Tr2VertexLayoutAL& layout ) throw( );
	ALResult SetShaderProgram( const Tr2ShaderProgramAL& shader ) throw( );

	ALResult SetResourceSet( const Tr2ResourceSetAL& resourceSet ) throw( );
	
	ALResult DrawIndexedPrimitive(
		uint32_t numVertices,
		uint32_t startIndex,
		uint32_t primitiveCount,
		uint32_t minimumIndex = 0 ) throw( );

	ALResult DrawPrimitive( uint32_t startVertex, uint32_t primitiveCount ) throw( );

	ALResult DrawIndexedInstanced(
		uint32_t numVertices,
		uint32_t startIndex,
		uint32_t primitiveCount,
		uint32_t numInstances ) throw( );
	
	ALResult DrawIndexedInstancedIndirect( Tr2BufferAL& params, uint32_t offset ) throw( );
	ALResult DrawInstancedIndirect( Tr2BufferAL& params, uint32_t offset ) throw( );

	ALResult DrawIndexedPrimitiveUP(
		uint32_t numVertices,
		uint32_t primitiveCount,
		const uint32_t* indexData,
		const void* vertexStreamZeroData,
		uint32_t vertexStreamZeroStride ) throw( );
	ALResult DrawIndexedPrimitiveUP(
		uint32_t numVertices,
		uint32_t primitiveCount,
		const uint16_t* indexData,
		const void* vertexStreamZeroData,
		uint32_t vertexStreamZeroStride ) throw( );
	ALResult DrawPrimitiveUP(
		uint32_t primitiveCount,
		const void* vertexStreamZeroData,
		uint32_t VertexStreamZeroStride ) throw( );

	ALResult RunComputeShader( unsigned groupDimX, unsigned groupDimY, unsigned groupDimZ ) throw( );
	ALResult RunComputeShaderIndirect( Tr2BufferAL& indirectParams, unsigned offset ) throw( );

	ALResult SetRenderState( Tr2RenderContextEnum::RenderState state, uint32_t value ) throw( );
	ALResult SetRenderStates( const uint32_t* stateValuePairs, uint32_t count ) throw( );

	ALResult SetConstants(
		const Tr2ConstantBufferAL& buffer,
		Tr2RenderContextEnum::ShaderType constantType,
		uint32_t registerIndex,
		uint32_t unusedArgument = 0 ) throw( );


	static void DestroyMainThreadRenderContext()
	{

	}

	ALResult Clear(
		uint32_t clearFlags,
		uint32_t color,
		float depth,
		uint32_t stencil = 0,
		uint32_t slot = 0 ) throw( );

	ALResult SetViewport( const Tr2Viewport& viewport ) throw( );
	ALResult GetViewport( Tr2Viewport& viewport ) throw( );

	ALResult SetRenderTarget( const Tr2TextureAL& renderTarget, uint32_t slot = 0, uint32_t slice = 0 ) throw( );
	ALResult PushRenderTarget( uint32_t slot = 0 ) throw( );
	ALResult PopRenderTarget( uint32_t slot = 0 ) throw( );

	ALResult SetDepthStencil( const Tr2TextureAL& depthStencil ) throw( );
	ALResult PushDepthStencil() throw( );
	ALResult PopDepthStencil() throw( );
	void SetReadOnlyDepth( bool enable ) throw( );
	bool GetReadOnlyDepth() const;

	void RenderPassHint( const Tr2ColorAttachment& rt0, const Tr2DepthAttachment& depth );
	void RenderPassHint( const Tr2ColorAttachment& rt0, const Tr2ColorAttachment& rt1, const Tr2DepthAttachment& depth );

	ALResult GetRenderTargetSize( uint32_t& width, uint32_t& height, uint32_t slot = 0 ) throw( );
	
	static const uint32_t SHADER_TYPE_MASK = 
		( 1 << Tr2RenderContextEnum::VERTEX_SHADER ) |
		( 1 << Tr2RenderContextEnum::PIXEL_SHADER ) |
		( 1 << Tr2RenderContextEnum::COMPUTE_SHADER ) |
		( 1 << Tr2RenderContextEnum::GEOMETRY_SHADER ) |
		( 1 << Tr2RenderContextEnum::HULL_SHADER ) |
		( 1 << Tr2RenderContextEnum::DOMAIN_SHADER );

	// Debug helpers
	size_t GetStackSizeRT( uint32_t = 0 )	const { return 0; }
	size_t GetStackSizeDS()						const { return 0; }

	void AddGpuMarker( const char* marker );
	void PushGpuMarker( const char* marker );
	void PopGpuMarker();

	ALResult FlushAndSyncDx12();

	bool IsBoundDx12( const TrinityALImpl::Tr2TextureAL& texture, D3D12_RESOURCE_STATES& boundState );
	void ResetDx12();

	/** Force the current descriptor cache to dirty if something has modified active heaps */
	void DirtyDescriptorCache();
	void ReApplyStateDx12();


	void ResourceBarrierDx12( size_t count, const D3D12_RESOURCE_BARRIER* barriers );
	void ResourceBarrierDx12( const D3D12_RESOURCE_BARRIER& barrier );
	void FlushBarriersDx12();
	void FlushBarriersDx12( ID3D12Resource* resource );
	void FlushBarriersDx12( ID3D12Resource* resource1, ID3D12Resource* resource2 );
	void FlushBarriersDx12( size_t count, ID3D12Resource** resources );

	void FlushGraphicsBarriersDx12( ID3D12Resource* resource = nullptr );
	void FlushComputeBarriersDx12( ID3D12Resource* resource = nullptr );

protected:
	ID3D12PipelineState* GetPipelineState();
	ALResult SetAllState();

	/** Forcibly reset and dirty all descriptor caches (used for explicit synchronization) */
	void ResetDescriptorCaches();

	struct VB
	{
		Tr2BufferAL buffer;
		uint32_t offset;
		uint32_t stride;
	};

	VB m_vertexBuffers[4];
	uint32_t m_dynamicVBs;

	Tr2BufferAL m_indexBuffer;
	bool m_dynamicIB;

	TrinityALImpl::PSODescription m_psoDescription;


	bool m_separateAlphaBlendEnabled;
	bool m_srgbWriteEnable;

	std::pair<uint32_t, uint32_t> m_primitiveToVertexCount;

	Tr2ResourceSetAL m_resourceSet;

	std::vector<std::shared_ptr<DescriptorStateCache>> m_descriptorCache;

	bool GetRenderTargetHandles( D3D12_CPU_DESCRIPTOR_HANDLE* handles, uint32_t& count );
public:
	// If you need this, you're probably doing something wrong :P
	//Tr2TextureAL&			GetDefaultBackBuffer()
	CComPtr<ID3D12GraphicsCommandList> m_commandList;
	CComPtr<ID3D12GraphicsCommandList2> m_commandList2;

	Tr2PrimaryRenderContextAL* m_ownerDevice;
	bool m_dirtyPso;
protected:


	static const uint32_t RENDER_TARGET_COUNT = 4;

	struct BoundRT
	{
		Tr2TextureAL texture;
		uint32_t slice;
	};
	BoundRT m_boundRenderTargets[RENDER_TARGET_COUNT];
	std::vector<BoundRT> m_rtStack[RENDER_TARGET_COUNT];

	Tr2TextureAL m_boundDepthStencil;
	std::vector<Tr2TextureAL> m_dsStack;
	bool m_readOnlyDepth;

	Tr2Viewport m_viewport;

	Tr2RenderContextEnum::Topology m_topology;
	TrinityALImpl::Tr2DrawUPHelper m_drawUPHelper;
private:

	std::vector<D3D12_RESOURCE_BARRIER> m_barriers;

	Tr2RenderContextAL( const Tr2RenderContextAL& ) /* = delete */;
	Tr2RenderContextAL& operator=( const Tr2RenderContextAL& ) /* = delete */;

};

#endif
