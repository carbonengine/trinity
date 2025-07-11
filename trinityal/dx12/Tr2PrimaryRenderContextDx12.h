////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../include/Tr2ShaderAL.h"
#include "../Tr2MemoryCounterAL.h"
#include "../include/Tr2RenderContextAL.h"
#include "../include/Tr2CapsAL.h"
#include "../include/Tr2SamplerStateAL.h"
#include "../include/Tr2TextureAL.h"
#include "../include/Tr2GpuTimerAL.h"
#include "../Tr2AdapterStructures.h"

#include "./util/GlobalDescriptorHeapAllocatorDx12.h"
#include "./util/DescriptorHeapViewDx12.h"
#include "./util/GpuMarkerBuffer.h"
#include "./util/GpuCrashTracker.h"
#include "upscaling/Tr2UpscalingALDx12.h"

#define USE_BORDERLESS_WINDOW 1

// JB: Enable this to get some more details on stuff being released
#define ENABLE_RELEASE_LATER_TAG 0
#define DUMP_RELEASE_LATER_TAGS 1
#if ENABLE_RELEASE_LATER_TAG
	#define _RT_STRINGIFY__(s) #s
	#define RT_STRINGIFY(s) _RT_STRINGIFY__(s)
//	#define RELEASE_LATER(ownerDevice, comObject) (ownerDevice)->ReleaseLater(comObject, __FILE__ ":" RT_STRINGIFY(__LINE__) " " #comObject);
	#define RELEASE_LATER(ownerDevice, comObject) \
		{ \
			char __dbo[256]; \
			sprintf_s(__dbo, __FILE__ ":" RT_STRINGIFY(__LINE__) " " #comObject " Frame(%i)", (ownerDevice)->GetCurrentBackBufferIndex()); \
			(ownerDevice)->ReleaseLater(comObject, __dbo); \
		}
#else
	#define RELEASE_LATER(ownerDevice, comObject) (ownerDevice)->ReleaseLater(comObject);
#endif


struct Tr2PresentParametersAL;
namespace TrinityALImpl
{
	class GenerateMipsResources;
}


class Tr2PrimaryRenderContextAL: public Tr2RenderContextAL
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

	void AddShaderBinaryToCrashTracker(const Tr2ShaderBytecodeAL& bytecode, const char* shaderPath);
	void UnRegisterFromCrashTracker(ID3D12GraphicsCommandList2* commandList);

	bool SupportsBindlessTextures() const;

	uint64_t GetRecordingFrameNumber() const;
	uint64_t GetRenderedFrameNumber() const;

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

	ALResult GetGpuStateMarker( Tr2RenderContextEnum::RenderContextStatus& status, std::string& marker, std::string& offendingShader ) const;
	ALResult GetGpuPageFaultResource(
		Tr2RenderContextEnum::PixelFormat& format,
		uint64_t& size,
		uint32_t& width,
		uint32_t& height,
		uint32_t& depth,
		uint32_t& mips ) const;

#if ENABLE_RELEASE_LATER_TAG
	void ReleaseLater(IUnknown* resource, const std::string& name);
#else
	void ReleaseLater(IUnknown* resource);
#endif
	void FlushPendingRelease();

	void OnShaderProgramDestroyedDx12( TrinityALImpl::Tr2ShaderProgramAL* sp );
	void OnVertexLayoutDestroyedDx12( TrinityALImpl::Tr2VertexLayoutAL* vl );
	ALResult FlushAndSyncDx12( Tr2RenderContextAL& renderContext );
	D3D12_CPU_DESCRIPTOR_HANDLE GetNullRtHandle( const Tr2TextureAL& compatibleWith );
	ALResult SignalDx12( uint64_t& fenceValue );
	uint64_t SignalDx12();
	ALResult WaitForFenceDx12( uint64_t value );
	const TrinityALImpl::GpuMarkerBuffer &GetMarkerBuffer() const;
	TrinityALImpl::GpuCrashTracker* GetGpuCrashTracker() const;

	void ScheduleSwapchainPresentDx12( IDXGISwapChain3* swapChain, const Tr2TextureAL& backbuffer, uint32_t presentInterval );


	ID3D12DescriptorHeap* GetGlobalSrvUavHeap() const;
	std::shared_ptr<ShaderResourceViewDx12> GetSrvHeapView() const;
	std::shared_ptr<UnorderedAccessViewDx12> GetUavHeapView() const;
	ID3D12DescriptorHeap* GetGlobalSamplerHeap() const;
	std::shared_ptr<SamplerStateDx12> GetSamplerHeapView() const;

	/** Create a ShaderResourceView */
	HRESULT CreateShaderResourceView(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, std::shared_ptr<ShaderResourceViewDx12>& srvView);

	/** Create an UnorderedAccessView */
	HRESULT CreateUnorderedAccessView(ID3D12Resource* resource, ID3D12Resource* counterResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc, std::shared_ptr<UnorderedAccessViewDx12>& uavView);

	/** Create a SamplerState */
	HRESULT CreateSamplerState(const D3D12_SAMPLER_DESC& desc, std::shared_ptr<SamplerStateDx12>& samplerState);

	/** Create a RenderTargetView */
	HRESULT CreateRenderTargetView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC* desc, std::shared_ptr<RenderTargetViewDx12>& rtvView);

	/** Create a DepthStencilView */
	HRESULT CreateDepthStencilView(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc, std::shared_ptr<DepthStencilViewDx12>& dsvView);

	/** Get the current buffer index */
	uint32_t GetCurrentBackBufferIndex() const { return m_currentBackBufferIndex; }
	uint32_t GetBackBufferCount() const
	{
		return uint32_t( m_frameFenceValues.size() );
	}

	bool FormatIsUAVCompatibleDx12( DXGI_FORMAT format ) const;
	std::shared_ptr<ShaderResourceViewDx12> GetNullSrvDx12( Tr2ShaderRegisterAL::RegisterType type ) const;
	std::shared_ptr<UnorderedAccessViewDx12> GetNullUavDx12( Tr2ShaderRegisterAL::RegisterType type ) const;
	std::shared_ptr<SamplerStateDx12> GetNullSamplerDx12() const;
	
	Tr2UpscalingAL::Result EnableUpscaling( Tr2UpscalingAL::Technique tech, Tr2UpscalingAL::Setting setting, bool framegeneration, uint32_t adapter );
	Tr2UpscalingContextAL* GetUpscalingContext( uint32_t upscalingContextID ) const;
	Tr2UpscalingContextAL* CreateUpscalingContext( Tr2UpscalingAL::UpscalingContextParams params , uint32_t existingContext = Tr2UpscalingAL::INVALID_CONTEXT_ID );
	void DeleteUpscalingContext( uint32_t contextID );
	Tr2UpscalingAL::UpscalingInfo GetUpscalingInfo( uint32_t upscalingContextID ) const;
	std::vector<std::tuple<Tr2UpscalingAL::Technique, uint32_t, bool>> GetSupportedUpscalingTechniques( uint32_t adapter );
	void GetUpscalingSetup( Tr2UpscalingAL::Technique& technique, Tr2UpscalingAL::Setting& setting, bool& framegeneration, bool& temporal ) const;

	void MarkFrameEvent( Tr2RenderContextEnum::FrameEvent frameEvent );
	
	HRESULT CreateSwapChainForHwnd(
		CComPtr<IDXGIFactory4>& factory4,
		ID3D12CommandQueue* commandQueue,
		HWND hWnd,
		const DXGI_SWAP_CHAIN_DESC1* pDesc,
		const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
		IDXGIOutput* pRestrictToOutput,
		CComPtr<IDXGISwapChain4>& swapchain );
	HRESULT CreateDevice( IUnknown* adapter, D3D_FEATURE_LEVEL featureLevel, CComPtr<ID3D12Device>& device ) const;
	HRESULT CreateCommandQueue( CComPtr<ID3D12Device>& device, D3D12_COMMAND_QUEUE_DESC* desc, CComPtr<ID3D12CommandQueue>& commandQueue ) const;
	HRESULT CreateFactory2( UINT flags, CComPtr<IDXGIFactory4>& factory ) const;

