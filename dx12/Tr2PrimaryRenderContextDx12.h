////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../Tr2MemoryCounterAL.h"
#include "../include/Tr2RenderContextAL.h"
#include "../include/Tr2CapsAL.h"
#include "../include/Tr2SamplerStateAL.h"
#include "../include/Tr2TextureAL.h"


struct Tr2PresentParametersAL;
namespace TrinityALImpl
{
	class GenerateMipsResources;
}


class Tr2PrimaryRenderContextAL : public Tr2RenderContextAL
{
public:
	Tr2PrimaryRenderContextAL();
	~Tr2PrimaryRenderContextAL();

	ALResult CreateDevice( uint32_t adapter, Tr2WindowHandle  focusWindow, const Tr2PresentParametersAL& presentationParameters );
	void Destroy();
	bool IsValid() const;

	ALResult SetPresentParameters( uint32_t adapter, const Tr2PresentParametersAL& presentationParameters );

	const Tr2CapsAL& GetCaps() const;
		
	ALResult Present();

	Tr2RenderContextEnum::PixelFormat GetBackBufferFormat() const;
	
	static const uint32_t SHADER_TYPE_MASK = 
		( 1 << Tr2RenderContextEnum::VERTEX_SHADER ) |
		( 1 << Tr2RenderContextEnum::PIXEL_SHADER ) |
		( 1 << Tr2RenderContextEnum::COMPUTE_SHADER ) |
		( 1 << Tr2RenderContextEnum::GEOMETRY_SHADER ) |
		( 1 << Tr2RenderContextEnum::HULL_SHADER ) |
		( 1 << Tr2RenderContextEnum::DOMAIN_SHADER );

public:
	Tr2TextureAL m_defaultBackBuffer;

	ITr2RenderContextEvents* m_events;

	TrinityALImpl::GenerateMipsResources* m_genMipsResources;

public:
	TrinityALImpl::Tr2SamplerStateALFactory m_samplerStateFactory;

	Tr2TextureAL&			GetDefaultBackBuffer()
	{
		return m_defaultBackBuffer;
	}

	void ReleaseLater( IUnknown* resource );
	uint64_t GetCurrentFrameIndexDx12() const;
	uint32_t GetBackBufferCountDx12() const;
	bool IsFrameCompletedDx12( uint64_t frameIndex ) const;

	void OnShaderProgramDestroyedDx12( Tr2ShaderProgramAL* sp );
	void OnVertexLayoutDestroyedDx12( Tr2VertexLayoutAL* vl );
	ALResult FlushDx12( Tr2RenderContextAL& renderContext );
	ALResult FlushAndSyncDx12( Tr2RenderContextAL& renderContext );
	D3D12_CPU_DESCRIPTOR_HANDLE GetNullRtHandle( const Tr2TextureAL& compatibleWith );
	uint64_t SignalDx12();
	ALResult WaitForFenceDx12( uint64_t value );
private:
	Tr2PrimaryRenderContextAL( const Tr2PrimaryRenderContextAL& ) /* = delete */;
	Tr2PrimaryRenderContextAL& operator=( const Tr2PrimaryRenderContextAL& ) /* = delete */;


	std::vector < CComPtr<ID3D12CommandAllocator>> m_commandAllocators;
	uint32_t m_commandAllocatorIndex;


	Tr2CapsAL m_caps;
	CComPtr<IDXGISwapChain3> m_swapChain;
	CComPtr<IDXGIOutput> m_output;

	uint32_t m_currentBackBufferIndex;

	CComPtr<ID3D12Fence> m_presentFence;
	HANDLE m_presentFenceEvent;

	uint64_t m_fenceValue;
	std::vector<uint64_t> m_frameFenceValues;
	std::vector<std::vector<CComPtr<IUnknown>>> m_pendingRelease;

	uint32_t m_syncInterval;

	CComPtr<ID3D12DescriptorHeap> m_nullRtHeap;
	struct NullRtDesc
	{
		Tr2RenderContextEnum::PixelFormat format;
		uint32_t width;
		uint32_t height;
		Tr2MsaaDesc msaa;

		bool operator==( const NullRtDesc& other ) const
		{
			return format == other.format && width == other.width && height == other.height && msaa == other.msaa;
		}

		operator size_t() const
		{
			return
				std::hash<uint32_t>()( uint32_t( format ) ) ^
				( std::hash<uint32_t>()( width ) << 1 ) ^
				( std::hash<uint32_t>()( height ) << 2 ) ^
				( std::hash<uint32_t>()( msaa.samples ) << 3 );
		}
	};
	std::unordered_map<NullRtDesc, D3D12_CPU_DESCRIPTOR_HANDLE> m_nullRts;
public:
	CComPtr<ID3D12Device> m_device;
	CComPtr<ID3D12CommandQueue> m_commandQueue;

	D3D_ROOT_SIGNATURE_VERSION m_rootSignatureVersion;

	struct Pso
	{
		CComPtr<ID3D12PipelineState> pipelineState;
		const Tr2ShaderProgramAL* shaderProgram;
		const Tr2VertexLayoutAL* vertexLayout;
	};

	typedef std::unordered_map<unsigned, Pso> PipelineStateMap;
	PipelineStateMap m_pipelineStates;

	Tr2ConstantBufferAL m_shadowCB;

	CComPtr<ID3D12CommandSignature> m_drawInstancedIndirect;
	CComPtr<ID3D12CommandSignature> m_drawIndexedInstancedIndirect;
	CComPtr<ID3D12CommandSignature> m_dispatchIndirect;
};

#endif
