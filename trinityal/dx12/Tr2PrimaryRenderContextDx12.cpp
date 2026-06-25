// Copyright © 2019 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2PrimaryRenderContextDx12.h"
#include "ALLog.h"
#include "Tr2VideoAdapterInfoALDx12.h"
#include "ITr2RenderContextEvents.h"
#include "Utilities.h"
#include "Tr2SwapChainALDx12.h"
#include "Tr2TextureALDx12.h"
#include "util/AmdExtDevice.h"

extern bool g_requestDeviceDebugLayer;
extern bool g_requestDebugMarkers;
bool g_gatherPipelineStatistics = false;
extern ICrashReporter* TrinityALCrashes;

CCP_STATS_DECLARE( dx12IAVertices, "Trinity/AL/Pipeline/IAVertices", false, CST_COUNTER_HIGH, "Number of vertices read by input assembler" );
CCP_STATS_DECLARE( dx12IAPrimitives, "Trinity/AL/Pipeline/IAPrimitives", false, CST_COUNTER_HIGH, "Number of primitives read by input assembler" );
CCP_STATS_DECLARE( dx12VSInvocations, "Trinity/AL/Pipeline/VSInvocations", false, CST_COUNTER_HIGH, "Number of times a vertex shader was invoked" );
CCP_STATS_DECLARE( dx12GSInvocations, "Trinity/AL/Pipeline/GSInvocations", false, CST_COUNTER_HIGH, "Number of times a geometry shader was invoked" );
CCP_STATS_DECLARE( dx12GSPrimitives, "Trinity/AL/Pipeline/GSPrimitives", false, CST_COUNTER_HIGH, "Number of primitives output by a geometry shader" );
CCP_STATS_DECLARE( dx12CInvocations, "Trinity/AL/Pipeline/CInvocations", false, CST_COUNTER_HIGH, "Number of primitives that were sent to the rasterizer" );
CCP_STATS_DECLARE( dx12CPrimitives, "Trinity/AL/Pipeline/CPrimitives", false, CST_COUNTER_HIGH, "Number of primitives that were rendered" );
CCP_STATS_DECLARE( dx12PSInvocations, "Trinity/AL/Pipeline/PSInvocations", false, CST_COUNTER_HIGH, "Number of times a pixel shader was invoked" );
CCP_STATS_DECLARE( dx12HSInvocations, "Trinity/AL/Pipeline/HSInvocations", false, CST_COUNTER_HIGH, "Number of times a hull shader was invoked" );
CCP_STATS_DECLARE( dx12DSInvocations, "Trinity/AL/Pipeline/DSInvocations", false, CST_COUNTER_HIGH, "Number of times a domain shader was invoked" );
CCP_STATS_DECLARE( gpuFrameTime, "Trinity/AL/GpuFrameTime", false, CST_TIME, "Time spent on GPU processing a frame" );

CCP_STATS_DECLARE( dx12GpuMemoryUsageLocal, "Trinity/AL/gpuMemory/usageLocal", false, CST_MEMORY, "Local (L1) video memory usage as reported by DXGI" );
CCP_STATS_DECLARE( dx12GpuMemoryBudgetLocal, "Trinity/AL/gpuMemory/budgetLocal", false, CST_MEMORY, "Local (L1) video memory budget as reported by DXGI" );


namespace
{
bool EnableDebugLayer()
{
	CComPtr<ID3D12DeviceRemovedExtendedDataSettings1> pDredSettings;
	if( SUCCEEDED( D3D12GetDebugInterface( IID_PPV_ARGS( &pDredSettings ) ) ) )
	{
		// Turn on auto-breadcrumbs and page fault reporting.
		pDredSettings->SetAutoBreadcrumbsEnablement( D3D12_DRED_ENABLEMENT_FORCED_ON );
		pDredSettings->SetPageFaultEnablement( D3D12_DRED_ENABLEMENT_FORCED_ON );
	}


	CComPtr<ID3D12Debug> debugInterface;
	CComPtr<ID3D12Debug1> debugInterface1;
	if( SUCCEEDED( D3D12GetDebugInterface( IID_PPV_ARGS( &debugInterface ) ) ) )
	{
		debugInterface->EnableDebugLayer();
		if( SUCCEEDED( debugInterface->QueryInterface( IID_PPV_ARGS( &debugInterface1 ) ) ) )
		{
			debugInterface1->SetEnableGPUBasedValidation( true );
		}
		return true;
	}
	else
	{
		CCP_AL_LOGWARN( "DX12: No debug interface available, no error logging will be available" );
		return false;
	}
}

void SetupInfoQueue( ID3D12Device* device )
{
	CComQIPtr<ID3D12InfoQueue> queue( device );
	if( queue )
	{
		D3D12_MESSAGE_SEVERITY severity[] = {
			D3D12_MESSAGE_SEVERITY_CORRUPTION,
			D3D12_MESSAGE_SEVERITY_ERROR,
		};

		// JB: Added some message filters
		D3D12_MESSAGE_ID blocked[] = {
			// JB: Removed this because it pops up due to a bug
			D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES,

			// JB: This spam a lot and should be suppressed.
			// *HOWEVER* it would be a good idea to uncomment them and try to use the feature!
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE
		};

		D3D12_INFO_QUEUE_FILTER filter;
		memset( &filter, 0, sizeof( filter ) );
		filter.AllowList.NumSeverities = 2;
		filter.AllowList.pSeverityList = severity;
		filter.DenyList.pIDList = blocked;
		filter.DenyList.NumIDs = sizeof( blocked ) / sizeof( blocked[0] );
		CR( queue->PushRetrievalFilter( &filter ) );
		CR( queue->PushStorageFilter( &filter ) );
	}
}

size_t RegisterTypeIndex( Tr2ShaderRegisterAL::RegisterType type )
{
	return size_t( type ) & 31;
}

size_t s_perFrameMinReleaseCount = 100;
}


class Tr2PrimaryRenderContextAL::MemoryWatchdog
{
public:
	MemoryWatchdog( ID3D12Device* device, IDXGIAdapter1* adapter )
	{
		CComQIPtr<IDXGIAdapter3> adapter3( adapter );
		if( adapter3 )
		{
			m_adapter = adapter3;

			bool isUMA = false;
			D3D12_FEATURE_DATA_ARCHITECTURE architecture = {};
			if( SUCCEEDED( device->CheckFeatureSupport( D3D12_FEATURE_ARCHITECTURE, &architecture, sizeof( architecture ) ) ) )
			{
				isUMA = architecture.UMA;
			}
			CCP_LOGNOTICE( "Detected GPU device architecture to be %s", isUMA ? "UMA" : "NUMA" );

			DXGI_QUERY_VIDEO_MEMORY_INFO info;
			if( SUCCEEDED( adapter3->QueryVideoMemoryInfo( 0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info ) ) )
			{
				CCP_LOGNOTICE( "GPU memory budget: %uMB", uint32_t( info.Budget / 1024 / 1024 ) );

				CCP_STATS_SET( dx12GpuMemoryBudgetLocal, info.Budget );
				if( TrinityALCrashes )
				{
					TrinityALCrashes->SetCrashKeyValue( "gpuMemoryBudgetLocal", ( std::to_string( info.Budget / 1024 / 1024 ) + "MB" ).c_str() );
				}
			}
			else
			{
				CCP_LOGERR( "Failed to query video memory info" );
			}

			auto event = CreateEvent( nullptr, FALSE, FALSE, nullptr );
			DWORD cookie = 0;
			if( SUCCEEDED( adapter3->RegisterVideoMemoryBudgetChangeNotificationEvent( event, &cookie ) ) )
			{
				CCP_LOG( "Registered for video memory budget change notifications" );
				m_bugdetChangeEvent = event;
				m_budgetChangeCookie = cookie;

				m_stopEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
				m_watchdogThread = std::thread( [this] { this->Watcher(); } );
			}
			else
			{
				CCP_LOGWARN( "Failed to register for video memory budget change notifications" );
				CloseHandle( event );
			}
		}
	}

	~MemoryWatchdog()
	{
		if( m_watchdogThread.joinable() )
		{
			SetEvent( m_stopEvent );
			m_watchdogThread.join();
		}
		if( m_adapter && m_bugdetChangeEvent )
		{
			m_adapter->UnregisterVideoMemoryBudgetChangeNotification( m_budgetChangeCookie );
		}
		if( m_stopEvent )
		{
			CloseHandle( m_stopEvent );
		}
		if( m_bugdetChangeEvent )
		{
			CloseHandle( m_bugdetChangeEvent );
		}
	}

private:
	void Watcher()
	{
		while( true )
		{
			HANDLE handles[] = { m_bugdetChangeEvent, m_stopEvent };
			switch( WaitForMultipleObjects( 2, handles, FALSE, 1000 ) )
			{
			case WAIT_OBJECT_0:
			case WAIT_TIMEOUT:
				ReportMemory();
				break;
			default:
				return;
			}
		}
	}