private:

	void ResetRenderTargets();

	Tr2PrimaryRenderContextAL( const Tr2PrimaryRenderContextAL& ) /* = delete */;
	Tr2PrimaryRenderContextAL& operator=( const Tr2PrimaryRenderContextAL& ) /* = delete */;

	void PopPendingRelease( size_t backBufferIndex );

	std::vector < CComPtr<ID3D12CommandAllocator>> m_commandAllocators;
	uint32_t m_commandAllocatorIndex;


	Tr2CapsAL m_caps;
	D3D12_FEATURE_DATA_D3D12_OPTIONS m_options = {};
	CComPtr<IDXGIOutput> m_output;

	uint32_t m_currentBackBufferIndex;

	CComPtr<ID3D12Fence> m_presentFence;
	HANDLE m_presentFenceEvent;

#if ENABLE_RELEASE_LATER_TAG
	struct ReleasePair
	{
		CComPtr<IUnknown> m_object;
		std::string m_name;
	};
	std::vector<std::vector<ReleasePair>> m_pendingRelease;
#else
	std::vector<std::vector<CComPtr<IUnknown>>> m_pendingRelease;
#endif

	uint64_t m_fenceValue;
	uint64_t m_completedFrameIndex;
	std::vector<uint64_t> m_frameFenceValues;
	

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

	struct NullRtDescHash
	{
		size_t operator()( const NullRtDesc& desc ) const
		{
			return desc;
		}
	};
	std::unordered_map<NullRtDesc, D3D12_CPU_DESCRIPTOR_HANDLE, NullRtDescHash> m_nullRts;

	TrinityALImpl::GpuMarkerBuffer m_immediateBuffer;
	TrinityALImpl::GpuCrashTracker* m_gpuCrashTracker;
	bool m_supportsVariableRefreshRate;

	CComPtr<ID3D12QueryHeap> m_statsQuery;
	CComPtr<ID3D12Resource> m_statsResult;
	uint64_t m_statsQueryFrameIndex;
	enum
	{
		STAT_READY,
		STAT_BEGIN_ISSUED,
		STAT_END_ISSUED,
	} m_statsStatus;

	Tr2GpuTimerAL m_frameTimer;

	struct PendingPresent
	{
		CComPtr<IDXGISwapChain3> swapChain;
		Tr2TextureAL backBuffer;
		uint32_t presentInterval;
	};
	std::vector<PendingPresent> m_pendingPresents;

	
	/** Descriptor heap specific content */
	enum
	{
		/*
		* NOTE: Total number of available descriptors will be PAGE_SIZE * NUM_PAGES
		* A page is backed by a descriptor heap when existing pages are full, so we should never hit the 'peak' amount
		*/

		CRVSRVUAV_PAGE_SIZE = 2048,
		CRVSRVUAV_NUM_PAGES = 1024,
		SAMPLER_PAGE_SIZE = 256,
		SAMPLER_NUM_PAGES = 32,
		RTV_PAGE_SIZE = 256,
		RTV_NUM_PAGES = 32,
		DSV_PAGE_SIZE = 256,
		DSV_NUM_PAGES = 32,

	};

	std::shared_ptr<GpuVisibleDescriptorAllocator> m_srvUavAllocator;
	std::shared_ptr<GpuVisibleDescriptorAllocator> m_samplerAllocator;
	std::shared_ptr<GlobalDescriptorHeapAllocator> m_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	std::shared_ptr<ShaderResourceViewDx12> m_srvHeapStart;
	std::shared_ptr<UnorderedAccessViewDx12> m_uavHeapStart;
	std::shared_ptr<SamplerStateDx12> m_samplerHeapStart;

	std::shared_ptr<ShaderResourceViewDx12> m_nullSrv[16];
	std::shared_ptr<UnorderedAccessViewDx12> m_nullUav[16];

	Tr2PresentParametersAL m_presentationParameters;
	uint32_t m_adapter;
	Tr2WindowHandle m_focusWindow;
	TrinityALImpl::Tr2UpscalingTechniqueDx12* m_upscalingTechnique;

	class MemoryWatchdog;
	std::unique_ptr<MemoryWatchdog> m_memoryWatchdog;

public:
	CComPtr<ID3D12Device> m_device;
	CComPtr<ID3D12Device1> m_device1;
	CComPtr<ID3D12Device5> m_device5;
	CComPtr<IDXGISwapChain4> m_swapchain;
	CComPtr<ID3D12CommandQueue> m_commandQueue;

	D3D_ROOT_SIGNATURE_VERSION m_rootSignatureVersion;

	typedef std::unordered_map<TrinityALImpl::PSODescription, CComPtr<ID3D12PipelineState>> PipelineStateMap;
	PipelineStateMap m_pipelineStates;

	CComPtr<ID3D12CommandSignature> m_drawInstancedIndirect;
	CComPtr<ID3D12CommandSignature> m_drawIndexedInstancedIndirect;
	CComPtr<ID3D12CommandSignature> m_dispatchIndirect;

	TrinityALImpl::Tr2ResourceHelper m_nullCB;

	void* m_amdExtDeviceObject;
};

#endif