	void ReportMemory()
	{
		DXGI_QUERY_VIDEO_MEMORY_INFO info;
		if( SUCCEEDED( m_adapter->QueryVideoMemoryInfo( 0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info ) ) )
		{
			auto budgetMB = uint32_t( info.Budget / 1024 / 1024 );
			auto usageMB = uint32_t( info.CurrentUsage / 1024 / 1024 );
			auto prevBudgetMB = uint32_t( m_budget / 1024 / 1024 );
			auto prevUsageMB = uint32_t( m_usage / 1024 / 1024 );
			if( info.Budget <= info.CurrentUsage )
			{
				if( budgetMB < prevBudgetMB || usageMB > prevUsageMB )
				{
					CCP_LOGERR( "Video memory over the budget. Current usage is %uMB and the budget is %uMB", usageMB, budgetMB );
				}
			}
			else if( info.Budget / 5 * 4 <= info.CurrentUsage )
			{
				if( budgetMB < prevBudgetMB || usageMB > prevUsageMB )
				{
					CCP_LOGWARN( "Video memory close to the budget. Current usage is %uMB and the budget is %uMB", usageMB, budgetMB );
				}
			}
			if( m_budget != 0 && info.Budget != m_budget )
			{
				CCP_LOGNOTICE( "Video memory budget has changed from %u to %u. Memory usage is %dMB", prevBudgetMB, budgetMB, usageMB );
			}
			m_budget = info.Budget;
			m_usage = info.CurrentUsage;

			CCP_STATS_SET( dx12GpuMemoryUsageLocal, info.CurrentUsage );
			CCP_STATS_SET( dx12GpuMemoryBudgetLocal, info.Budget );
			if( TrinityALCrashes )
			{
				TrinityALCrashes->SetCrashKeyValue( "gpuMemoryUsageLocal", ( std::to_string( usageMB ) + "MB" ).c_str() );
				TrinityALCrashes->SetCrashKeyValue( "gpuMemoryBudgetLocal", ( std::to_string( budgetMB ) + "MB" ).c_str() );
			}
		}
	}

	std::thread m_watchdogThread;
	CComPtr<IDXGIAdapter3> m_adapter;
	HANDLE m_bugdetChangeEvent = {};
	HANDLE m_stopEvent = {};
	UINT64 m_budget = 0;
	UINT64 m_usage = 0;
	DWORD m_budgetChangeCookie = 0;
};


Tr2PrimaryRenderContextAL::Tr2PrimaryRenderContextAL() :
	m_events( nullptr ),
	m_currentBackBufferIndex( 0 ),
	m_presentFenceEvent( nullptr ),
	m_syncInterval( 0 ),
	m_fenceValue( 0 ),
	m_rootSignatureVersion( D3D_ROOT_SIGNATURE_VERSION_1_0 ),
	m_genMipsResources( nullptr ),
	m_commandAllocatorIndex( 0 ),
	m_statsQueryFrameIndex( 0 ),
	m_statsStatus( STAT_READY ),
	m_completedFrameIndex( 0 ),
	m_amdExtDeviceObject( 0 ),
	m_gpuCrashTracker( nullptr ),
	m_presentationParameters( {} ),
	m_adapter( 0 ),
	m_focusWindow( 0 ),
	m_upscalingTechnique( nullptr )

{
	m_ownerDevice = this;
	m_defaultBackBuffer.m_texture = std::make_shared<TrinityALImpl::Tr2TextureAL>();
	m_supportsVariableRefreshRate = m_caps.SupportsVariableRefreshRate();
}

Tr2PrimaryRenderContextAL::~Tr2PrimaryRenderContextAL()
{
	Destroy();

	if( m_upscalingTechnique )
	{
		delete m_upscalingTechnique;
		m_upscalingTechnique = nullptr;
	}
}


ALResult Tr2PrimaryRenderContextAL::CreateDevice(
	uint32_t adapter,
	Tr2WindowHandle focusWindow,
	const Tr2PresentParametersAL& pp )
{
	Destroy();
	auto presentationParameters = pp;
	presentationParameters.variableRefreshRateSupported = m_supportsVariableRefreshRate;
	m_presentationParameters = presentationParameters;
	m_focusWindow = focusWindow;
	m_adapter = adapter;

	bool hasDebugLayer = false;
	if( g_requestDeviceDebugLayer )
	{
		hasDebugLayer = EnableDebugLayer();
	}

	CComPtr<ID3D12Device> device;
	CComPtr<IDXGIAdapter1> dxgiAdapter;
	CComPtr<IDXGIOutput> output;

	if( !m_gpuCrashTracker )
	{
		m_gpuCrashTracker = new TrinityALImpl::GpuCrashTracker();
	}
	CR_RETURN_HR( TrinityALImpl::GetVideoAdapter( adapter, &dxgiAdapter, &output ) );

	bool dxrAvailable = false;
	// Create directX 12.1 device for raytracing, if it fails fall back to 12.0
	if( SUCCEEDED( CreateDevice( dxgiAdapter, D3D_FEATURE_LEVEL_12_1, device ) ) )
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 caps = {};
		if( FAILED( device->CheckFeatureSupport( D3D12_FEATURE_D3D12_OPTIONS5, &caps, sizeof( caps ) ) ) || caps.RaytracingTier < D3D12_RAYTRACING_TIER_1_1 )
		{
			CCP_LOGNOTICE( "DirectX device or driver does not support raytracing" );
		}
		else
		{
			CCP_LOGNOTICE( "Successfully created DirectX 12.1 device, ray tracing will be available" );
			dxrAvailable = true;
		}
	}
	else
	{
		CCP_LOGNOTICE( "Failed to create DirectX 12.1 device, now creating DirectX 12.0 device instead" );
		CR_RETURN_HR( CreateDevice( dxgiAdapter, D3D_FEATURE_LEVEL_12_0, device ) );
	}

	if( !m_gpuCrashTracker->IsValid() )
	{
		m_gpuCrashTracker = nullptr;
	}
	else
	{
		m_gpuCrashTracker->Initialize( device );
	}

	if( hasDebugLayer )
	{
		SetupInfoQueue( device );
	}

	// Init descriptor heap allocators
	struct DescriptorHeapLayout
	{
		D3D12_DESCRIPTOR_HEAP_TYPE m_type;
		uint32_t m_pageSize;
		uint32_t m_numPages;
	} heapLayout[] = {
		{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, CRVSRVUAV_PAGE_SIZE, CRVSRVUAV_NUM_PAGES },
		{ D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, SAMPLER_PAGE_SIZE, SAMPLER_NUM_PAGES },
		{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV, RTV_PAGE_SIZE, RTV_NUM_PAGES },
		{ D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DSV_PAGE_SIZE, DSV_NUM_PAGES }
	};

	for( int32_t idx = 0; idx < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++idx )
	{
		CCP_ASSERT( heapLayout[idx].m_type == D3D12_DESCRIPTOR_HEAP_TYPE( idx ) );

		m_allocators[idx] = std::make_shared<GlobalDescriptorHeapAllocator>( device, heapLayout[idx].m_numPages, heapLayout[idx].m_pageSize, heapLayout[idx].m_type );
		CCP_ASSERT( m_allocators[idx].get() != nullptr );
	}

	m_srvUavAllocator = std::make_shared<GpuVisibleDescriptorAllocator>( device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16 * 1024 );
	if( auto entry = m_srvUavAllocator->Allocate() )
	{
		m_srvHeapStart = std::make_shared<ShaderResourceViewDx12>( m_srvUavAllocator.get(), entry );
		m_uavHeapStart = std::make_shared<UnorderedAccessViewDx12>( m_srvUavAllocator.get(), entry );
	}

	m_samplerAllocator = std::make_shared<GpuVisibleDescriptorAllocator>( device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1024 );
	if( auto entry = m_samplerAllocator->Allocate() )
	{
		D3D12_SAMPLER_DESC nullSamplerDesc = { D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, 0, D3D12_COMPARISON_FUNC_ALWAYS };
		device->CreateSampler( &nullSamplerDesc, entry->m_offsetCPU );
		m_samplerHeapStart = std::make_shared<SamplerStateDx12>( m_samplerAllocator.get(), entry );
	}

	CComPtr<ID3D12CommandQueue> commandQueue;

	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	CR_RETURN_HR( CreateCommandQueue( device, &desc, commandQueue ) );

	const bool isWindowless = ( focusWindow == 0 ) && presentationParameters.software;

	CComPtr<IDXGISwapChain4> swapchain;
	uint32_t backBufferCount = 1;
	if( !isWindowless )
	{
		backBufferCount = Tr2SwapChainUtils::BACK_BUFFER_COUNT;
		FORWARD_HR( Tr2SwapChainUtils::CreateSwapChain( swapchain, focusWindow, presentationParameters, commandQueue, output, *this ) );
	}


	std::vector<std::shared_ptr<RenderTargetViewDx12>> rtvs;
	std::vector<CComPtr<ID3D12Resource>> backBuffers;

	{
		// JB: Not pretty, but createRenderTargetView requires a valid device!
		m_device = device;
		HRESULT hr = TrinityALImpl::Tr2SwapChainAL::GetBackBuffers( this, backBuffers, rtvs, swapchain );
		m_device = nullptr;

		FORWARD_HR( hr );
	}

	std::vector<CComPtr<ID3D12CommandAllocator>> commandAllocators;
	for( uint32_t i = 0; i < backBufferCount; ++i )
	{
		CComPtr<ID3D12CommandAllocator> commandAllocator;
		CR_RETURN_HR( device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &commandAllocator ) ) );
		commandAllocators.push_back( commandAllocator );
	}

	uint32_t currentBackBufferIndex;
	if( swapchain )
	{
		currentBackBufferIndex = swapchain->GetCurrentBackBufferIndex();
	}
	else
	{
		currentBackBufferIndex = 0;
	}

	CComPtr<ID3D12Fence> fence;
	CR_RETURN_HR( device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &fence ) ) );


	CComPtr<ID3D12DescriptorHeap> nullRtHeap;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 256;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	CR_RETURN_HR( device->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( &nullRtHeap ) ) );

	CComPtr<ID3D12CommandSignature> drawInstancedIndirect;
	CComPtr<ID3D12CommandSignature> drawIndexedInstancedIndirect;
	CComPtr<ID3D12CommandSignature> dispatchIndirect;

	D3D12_INDIRECT_ARGUMENT_DESC indirectArg;
	indirectArg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

	D3D12_COMMAND_SIGNATURE_DESC signature;
	signature.ByteStride = sizeof( D3D12_DRAW_ARGUMENTS );
	signature.NodeMask = 0;
	signature.NumArgumentDescs = 1;
	signature.pArgumentDescs = &indirectArg;

	CR_RETURN_HR( device->CreateCommandSignature( &signature, nullptr, IID_PPV_ARGS( &drawInstancedIndirect ) ) );
	signature.ByteStride = sizeof( D3D12_DRAW_INDEXED_ARGUMENTS );
	indirectArg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
	CR_RETURN_HR( device->CreateCommandSignature( &signature, nullptr, IID_PPV_ARGS( &drawIndexedInstancedIndirect ) ) );
	signature.ByteStride = sizeof( D3D12_DISPATCH_ARGUMENTS );
	indirectArg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
	CR_RETURN_HR( device->CreateCommandSignature( &signature, nullptr, IID_PPV_ARGS( &dispatchIndirect ) ) );


	m_device = device;
	device.QueryInterface( &m_device1 );
	device.QueryInterface( &m_device5 );
	m_commandQueue = commandQueue;
	m_swapchain = swapchain;
	m_output = output;

	m_commandAllocators = commandAllocators;
	m_commandAllocatorIndex = 0;
	m_currentBackBufferIndex = currentBackBufferIndex;

	m_immediateBuffer.Create( m_device );

	m_presentFence = fence;
	m_presentFenceEvent = ::CreateEvent( NULL, FALSE, FALSE, NULL );

	m_completedFrameIndex = 0;
	m_fenceValue = 1;
	m_srvUavAllocator->SetFrameIndices( GetRecordingFrameNumber(), GetRenderedFrameNumber() );
	m_frameFenceValues.clear();
	m_frameFenceValues.resize( backBufferCount, 0 );
	m_pendingRelease.resize( backBufferCount );

	m_caps.m_supportsDxr = dxrAvailable;

	CreateDx12( commandAllocators[currentBackBufferIndex], *this );

	m_syncInterval = presentationParameters.presentInterval;

	m_defaultBackBuffer.m_texture->AssignFromSwapChainDx12( backBuffers, rtvs, *this );

	m_nullRtHeap = nullRtHeap;

	m_drawInstancedIndirect = drawInstancedIndirect;
	m_drawIndexedInstancedIndirect = drawIndexedInstancedIndirect;
	m_dispatchIndirect = dispatchIndirect;

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if( SUCCEEDED( m_device->CheckFeatureSupport( D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof( featureData ) ) ) )
	{
		m_rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	}
	else
	{
		m_rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	if( g_requestDebugMarkers )
	{
		DXGI_ADAPTER_DESC adapterDesc;
		if( SUCCEEDED( dxgiAdapter->GetDesc( &adapterDesc ) ) && adapterDesc.VendorId == 4098 )
		{
			if( HMODULE amdD3dDl2 = LoadLibrary( "amdxc64.dll" ) )
			{
				auto amdExtD3dCreateFunc = (PFNAmdExtD3DCreateInterface)GetProcAddress( amdD3dDl2, "AmdExtD3DCreateInterface" );

				if( amdExtD3dCreateFunc )
				{
					IAmdExtD3DFactory* amdExtObject = nullptr;
					amdExtD3dCreateFunc( m_device, __uuidof( IAmdExtD3DFactory ), reinterpret_cast<void**>( &amdExtObject ) );

					if( amdExtObject )
					{
						amdExtObject->CreateInterface( m_device, __uuidof( IAmdExtD3DDevice1 ), &m_amdExtDeviceObject );
						amdExtObject->Release();
					}
				}
				FreeLibrary( amdD3dDl2 );
			}
		}
	}

	if( m_events )
	{
		m_events->OnContextCreated( *this );
	}

	m_defaultBackBuffer.m_texture->SetSwapChainBufferIndexDx12( m_currentBackBufferIndex );

	auto commandAllocator = m_commandAllocators[m_commandAllocatorIndex++ % m_commandAllocators.size()];
	commandAllocator->Reset();
	m_commandList->Reset( commandAllocator, nullptr );

	ResetRenderTargets();

	m_genMipsResources = new TrinityALImpl::GenerateMipsResources( m_device );

	const uint32_t nullCbSize = 128 * 1024;
	std::vector<uint8_t> cb( nullCbSize );
	D3D12_SUBRESOURCE_DATA cbd = { cb.data(), nullCbSize, nullCbSize };
	m_nullCB.Create( TrinityALImpl::Tr2ResourceHelper::STATIC, nullCbSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, false, 1, &cbd, *this );

	m_immediateBuffer.PutMarker( m_commandList2, "" );
	m_statsQueryFrameIndex = 0;
	m_statsStatus = STAT_READY;

	{
		Tr2AdapterInfo info;
		Tr2VideoAdapterInfo::GetAdapterInfo( adapter, info );
		CCP_AL_LOGNOTICE( "Created dx12 device for adapter %S", info.description.c_str() );
	}

	m_options = {};
	m_device->CheckFeatureSupport( D3D12_FEATURE_D3D12_OPTIONS, &m_options, sizeof( m_options ) );


	{
		D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc = {};
		nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		nullSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
		nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		nullSrvDesc.Buffer = { 0, 1, 4 };
		CreateShaderResourceView( nullptr, nullSrvDesc, m_nullSrv[RegisterTypeIndex( Tr2ShaderRegisterAL::SRV_BUFFER )] );
		CreateShaderResourceView( nullptr, nullSrvDesc, m_nullSrv[RegisterTypeIndex( Tr2ShaderRegisterAL::SRV_STRUCTURED_BUFFER )] );
		nullSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
		nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
		nullSrvDesc.Texture1D = { 0, 1 };
		CreateShaderResourceView( nullptr, nullSrvDesc, m_nullSrv[RegisterTypeIndex( Tr2ShaderRegisterAL::SRV_TEXTURE1D )] );
		nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
		nullSrvDesc.Texture1DArray = { 0, 1, 0, 1 };
		CreateShaderResourceView( nullptr, nullSrvDesc, m_nullSrv[RegisterTypeIndex( Tr2ShaderRegisterAL::SRV_TEXTURE1DARRAY )] );
		nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		nullSrvDesc.Texture2D = { 0, 1 };
		CreateShaderResourceView( nullptr, nullSrvDesc, m_nullSrv[RegisterTypeIndex( Tr2ShaderRegisterAL::SRV_TEXTURE2D )] );
		nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		nullSrvDesc.Texture2DArray = { 0, 1, 0, 1 };
		CreateShaderResourceView( nullptr, nullSrvDesc, m_nullSrv[RegisterTypeIndex( Tr2ShaderRegisterAL::SRV_TEXTURE2DARRAY )] );
		nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
		CreateShaderResourceView( nullptr, nullSrvDesc, m_nullSrv[RegisterTypeIndex( Tr2ShaderRegisterAL::SRV_TEXTURE2DMS )] );
		nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
		nullSrvDesc.Texture2DMSArray = { 0, 1 };
		CreateShaderResourceView( nullptr, nullSrvDesc, m_nullSrv[RegisterTypeIndex( Tr2ShaderRegisterAL::SRV_TEXTURE2DMSARRAY )] );
		nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
		nullSrvDesc.Texture3D = { 0, 1 };
		CreateShaderResourceView( nullptr, nullSrvDesc, m_nullSrv[RegisterTypeIndex( Tr2ShaderRegisterAL::SRV_TEXTURE3D )] );
		nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		nullSrvDesc.TextureCube = { 0, 1 };
		CreateShaderResourceView( nullptr, nullSrvDesc, m_nullSrv[RegisterTypeIndex( Tr2ShaderRegisterAL::SRV_TEXTURECUBE )] );
		nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
		nullSrvDesc.TextureCubeArray = { 0, 1, 0, 1 };
		CreateShaderResourceView( nullptr, nullSrvDesc, m_nullSrv[RegisterTypeIndex( Tr2ShaderRegisterAL::SRV_TEXTURECUBEARRAY )] );

		D3D12_UNORDERED_ACCESS_VIEW_DESC nullUavDesc = {};

		nullUavDesc.Format = DXGI_FORMAT_UNKNOWN;
		nullUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		nullUavDesc.Buffer = { 0, 1, 4 };
		CreateUnorderedAccessView( nullptr, nullptr, nullUavDesc, m_nullUav[RegisterTypeIndex( Tr2ShaderRegisterAL::SRV_BUFFER )] );
		CreateUnorderedAccessView( nullptr, nullptr, nullUavDesc, m_nullUav[RegisterTypeIndex( Tr2ShaderRegisterAL::SRV_STRUCTURED_BUFFER )] );
		nullUavDesc.Format = DXGI_FORMAT_R32_FLOAT;
		nullUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
		nullUavDesc.Texture1D = {};
		CreateUnorderedAccessView( nullptr, nullptr, nullUavDesc, m_nullUav[RegisterTypeIndex( Tr2ShaderRegisterAL::SRV_TEXTURE1D )] );
		nullUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
		nullUavDesc.Texture1DArray = { 0, 0, 1 };
		CreateUnorderedAccessView( nullptr, nullptr, nullUavDesc, m_nullUav[RegisterTypeIndex( Tr2ShaderRegisterAL::SRV_TEXTURE1DARRAY )] );
		nullUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		nullUavDesc.Texture2D = {};
		CreateUnorderedAccessView( nullptr, nullptr, nullUavDesc, m_nullUav[RegisterTypeIndex( Tr2ShaderRegisterAL::SRV_TEXTURE2D )] );
		nullUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		nullUavDesc.Texture2DArray = { 0, 0, 1 };
		CreateUnorderedAccessView( nullptr, nullptr, nullUavDesc, m_nullUav[RegisterTypeIndex( Tr2ShaderRegisterAL::SRV_TEXTURE2DARRAY )] );
		nullUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
		nullUavDesc.Texture3D = { 0, 0, 1 };
		CreateUnorderedAccessView( nullptr, nullptr, nullUavDesc, m_nullUav[RegisterTypeIndex( Tr2ShaderRegisterAL::SRV_TEXTURE3D )] );
	}

	m_frameTimer.Create( *this );

	m_memoryWatchdog = std::make_unique<MemoryWatchdog>( device, dxgiAdapter );

	return S_OK;
}


void Tr2PrimaryRenderContextAL::Destroy()
{
	if( IsValid() )
	{
		uint64_t value;
		if( SUCCEEDED( SignalDx12( value ) ) )
		{
			WaitForFenceDx12( value );
		}
	}

	if( m_upscalingTechnique )
	{
		m_upscalingTechnique->ReleaseResources();
	}

	std::fill( begin( m_nullSrv ), end( m_nullSrv ), nullptr );
	std::fill( begin( m_nullUav ), end( m_nullUav ), nullptr );

	if( m_amdExtDeviceObject )
	{
		static_cast<IAmdExtD3DDevice1*>( m_ownerDevice->m_amdExtDeviceObject )->Release();
		m_amdExtDeviceObject = nullptr;
	}

	m_pendingPresents.clear();

	m_statsQuery = nullptr;
	m_statsResult = nullptr;

	Tr2RenderContextAL::Destroy();

	m_immediateBuffer.Destroy();

	delete m_genMipsResources;
	m_genMipsResources = nullptr;

	m_nullRtHeap = nullptr;
	m_nullRts.clear();

	m_drawInstancedIndirect = nullptr;
	m_drawIndexedInstancedIndirect = nullptr;
	m_dispatchIndirect = nullptr;

	m_pipelineStates.clear();
	m_defaultBackBuffer.m_texture->Destroy();

	m_presentFence = nullptr;

	m_commandAllocators.clear();

	if( m_swapchain )
	{
		CR( m_swapchain->SetFullscreenState( FALSE, nullptr ) );
	}
	m_swapchain = nullptr;

	m_commandQueue = nullptr;
	FlushPendingRelease();

	// JB: Forcing the destruction of samplers because they now hold a SamplerStateDx12 object
	m_samplerStateFactory.Clear();
	m_srvHeapStart = nullptr;
	m_uavHeapStart = nullptr;
	m_srvUavAllocator = nullptr;
	m_samplerHeapStart = nullptr;
	m_samplerAllocator = nullptr;
	for( int32_t idx = 0; idx < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++idx )
	{
		m_allocators[idx] = nullptr;
	}

	m_device5 = nullptr;
	m_device1 = nullptr;
	m_device = nullptr;

	if( m_presentFenceEvent )
	{
		CloseHandle( m_presentFenceEvent );
		m_presentFenceEvent = nullptr;
	}

	m_completedFrameIndex = 0;
	m_fenceValue = 0;
	m_frameFenceValues.clear();

	m_rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	m_pendingRelease.clear();

	if( m_gpuCrashTracker )
	{
		delete m_gpuCrashTracker;
		m_gpuCrashTracker = nullptr;
	}

	m_memoryWatchdog.reset();

	TrinityALImpl::Destroy();
}

bool Tr2PrimaryRenderContextAL::IsValid() const
{
	return m_device != nullptr;
}

ALResult Tr2PrimaryRenderContextAL::SetPresentParameters( uint32_t adapter, const Tr2PresentParametersAL& presentationParameters )
{
	if( !m_swapchain )
	{
		return E_FAIL;
	}

	CComPtr<IDXGIOutput> dxgiOutput;
	CComPtr<IDXGIAdapter1> adapterPtr;
	auto hr = TrinityALImpl::GetVideoAdapter( adapter, &adapterPtr, &dxgiOutput );
	if( FAILED( hr ) )
	{
		return hr;
	}

	if( m_statsQuery && m_statsStatus == STAT_BEGIN_ISSUED )
	{
		m_commandList->EndQuery( m_statsQuery, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, 0 );
		m_statsStatus = STAT_READY;
	}
	FlushBarriersDx12();
	if( SUCCEEDED( m_commandList->Close() ) )
	{
		ID3D12CommandList* const commandLists[] = { m_commandList };
		m_commandQueue->ExecuteCommandLists( _countof( commandLists ), commandLists );
	}

	//if( !presentationParameters.windowed && dxgiOutput != m_output )
	//{
	//	// If we are switching between two monitors in fullscreen mode it seems we first should
	//	// go windowed and then fullscreen on another monitor, otherwise DXGI behaves funny.
	//	CR( m_swapChain->SetFullscreenState( FALSE, nullptr ) );
	//}

	m_output = dxgiOutput;

	DXGI_FORMAT fmt = Tr2SwapChainUtils::SafeConvertD3DBackBufferFormat( presentationParameters.mode.format );

	DXGI_MODE_DESC modeDesc;
	modeDesc.Width = presentationParameters.mode.width;
	modeDesc.Height = presentationParameters.mode.height;
	modeDesc.RefreshRate.Numerator = 0;
	modeDesc.RefreshRate.Denominator = 0;
	modeDesc.Format = fmt;
	modeDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER( presentationParameters.mode.scanlineOrdering );
	modeDesc.Scaling = DXGI_MODE_SCALING( presentationParameters.mode.scaling );

	m_defaultBackBuffer.m_texture->Destroy();

	WaitForFenceDx12( SignalDx12() );
	for( auto it = begin( m_pendingRelease ); it != end( m_pendingRelease ); ++it )
	{
		it->clear();
	}
	m_srvUavAllocator->SetFrameIndices( GetRecordingFrameNumber(), GetRenderedFrameNumber() );

	auto value = m_frameFenceValues[m_currentBackBufferIndex];
	m_frameFenceValues.clear();
	m_frameFenceValues.resize( Tr2SwapChainUtils::BACK_BUFFER_COUNT, value + 1 );

	UINT resizeFlags = 0;

	if( m_supportsVariableRefreshRate )
	{
		resizeFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	}

	// Dx12 is never in proper fullscreen
	CR( m_swapchain->ResizeBuffers( Tr2SwapChainUtils::BACK_BUFFER_COUNT,
									presentationParameters.mode.width,
									presentationParameters.mode.height,
									fmt,
									resizeFlags ) );

	m_syncInterval = presentationParameters.presentInterval;

	std::vector<std::shared_ptr<RenderTargetViewDx12>> rtvs;
	std::vector<CComPtr<ID3D12Resource>> backBuffers;
	hr = TrinityALImpl::Tr2SwapChainAL::GetBackBuffers( this, backBuffers, rtvs, m_swapchain );
	if( FAILED( hr ) )
	{
		auto commandAllocator = m_commandAllocators[m_commandAllocatorIndex++ % m_commandAllocators.size()];
		commandAllocator->Reset();
		m_commandList->Reset( commandAllocator, nullptr );

		return hr;
	}

	m_currentBackBufferIndex = m_swapchain->GetCurrentBackBufferIndex();
	m_defaultBackBuffer.m_texture->AssignFromSwapChainDx12( backBuffers, rtvs, *this );
	m_defaultBackBuffer.m_texture->SetSwapChainBufferIndexDx12( m_currentBackBufferIndex );

	auto commandAllocator = m_commandAllocators[m_commandAllocatorIndex++ % m_commandAllocators.size()];
	commandAllocator->Reset();
	m_commandList->Reset( commandAllocator, nullptr );

	ResetDescriptorCaches();
	ResetRenderTargets();

	m_frameTimer.Create( *this );

	return S_OK;
}

const Tr2CapsAL& Tr2PrimaryRenderContextAL::GetCaps() const
{
	return m_caps;
}

ALResult Tr2PrimaryRenderContextAL::Present()
{
	// some swap chains scheduled for presenting might have already been destroyed
	m_pendingPresents.erase(
		std::remove_if( begin( m_pendingPresents ), end( m_pendingPresents ), []( const PendingPresent& p ) -> bool { return !p.backBuffer.IsValid(); } ),
		end( m_pendingPresents ) );

	SetResourceSet( Tr2ResourceSetAL() );

	{
		std::vector<D3D12_RESOURCE_BARRIER> barriers;
		barriers.reserve( 1 + m_pendingPresents.size() );
		barriers.push_back( TrinityALImpl::Transition( m_defaultBackBuffer.m_texture->GetResourceDx12(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT ) );
		for( auto it = begin( m_pendingPresents ); it != end( m_pendingPresents ); ++it )
		{
			barriers.push_back( TrinityALImpl::Transition( it->backBuffer.m_texture->GetResourceDx12(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT ) );
		}
		ResourceBarrierDx12( barriers.size(), barriers.data() );
		FlushBarriersDx12();
	}
	ResetDx12();

	if( g_gatherPipelineStatistics && m_statsQuery )
	{
		if( m_statsStatus == STAT_BEGIN_ISSUED )
		{
			m_commandList->EndQuery( m_statsQuery, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, 0 );
			m_commandList->ResolveQueryData( m_statsQuery, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, 0, 1, m_statsResult, 0 );
			m_statsStatus = STAT_END_ISSUED;
			m_statsQueryFrameIndex = GetRecordingFrameNumber();
		}

		if( m_statsStatus == STAT_END_ISSUED && GetRenderedFrameNumber() >= m_statsQueryFrameIndex )
		{
			D3D12_QUERY_DATA_PIPELINE_STATISTICS stats;
			void* data;
			if( SUCCEEDED( m_statsResult->Map( 0, nullptr, &data ) ) )
			{
				memcpy( &stats, data, sizeof( stats ) );
				D3D12_RANGE writeRange = { 0, 0 };
				m_statsResult->Unmap( 0, &writeRange );

				CCP_STATS_SET( dx12IAVertices, stats.IAVertices );
				CCP_STATS_SET( dx12IAPrimitives, stats.IAPrimitives );
				CCP_STATS_SET( dx12VSInvocations, stats.VSInvocations );
				CCP_STATS_SET( dx12GSInvocations, stats.GSInvocations );
				CCP_STATS_SET( dx12GSPrimitives, stats.GSPrimitives );
				CCP_STATS_SET( dx12CInvocations, stats.CInvocations );
				CCP_STATS_SET( dx12CPrimitives, stats.CPrimitives );
				CCP_STATS_SET( dx12PSInvocations, stats.PSInvocations );
				CCP_STATS_SET( dx12HSInvocations, stats.HSInvocations );
				CCP_STATS_SET( dx12DSInvocations, stats.DSInvocations );

				m_statsStatus = STAT_READY;
			}
		}
	}

	m_frameTimer.End( *this );
	auto frameTime = m_frameTimer.GetTime( *this );
	if( frameTime > 0 )
	{
#if CCP_STATS_ENABLED
		g_ccpStatistics_gpuFrameTime.Set( frameTime );
#endif
	}



	CR( m_commandList->Close() );


	ID3D12CommandList* const commandLists[] = { m_commandList };
	m_commandQueue->ExecuteCommandLists( _countof( commandLists ), commandLists );
	m_frameFenceValues[m_currentBackBufferIndex] = SignalDx12();

	auto presentFlag = ( m_syncInterval == Tr2RenderContextEnum::PRESENT_INTERVAL_IMMEDIATE && m_supportsVariableRefreshRate ) ? DXGI_PRESENT_ALLOW_TEARING : 0;

	CR( m_swapchain->Present( m_syncInterval, presentFlag ) );
	FORWARD_HR( m_device->GetDeviceRemovedReason(); )
	for( auto it = begin( m_pendingPresents ); it != end( m_pendingPresents ); ++it )
	{
		it->swapChain->Present( it->presentInterval, 0 );
		FORWARD_HR( m_device->GetDeviceRemovedReason(); )
	}

	m_currentBackBufferIndex = m_swapchain->GetCurrentBackBufferIndex();
	FORWARD_HR( WaitForFenceDx12( m_frameFenceValues[m_currentBackBufferIndex] ) );

	PopPendingRelease( m_currentBackBufferIndex );
	m_srvUavAllocator->SetFrameIndices( GetRecordingFrameNumber(), GetRenderedFrameNumber() );

	// JB: As with creation, if additional render contexts are added, this will need to be in that contexts reset function
	m_descriptorCache[m_currentBackBufferIndex]->Reset();

	m_defaultBackBuffer.m_texture->SetSwapChainBufferIndexDx12( m_currentBackBufferIndex );

	auto commandAllocator = m_commandAllocators[m_commandAllocatorIndex++ % m_commandAllocators.size()];
	commandAllocator->Reset();
	m_commandList->Reset( commandAllocator, nullptr );

	if( !m_pendingPresents.empty() )
	{
		std::vector<D3D12_RESOURCE_BARRIER> barriers;
		barriers.reserve( m_pendingPresents.size() );
		for( auto it = begin( m_pendingPresents ); it != end( m_pendingPresents ); ++it )
		{
			it->backBuffer.m_texture->SetSwapChainBufferIndexDx12( it->swapChain->GetCurrentBackBufferIndex() );
			barriers.push_back( TrinityALImpl::Transition( it->backBuffer.m_texture->GetResourceDx12(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET ) );
		}
		ResourceBarrierDx12( barriers.size(), barriers.data() );

		// 	FlushBarriersDx12(); ?
	}

	for( auto it = begin( m_pendingPresents ); it != end( m_pendingPresents ); ++it )
	{
		RELEASE_LATER( this, it->swapChain );
	}
	m_pendingPresents.clear();

	ResetRenderTargets();

	m_frameTimer.Begin( *this );

	if( g_gatherPipelineStatistics )
	{
		if( !m_statsQuery )
		{
			D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
			queryHeapDesc.Count = 1;
			queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
			CR( m_device->CreateQueryHeap( &queryHeapDesc, IID_PPV_ARGS( &m_statsQuery ) ) );

			auto scratchHeap = TrinityALImpl::HeapDesc( D3D12_HEAP_TYPE_READBACK );
			auto bufferDesc = TrinityALImpl::BufferDesc( sizeof( D3D12_QUERY_DATA_PIPELINE_STATISTICS ) );
			CR( m_device->CreateCommittedResource(
				&scratchHeap,
				D3D12_HEAP_FLAG_NONE,
				&bufferDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS( &m_statsResult ) ) );
			if( m_statsResult )
			{
				m_statsStatus = STAT_READY;
			}
			else
			{
				m_statsQuery = nullptr;
			}
		}
		if( m_statsStatus == STAT_READY )
		{
			m_commandList->BeginQuery( m_statsQuery, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, 0 );
			m_statsStatus = STAT_BEGIN_ISSUED;
		}
	}

	SetRenderTarget( m_defaultBackBuffer );
	return S_OK;
}

void Tr2PrimaryRenderContextAL::PopPendingRelease( size_t backBufferIndex )
{
	auto& releaseBuffer = m_pendingRelease[backBufferIndex];

	if( releaseBuffer.size() == 0 )
	{
		return;
	}

	// Release an eighth of the buffer per frame or the preferred release count, which one is larger so we will eventually release all things
	size_t releaseCount = std::max( s_perFrameMinReleaseCount, releaseBuffer.size() / 8 );
	releaseCount = std::min( releaseCount, releaseBuffer.size() );
#if ENABLE_RELEASE_LATER_TAG && DUMP_RELEASE_LATER_TAGS
	CCP_LOG( "Release buffer[%zu] size %zu, releasing %zu", backBufferIndex, releaseBuffer.size(), releaseCount );
	for( auto r = releaseBuffer.end() - releaseCount; r != releaseBuffer.end(); r++ )
	{
		char dbo[512];
		sprintf_s( dbo, "Releasing %zu: %s\n", r - releaseBuffer.begin(), r->m_name.c_str() );

		OutputDebugStringA( dbo );
		CCP_LOG( dbo );

		r->m_object = nullptr;
	}
#endif
	releaseBuffer.erase( releaseBuffer.end() - releaseCount, releaseBuffer.end() );
#if ENABLE_RELEASE_LATER_TAG && DUMP_RELEASE_LATER_TAGS
	CCP_LOG( "Release buffer[%zu] size is %zu after releasing %zu", backBufferIndex, releaseBuffer.size(), releaseCount );
#endif
}

Tr2RenderContextEnum::PixelFormat Tr2PrimaryRenderContextAL::GetBackBufferFormat() const
{
	return m_defaultBackBuffer.GetFormat();
}

ALResult Tr2PrimaryRenderContextAL::SignalDx12( uint64_t& fenceValue )
{
	fenceValue = ++m_fenceValue;
	return m_commandQueue->Signal( m_presentFence, fenceValue );
}

uint64_t Tr2PrimaryRenderContextAL::SignalDx12()
{
	uint64_t value;
	CR( SignalDx12( value ) );
	return value;
}

ALResult Tr2PrimaryRenderContextAL::WaitForFenceDx12( uint64_t value )
{
	CR_RETURN_HR( m_presentFence->SetEventOnCompletion( value, m_presentFenceEvent ) );
	for( uint32_t count = 0;; ++count )
	{
		if( WaitForSingleObject( m_presentFenceEvent, 1000 ) == WAIT_OBJECT_0 )
		{
			m_completedFrameIndex = value;
			return S_OK;
		}
		auto hr = m_device->GetDeviceRemovedReason();
		if( FAILED( hr ) )
		{
			CCP_LOGERR( "Detected GPU device removed while waiting for GPU. Removed reason is: 0x%08X", hr );
			return hr;
		}
		if( count == 3 )
		{
			CCP_LOGWARN( "Waiting for GPU takes more than %d seconds", count );
		}
	}
}

#if ENABLE_RELEASE_LATER_TAG
void Tr2PrimaryRenderContextAL::ReleaseLater( IUnknown* resource, const std::string& name )
{
	ReleasePair pair = { resource, name };
	m_pendingRelease[m_currentBackBufferIndex].push_back( pair );
}
#else
void Tr2PrimaryRenderContextAL::ReleaseLater( IUnknown* resource )
{
	m_pendingRelease[m_currentBackBufferIndex].push_back( resource );
}
#endif

void Tr2PrimaryRenderContextAL::FlushPendingRelease()
{
	for( auto& releases : m_pendingRelease )
	{
		releases.clear();
	}
}

uint64_t Tr2PrimaryRenderContextAL::GetRecordingFrameNumber() const
{
	return m_fenceValue + 1;
}

uint64_t Tr2PrimaryRenderContextAL::GetRenderedFrameNumber() const
{
	return m_completedFrameIndex;
}

void Tr2PrimaryRenderContextAL::OnShaderProgramDestroyedDx12( TrinityALImpl::Tr2ShaderProgramAL* sp )
{
	for( auto it = begin( m_pipelineStates ); it != end( m_pipelineStates ); )
	{
		if( it->first.m_shaderProgram.m_program.get() == sp )
		{
			RELEASE_LATER( this, it->second );
			it = m_pipelineStates.erase( it );
		}
		else
		{
			++it;
		}
	}
}

void Tr2PrimaryRenderContextAL::OnVertexLayoutDestroyedDx12( TrinityALImpl::Tr2VertexLayoutAL* vl )
{
	for( auto it = begin( m_pipelineStates ); it != end( m_pipelineStates ); )
	{
		if( it->first.m_vertexLayout.m_layout.get() == vl )
		{
			RELEASE_LATER( this, it->second );
			it = m_pipelineStates.erase( it );
		}
		else
		{
			++it;
		}
	}
}

ALResult Tr2PrimaryRenderContextAL::FlushAndSyncDx12( Tr2RenderContextAL& renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	if( m_statsQuery && m_statsStatus == STAT_BEGIN_ISSUED )
	{
		m_commandList->EndQuery( m_statsQuery, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, 0 );
		m_statsStatus = STAT_READY;
	}
	FlushBarriersDx12();

	CR_RETURN_HR( renderContext.m_commandList->Close() );
	ID3D12CommandList* const commandLists[] = { renderContext.m_commandList };
	m_commandQueue->ExecuteCommandLists( _countof( commandLists ), commandLists );

	FORWARD_HR( WaitForFenceDx12( SignalDx12() ) );

	m_pendingPresents.erase(
		std::remove_if( begin( m_pendingPresents ), end( m_pendingPresents ), []( const PendingPresent& p ) -> bool { return !p.backBuffer.IsValid(); } ),
		end( m_pendingPresents ) );
	m_srvUavAllocator->SetFrameIndices( GetRecordingFrameNumber(), GetRenderedFrameNumber() );

	// Reduce the pending releases
	for( auto index = 0; index < m_pendingRelease.size(); ++index )
	{
		PopPendingRelease( index );
	}

	auto commandAllocator = m_commandAllocators[m_commandAllocatorIndex++ % m_commandAllocators.size()];
	commandAllocator->Reset();
	renderContext.m_commandList->Reset( commandAllocator, nullptr );

	renderContext.ReApplyStateDx12();

	return S_OK;
}

D3D12_CPU_DESCRIPTOR_HANDLE Tr2PrimaryRenderContextAL::GetNullRtHandle( const Tr2TextureAL& compatibleWith )
{
	NullRtDesc desc = { compatibleWith.GetFormat(), compatibleWith.GetWidth(), compatibleWith.GetHeight(), compatibleWith.GetMsaaDesc() };
	auto found = m_nullRts.find( desc );
	if( found != end( m_nullRts ) )
	{
		return found->second;
	}

	D3D12_RENDER_TARGET_VIEW_DESC rtv;
	rtv.Format = DXGI_FORMAT( desc.format );
	if( desc.msaa.samples > 1 )
	{
		rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
	}
	else
	{
		rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtv.Texture2D.MipSlice = 0;
		rtv.Texture2D.PlaneSlice = 0;
	}

	auto handle = m_nullRtHeap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += m_nullRts.size() * m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
	m_device->CreateRenderTargetView( nullptr, &rtv, handle );

	m_nullRts[desc] = handle;
	return handle;
}

const TrinityALImpl::GpuMarkerBuffer& Tr2PrimaryRenderContextAL::GetMarkerBuffer() const
{
	return m_immediateBuffer;
}

TrinityALImpl::GpuCrashTracker* Tr2PrimaryRenderContextAL::GetGpuCrashTracker() const
{
	return m_gpuCrashTracker;
}

void Tr2PrimaryRenderContextAL::UnRegisterFromCrashTracker( ID3D12GraphicsCommandList2* commandList )
{
	if( m_gpuCrashTracker )
	{
		m_gpuCrashTracker->UnRegisterCommandList( commandList );
	}
}

void Tr2PrimaryRenderContextAL::AddShaderBinaryToCrashTracker( const Tr2ShaderBytecodeAL& bytecode, const char* shaderPath )
{
	if( m_gpuCrashTracker )
	{
		m_gpuCrashTracker->RegisterShaderBinary( bytecode, shaderPath );
	}
}


ALResult Tr2PrimaryRenderContextAL::GetGpuStateMarker( Tr2RenderContextEnum::RenderContextStatus& status, std::string& marker, std::string& offendingShader ) const
{
	if( SUCCEEDED( m_immediateBuffer.GetMarker( marker ) ) )
	{
		offendingShader = "";

		if( m_gpuCrashTracker )
		{
			m_gpuCrashTracker->GetOffendingShader( offendingShader );
		}

		status = Tr2RenderContextEnum::CONTEXT_STATUS_FINISHED;
		return S_OK;
	}

	return E_FAIL;
}

ALResult Tr2PrimaryRenderContextAL::GetGpuPageFaultResource(
	Tr2RenderContextEnum::PixelFormat&,
	uint64_t&,
	uint32_t&,
	uint32_t&,
	uint32_t&,
	uint32_t& ) const
{
	return E_FAIL;
}

void Tr2PrimaryRenderContextAL::ScheduleSwapchainPresentDx12( IDXGISwapChain3* swapChain, const Tr2TextureAL& backbuffer, uint32_t presentInterval )
{
	for( auto it = begin( m_pendingPresents ); it != end( m_pendingPresents ); ++it )
	{
		if( it->swapChain == swapChain )
		{
			return;
		}
	}
	PendingPresent pp = { swapChain, backbuffer, presentInterval };
	m_pendingPresents.push_back( pp );
}

ID3D12DescriptorHeap* Tr2PrimaryRenderContextAL::GetGlobalSrvUavHeap() const
{
	return m_srvUavAllocator->GetGpuVisibleHeap();
}

std::shared_ptr<ShaderResourceViewDx12> Tr2PrimaryRenderContextAL::GetSrvHeapView() const
{
	return m_srvHeapStart;
}

std::shared_ptr<UnorderedAccessViewDx12> Tr2PrimaryRenderContextAL::GetUavHeapView() const
{
	return m_uavHeapStart;
}

ID3D12DescriptorHeap* Tr2PrimaryRenderContextAL::GetGlobalSamplerHeap() const
{
	return m_samplerAllocator->GetGpuVisibleHeap();
}

std::shared_ptr<SamplerStateDx12> Tr2PrimaryRenderContextAL::GetSamplerHeapView() const
{
	return m_samplerHeapStart;
}

/** Create a ShaderResourceView */
HRESULT Tr2PrimaryRenderContextAL::CreateShaderResourceView( ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, std::shared_ptr<class ShaderResourceViewDx12>& srvView )
{
	auto entry = m_srvUavAllocator->Allocate();
	if( entry == nullptr )
	{
		return E_OUTOFMEMORY;
	}
	// CHECK
	m_device->CreateShaderResourceView( desc.ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE ? nullptr : resource, &desc, entry->m_offsetCPU );
	m_device->CreateShaderResourceView( desc.ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE ? nullptr : resource, &desc, m_srvUavAllocator->GetDescriptorInCpuHeap( entry ) );
	srvView = std::make_shared<ShaderResourceViewDx12>( m_srvUavAllocator.get(), entry );
	return S_OK;
}

/** Create an UnorderedAccessView */
HRESULT Tr2PrimaryRenderContextAL::CreateUnorderedAccessView( ID3D12Resource* resource, ID3D12Resource* counterResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc, std::shared_ptr<class UnorderedAccessViewDx12>& uavView )
{
	auto entry = m_srvUavAllocator->Allocate();
	if( entry == nullptr )
	{
		return E_OUTOFMEMORY;
	}
	m_device->CreateUnorderedAccessView( resource, counterResource, &desc, entry->m_offsetCPU );
	m_device->CreateUnorderedAccessView( resource, counterResource, &desc, m_srvUavAllocator->GetDescriptorInCpuHeap( entry ) );
	uavView = std::make_shared<UnorderedAccessViewDx12>( m_srvUavAllocator.get(), entry );
	return S_OK;
}

/** Create a SamplerState */
HRESULT Tr2PrimaryRenderContextAL::CreateSamplerState( const D3D12_SAMPLER_DESC& desc, std::shared_ptr<SamplerStateDx12>& samplerState )
{
	GlobalDescriptorHeapPage::DescriptorEntry* entry = m_samplerAllocator->Allocate();
	if( entry == nullptr )
	{
		return E_OUTOFMEMORY;
	}
	m_device->CreateSampler( &desc, entry->m_offsetCPU );
	samplerState = std::make_shared<SamplerStateDx12>( m_samplerAllocator.get(), entry );
	return S_OK;
}

/** Create a RenderTargetView */
HRESULT Tr2PrimaryRenderContextAL::CreateRenderTargetView( ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC* desc, std::shared_ptr<RenderTargetViewDx12>& rtvView )
{
	const std::shared_ptr<GlobalDescriptorHeapAllocator>& allocator = m_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_RTV];
	GlobalDescriptorHeapPage::DescriptorEntry* entry = allocator->Allocate();
	if( entry == nullptr )
		return E_OUTOFMEMORY;

	m_device->CreateRenderTargetView( resource, desc, entry->m_offsetCPU );

	rtvView = std::make_shared<RenderTargetViewDx12>( allocator.get(), entry );
	return S_OK;
}

/** Create a DepthStencilView */
HRESULT Tr2PrimaryRenderContextAL::CreateDepthStencilView( ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc, std::shared_ptr<DepthStencilViewDx12>& dsvView )
{
	const std::shared_ptr<GlobalDescriptorHeapAllocator>& allocator = m_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_DSV];
	GlobalDescriptorHeapPage::DescriptorEntry* entry = allocator->Allocate();
	if( entry == nullptr )
		return E_OUTOFMEMORY;

	m_device->CreateDepthStencilView( resource, &desc, entry->m_offsetCPU );

	dsvView = std::make_shared<DepthStencilViewDx12>( allocator.get(), entry );
	return S_OK;
}

void Tr2PrimaryRenderContextAL::ResetRenderTargets()
{
	Tr2TextureAL nullTex;
	SetDepthStencil( nullTex );
	ResourceBarrierDx12( TrinityALImpl::Transition( m_defaultBackBuffer.m_texture->GetResourceDx12(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET ) );
	SetRenderTarget( m_defaultBackBuffer );
	for( uint32_t i = 1; i < RENDER_TARGET_COUNT; ++i )
	{
		SetRenderTarget( nullTex, i );
	}
}

bool Tr2PrimaryRenderContextAL::FormatIsUAVCompatibleDx12( DXGI_FORMAT format ) const
{
	switch( format )
	{
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
		return true;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SINT:
		return m_options.TypedUAVLoadAdditionalFormats != 0;
	case DXGI_FORMAT_R11G11B10_FLOAT:
		if( m_device && m_options.TypedUAVLoadAdditionalFormats != 0 )
		{
			D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = { format, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE };
			if( SUCCEEDED( m_device->CheckFeatureSupport( D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof( formatSupport ) ) ) )
			{
				const DWORD mask = D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD | D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE;
				return ( ( formatSupport.Support2 & mask ) == mask );
			}
		}
		return false;
	default:
		return false;
	}
}

std::shared_ptr<ShaderResourceViewDx12> Tr2PrimaryRenderContextAL::GetNullSrvDx12( Tr2ShaderRegisterAL::RegisterType type ) const
{
	return m_nullSrv[RegisterTypeIndex( type )];
}

std::shared_ptr<UnorderedAccessViewDx12> Tr2PrimaryRenderContextAL::GetNullUavDx12( Tr2ShaderRegisterAL::RegisterType type ) const
{
	return m_nullUav[RegisterTypeIndex( type )];
}

std::shared_ptr<SamplerStateDx12> Tr2PrimaryRenderContextAL::GetNullSamplerDx12() const
{
	return m_samplerHeapStart;
}

Tr2UpscalingAL::Result Tr2PrimaryRenderContextAL::EnableUpscaling( Tr2UpscalingAL::Technique tech, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter )
{
	if( m_upscalingTechnique )
	{
		delete m_upscalingTechnique;
		m_upscalingTechnique = nullptr;
	}

	if( tech == Tr2UpscalingAL::Technique::NONE )
	{
		return Tr2UpscalingAL::Result::OK;
	}

	// find the technique
	auto supportedTechnique = std::find( std::begin( TrinityALImpl::AVAILABLE_UPSCALING_TECHNIQUES ), std::end( TrinityALImpl::AVAILABLE_UPSCALING_TECHNIQUES ), tech );
	if( supportedTechnique == std::end( TrinityALImpl::AVAILABLE_UPSCALING_TECHNIQUES ) )
	{
		return Tr2UpscalingAL::Result::TECHNIQUE_NOT_SUPPORTED;
	}

	m_upscalingTechnique = TrinityALImpl::CreateUpscalingTechnique( *this, tech, setting, frameGeneration, adapter );
	if( m_upscalingTechnique == nullptr )
	{
		return Tr2UpscalingAL::Result::TECHNIQUE_NOT_SUPPORTED;
	}

	return Tr2UpscalingAL::Result::OK;
}

Tr2UpscalingContextAL* Tr2PrimaryRenderContextAL::GetUpscalingContext( uint32_t upscalingContextId ) const
{
	if( m_upscalingTechnique == nullptr )
	{
		return nullptr;
	}

	return m_upscalingTechnique->GetContext( upscalingContextId );
}

Tr2UpscalingContextAL* Tr2PrimaryRenderContextAL::CreateUpscalingContext( Tr2UpscalingAL::UpscalingContextParams params, uint32_t existingContext )
{
	if( m_upscalingTechnique == nullptr || params.displayWidth == 0 || params.displayHeight == 0 )
	{
		return nullptr;
	}

	return m_upscalingTechnique->CreateContext( params, existingContext );
}


void Tr2PrimaryRenderContextAL::DeleteUpscalingContext( uint32_t contextID )
{
	if( m_upscalingTechnique == nullptr )
	{
		return;
	}

	m_upscalingTechnique->DeleteContext( contextID );
}

Tr2UpscalingAL::UpscalingInfo Tr2PrimaryRenderContextAL::GetUpscalingInfo( uint32_t upscalingContextID ) const
{
	auto context = GetUpscalingContext( upscalingContextID );
	Tr2UpscalingAL::UpscalingInfo info = Tr2UpscalingAL::UpscalingInfo();

	if( context != nullptr )
	{
		info.temporal = m_upscalingTechnique->IsTemporal();
		info.upscalingAmount = context->GetUpscalingAmount();
		info.mipLevelBias = context->GetMipLevelBias( info.temporal );
		info.hasSharpening = context->HasSharpening();
		context->GetJitter( info.jitterX, info.jitterY );
		context->GetRenderDimensions( info.renderWidth, info.renderHeight );
		context->GetDisplayDimensions( info.displayWidth, info.displayHeight );
		m_upscalingTechnique->GetState( info.technique, info.setting, info.frameGeneration );
	}
	return info;
}

std::vector<std::tuple<Tr2UpscalingAL::Technique, uint32_t, bool>> Tr2PrimaryRenderContextAL::GetSupportedUpscalingTechniques( uint32_t adapter )
{
	Tr2UpscalingAL::Technique activeTechnique = Tr2UpscalingAL::Technique::NONE;
	if( m_upscalingTechnique )
	{
		Tr2UpscalingAL::Setting setting;
		bool framegeneration;
		m_upscalingTechnique->GetState( activeTechnique, setting, framegeneration );
	}

	std::vector<std::tuple<Tr2UpscalingAL::Technique, uint32_t, bool>> supportedTechniques;
	for( auto& technique : TrinityALImpl::AVAILABLE_UPSCALING_TECHNIQUES )
	{
		if( technique == activeTechnique && m_upscalingTechnique )
		{
			uint32_t allSettings = 0;

			for( auto& setting : m_upscalingTechnique->GetAvailableSettings() )
			{
				allSettings |= setting;
			}

			supportedTechniques.push_back( { technique, allSettings, m_upscalingTechnique->SupportsFrameGeneration() } );
			continue;
		}

		auto tech = TrinityALImpl::CreateUpscalingTechnique( *this, technique, Tr2UpscalingAL::Setting::NATIVE, false, adapter );
		if( tech )
		{
			uint32_t allSettings = 0;

			for( auto& setting : tech->GetAvailableSettings() )
			{
				allSettings |= setting;
			}

			supportedTechniques.push_back( { technique, allSettings, tech->SupportsFrameGeneration() } );

			delete tech;
			tech = nullptr;
		}
	}
	return supportedTechniques;
}

void Tr2PrimaryRenderContextAL::GetUpscalingSetup( Tr2UpscalingAL::Technique& technique, Tr2UpscalingAL::Setting& setting, bool& framegeneration, bool& temporal ) const
{
	if( m_upscalingTechnique )
	{
		m_upscalingTechnique->GetState( technique, setting, framegeneration );
		temporal = m_upscalingTechnique->IsTemporal();
		return;
	}
	technique = Tr2UpscalingAL::Technique::NONE;
	setting = Tr2UpscalingAL::Setting::NATIVE;
	framegeneration = false;
	temporal = false;
}


void Tr2PrimaryRenderContextAL::MarkFrameEvent( Tr2RenderContextEnum::FrameEvent frameEvent )
{
	if( m_upscalingTechnique )
	{
		m_upscalingTechnique->MarkFrameEvent( frameEvent );
	}
}

HRESULT Tr2PrimaryRenderContextAL::CreateSwapChainForHwnd(
	CComPtr<IDXGIFactory4>& factory4,
	ID3D12CommandQueue* commandQueue,
	HWND hWnd,
	const DXGI_SWAP_CHAIN_DESC1* pDesc,
	const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
	IDXGIOutput* pRestrictToOutput,
	CComPtr<IDXGISwapChain4>& swapchain )
{
	CComPtr<IDXGISwapChain1> swapChain1;
	CR_RETURN_HR( factory4->CreateSwapChainForHwnd(
		commandQueue,
		hWnd,
		pDesc,
		pFullscreenDesc,
		pRestrictToOutput,
		&swapChain1 ) );

	CR_RETURN_HR( swapChain1.QueryInterface( &swapchain ) );

	if( m_upscalingTechnique && m_upscalingTechnique->ReplacesSwapchain() )
	{
		swapChain1 = nullptr;
		m_upscalingTechnique->ReplaceSwapchain( swapchain, hWnd, commandQueue );
	}
	return S_OK;
}

HRESULT Tr2PrimaryRenderContextAL::CreateDevice( IUnknown* adapter, D3D_FEATURE_LEVEL featureLevel, CComPtr<ID3D12Device>& device ) const
{
	CR_RETURN_HR( D3D12CreateDevice( adapter, featureLevel, IID_PPV_ARGS( &device ) ) );

	if( m_upscalingTechnique && m_upscalingTechnique->ReplacesDevice() )
	{
		device = m_upscalingTechnique->ReplaceDevice( device );
	}
	return S_OK;
}

HRESULT Tr2PrimaryRenderContextAL::CreateCommandQueue( CComPtr<ID3D12Device>& device, D3D12_COMMAND_QUEUE_DESC* desc, CComPtr<ID3D12CommandQueue>& commandQueue ) const
{
	CR_RETURN_HR( device->CreateCommandQueue( desc, IID_PPV_ARGS( &commandQueue ) ) );
	if( m_upscalingTechnique && m_upscalingTechnique->ReplacesCommandQueue() )
	{
		commandQueue = m_upscalingTechnique->ReplaceCommandQueue( commandQueue );
	}
	return S_OK;
}

HRESULT Tr2PrimaryRenderContextAL::CreateFactory2( UINT flags, CComPtr<IDXGIFactory4>& factory ) const
{
	CR_RETURN_HR( CreateDXGIFactory2( flags, IID_PPV_ARGS( &factory ) ) );
	if( m_upscalingTechnique && m_upscalingTechnique->ReplacesFactory() )
	{
		factory = m_upscalingTechnique->ReplaceFactory( factory );
	}
	return S_OK;
}

bool Tr2PrimaryRenderContextAL::SupportsBindlessTextures() const
{
	return true;
}

#endif