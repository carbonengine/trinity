////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2PrimaryRenderContextDx12.h"
#include "ALLog.h"
#include "Tr2VideoAdapterInfoALDx12.h"
#include "Tr2AdapterStructures.h"
#include "ITr2RenderContextEvents.h"
#include "Utilities.h"
#include "Tr2FragmentOpSettings.h"
#include "Tr2SwapChainALDx12.h"
#include "Tr2TextureALDx12.h"
#include "util/AmdExtDevice.h"

extern bool g_requestDeviceDebugLayer;
extern bool g_requestDebugMarkers;
bool g_gatherPipelineStatistics = false;

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


namespace
{
	bool EnableDebugLayer()
	{
		CComPtr<ID3D12Debug> debugInterface;
		if( SUCCEEDED( D3D12GetDebugInterface( IID_PPV_ARGS( &debugInterface ) ) ) )
		{
			debugInterface->EnableDebugLayer();
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
			D3D12_MESSAGE_SEVERITY severity[] =
			{
				D3D12_MESSAGE_SEVERITY_CORRUPTION,
				D3D12_MESSAGE_SEVERITY_ERROR,
			};

			// JB: Added some message filters
			D3D12_MESSAGE_ID blocked[] =
			{
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
			filter.DenyList.NumIDs = sizeof(blocked) / sizeof(blocked[0]);
			CR( queue->PushRetrievalFilter( &filter ) );
			CR( queue->PushStorageFilter(&filter) );
		}
	}

	DXGI_FORMAT SafeConvertD3DBackBufferFormat( Tr2RenderContextEnum::PixelFormat bbFormat )
	{
		DXGI_FORMAT out = static_cast<DXGI_FORMAT>( bbFormat );
		if( out == DXGI_FORMAT_B8G8R8X8_UNORM )
		{
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		}
		if( out == Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN )
		{
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		}
		return out;
	}

	uint32_t BACK_BUFFER_COUNT = 3;

	ALResult CreateSwapChain(
		CComPtr<IDXGISwapChain3>& swapChain,
		Tr2WindowHandle focusWindow,
		const Tr2PresentParametersAL& presentationParameters,
		ID3D12CommandQueue* commandQueue,
		IDXGIOutput* output )
	{
		CComPtr<IDXGIFactory4> dxgiFactory;

		UINT createFactoryFlags = 0;
		if( g_requestDeviceDebugLayer )
		{
			createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
		}

		CR_RETURN_HR( CreateDXGIFactory2( createFactoryFlags, IID_PPV_ARGS( &dxgiFactory ) ) );

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = presentationParameters.mode.width;
		swapChainDesc.Height = presentationParameters.mode.height;
		swapChainDesc.Format = SafeConvertD3DBackBufferFormat( presentationParameters.mode.format );
		swapChainDesc.Stereo = FALSE;
		swapChainDesc.SampleDesc.Count = std::max( presentationParameters.msaaType, 1u );
		swapChainDesc.SampleDesc.Quality = presentationParameters.msaaQuality;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = BACK_BUFFER_COUNT;// presentationParameters.windowed ? 2 : 3;
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;// DXGI_MODE_SCALING( presentationParameters.mode.scaling );
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;


		auto wnd = Tr2WindowHandle( presentationParameters.outputWindow );
		if( !presentationParameters.outputWindow )
		{
			wnd = focusWindow;
		}

		CComPtr<IDXGISwapChain1> swapChain1;
		if( presentationParameters.windowed )
		{
			CR_RETURN_HR( dxgiFactory->CreateSwapChainForHwnd(
				commandQueue,
				wnd,
				&swapChainDesc,
				nullptr,
				output,
				&swapChain1 ) );
		}
		else
		{
			DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen;
			fullscreen.RefreshRate.Numerator = 0;
			fullscreen.RefreshRate.Denominator = 0;
			fullscreen.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER( presentationParameters.mode.scanlineOrdering );
			fullscreen.Scaling = DXGI_MODE_SCALING( presentationParameters.mode.scaling );
			fullscreen.Windowed = FALSE;

			CR_RETURN_HR( dxgiFactory->CreateSwapChainForHwnd(
				commandQueue,
				wnd,
				&swapChainDesc,
				&fullscreen,
				output,
				&swapChain1 ) );
		}
		CR_RETURN_HR( swapChain1.QueryInterface( &swapChain ) );
		CR_RETURN_HR( dxgiFactory->MakeWindowAssociation( wnd, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES ) );
		return S_OK;
	}
}


Tr2PrimaryRenderContextAL::Tr2PrimaryRenderContextAL()
	:m_events( nullptr ),
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
	m_amdExtDeviceObject( 0 )
{
	m_ownerDevice = this;
	m_defaultBackBuffer.m_texture = std::make_shared<TrinityALImpl::Tr2TextureAL>();
}

Tr2PrimaryRenderContextAL::~Tr2PrimaryRenderContextAL()
{
	Destroy();
}


ALResult Tr2PrimaryRenderContextAL::CreateDevice(
	uint32_t adapter,
	Tr2WindowHandle  focusWindow,
	const Tr2PresentParametersAL& presentationParameters )
{
	Destroy();

	bool hasDebugLayer = false;
	if( g_requestDeviceDebugLayer )
	{
		hasDebugLayer = EnableDebugLayer();
	}

	CComPtr<ID3D12Device> device;
	CComPtr<IDXGIAdapter1> dxgiAdapter;
	CComPtr<IDXGIOutput> output;

	CR_RETURN_HR( TrinityALImpl::GetVideoAdapter( adapter, &dxgiAdapter, &output ) );

	CR_RETURN_HR( D3D12CreateDevice( dxgiAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS( &device ) ) );

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
	} heapLayout[] =
	{
		{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, CRVSRVUAV_PAGE_SIZE, CRVSRVUAV_NUM_PAGES },
		{ D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, SAMPLER_PAGE_SIZE, SAMPLER_NUM_PAGES },
		{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV, RTV_PAGE_SIZE, RTV_NUM_PAGES },
		{ D3D12_DESCRIPTOR_HEAP_TYPE_DSV, DSV_PAGE_SIZE, DSV_NUM_PAGES }
	};

	for (int32_t idx = 0; idx < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++idx)
	{
		CCP_ASSERT(heapLayout[idx].m_type == D3D12_DESCRIPTOR_HEAP_TYPE(idx));

		m_allocators[idx] = std::make_shared<GlobalDescriptorHeapAllocator>(device, heapLayout[idx].m_numPages, heapLayout[idx].m_pageSize, heapLayout[idx].m_type);
		CCP_ASSERT(m_allocators[idx].get() != nullptr);
	}

	CComPtr<ID3D12CommandQueue> commandQueue;

	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	CR_RETURN_HR( device->CreateCommandQueue( &desc, IID_PPV_ARGS( &commandQueue ) ) );

	const bool isWindowless = ( focusWindow == 0 ) && presentationParameters.software;

	CComPtr<IDXGISwapChain3> swapChain;
	uint32_t backBufferCount = 1;
	if( !isWindowless )
	{
		backBufferCount = BACK_BUFFER_COUNT;
		FORWARD_HR( CreateSwapChain( swapChain, focusWindow, presentationParameters, commandQueue, output ) );
	}

	
	std::vector<std::shared_ptr<RenderTargetViewDx12>> rtvs;
	std::vector<CComPtr<ID3D12Resource>> backBuffers;

	{
		// JB: Not pretty, but createRenderTargetView requires a valid device!
		m_device = device;
		HRESULT hr = TrinityALImpl::Tr2SwapChainAL::GetBackBuffers(this, backBuffers, rtvs, swapChain);
		m_device = nullptr;

		FORWARD_HR(hr);
	}

	std::vector < CComPtr<ID3D12CommandAllocator>> commandAllocators;
	for( uint32_t i = 0; i < backBufferCount; ++i )
	{
		CComPtr<ID3D12CommandAllocator> commandAllocator;
		CR_RETURN_HR( device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &commandAllocator ) ) );
		commandAllocators.push_back( commandAllocator );
	}

	uint32_t currentBackBufferIndex;
	if( swapChain )
	{
		currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();
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
	m_commandQueue = commandQueue;
	m_swapChain = swapChain;
	m_output = output;

	m_commandAllocators = commandAllocators;
	m_commandAllocatorIndex = 0;
	m_currentBackBufferIndex = currentBackBufferIndex;

	m_immediateBuffer.Create( m_device );

	m_presentFence = fence;
	m_presentFenceEvent = ::CreateEvent( NULL, FALSE, FALSE, NULL );

	m_completedFrameIndex = 0;
	m_fenceValue = 1;
	m_frameFenceValues.clear();
	m_frameFenceValues.resize( backBufferCount, 0 );
	m_pendingRelease.resize( backBufferCount );

	CreateDx12( commandAllocators[currentBackBufferIndex], *this );

	m_syncInterval = presentationParameters.presentInterval & 0xf;

	m_defaultBackBuffer.m_texture->AssignFromSwapChainDx12( backBuffers, rtvs, *this );

	m_nullRtHeap = nullRtHeap;

	m_drawInstancedIndirect = drawInstancedIndirect;
	m_drawIndexedInstancedIndirect = drawIndexedInstancedIndirect;
	m_dispatchIndirect = dispatchIndirect;

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if( SUCCEEDED( device->CheckFeatureSupport( D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof( featureData ) ) ) )
	{
		m_rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	}
	else
	{
		m_rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	if( g_requestDebugMarkers )
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
				}
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

	m_shadowCB.Create( uint32_t( sizeof( Tr2FragmentOpSettings ) ), *this );

	const uint32_t nullCbSize = 128 * 1024;
	std::vector<uint8_t> cb( nullCbSize );
	D3D12_SUBRESOURCE_DATA cbd = { cb.data(), nullCbSize, nullCbSize };
	m_nullCB.Create( TrinityALImpl::Tr2ResourceHelper::STATIC, nullCbSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, 1, &cbd, *this );

	m_immediateBuffer.PutMarker( m_commandList2, "" );
	m_statsQueryFrameIndex = 0;
	m_statsStatus = STAT_READY;

	{
		Tr2AdapterInfo info;
		Tr2VideoAdapterInfo::GetAdapterInfo( adapter, info );
		CCP_AL_LOGNOTICE( "Created dx12 device for adapter %S", info.description.c_str() );
	}

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

	if( m_amdExtDeviceObject )
	{
		static_cast<IAmdExtD3DDevice1*>( m_ownerDevice->m_amdExtDeviceObject )->Release();
		m_amdExtDeviceObject = nullptr;
	}

	m_pendingPresents.clear();

	m_statsQuery = nullptr;
	m_statsResult = nullptr;

	Tr2RenderContextAL::Destroy();

	m_shadowCB = Tr2ConstantBufferAL();

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

	m_commandQueue = nullptr;

	if( m_swapChain )
	{
		CR( m_swapChain->SetFullscreenState( FALSE, nullptr ) );
	}
	m_swapChain = nullptr;

	// JB: Forcing the destruction of samplers because they now hold a SamplerStateDx12 object
	m_samplerStateFactory.Clear();

	for (int32_t idx = 0; idx < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++idx)
	{
		m_allocators[idx] = nullptr;
	}

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

	TrinityALImpl::Destroy();
}

bool Tr2PrimaryRenderContextAL::IsValid() const
{
	return m_device != nullptr;
}

ALResult Tr2PrimaryRenderContextAL::SetPresentParameters( uint32_t adapter, const Tr2PresentParametersAL& presentationParameters )
{
	if( !m_swapChain )
	{
		return E_FAIL;
	}
	if( m_statsQuery && m_statsStatus == STAT_BEGIN_ISSUED )
	{
		m_commandList->EndQuery( m_statsQuery, D3D12_QUERY_TYPE_PIPELINE_STATISTICS, 0 );
		m_statsStatus = STAT_READY;
	}
	FlushBarriersDx12();
	CR( m_commandList->Close() );

	CComPtr<IDXGIOutput> dxgiOutput;
	CComPtr<IDXGIAdapter1> adapterPtr;
	FORWARD_HR( TrinityALImpl::GetVideoAdapter( adapter, &adapterPtr, &dxgiOutput ) );

	//if( !presentationParameters.windowed && dxgiOutput != m_output )
	//{
	//	// If we are switching between two monitors in fullscreen mode it seems we first should
	//	// go windowed and then fullscreen on another monitor, otherwise DXGI behaves funny.
	//	CR( m_swapChain->SetFullscreenState( FALSE, nullptr ) );
	//}

	m_output = dxgiOutput;

	DXGI_FORMAT fmt = SafeConvertD3DBackBufferFormat( presentationParameters.mode.format );

	DXGI_MODE_DESC modeDesc;
	modeDesc.Width = presentationParameters.mode.width;
	modeDesc.Height = presentationParameters.mode.height;
	modeDesc.RefreshRate.Numerator = 0;
	modeDesc.RefreshRate.Denominator = 0;
	modeDesc.Format = fmt;
	modeDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER( presentationParameters.mode.scanlineOrdering );
	modeDesc.Scaling = DXGI_MODE_SCALING( presentationParameters.mode.scaling );

	BOOL wasFullScreen = false;
	CR( m_swapChain->GetFullscreenState( &wasFullScreen, nullptr ) );

	m_defaultBackBuffer.m_texture->Destroy();

	WaitForFenceDx12( SignalDx12() );
	for( auto it = begin( m_pendingRelease ); it != end( m_pendingRelease ); ++it )
	{
		it->clear();
	}

	auto value = m_frameFenceValues[m_currentBackBufferIndex];
	m_frameFenceValues.clear();
	m_frameFenceValues.resize( BACK_BUFFER_COUNT, value );

	if( !wasFullScreen && presentationParameters.windowed )
	{
		// windowed -> windowed
		CR( m_swapChain->ResizeTarget( &modeDesc ) );
		CR( m_swapChain->ResizeBuffers( BACK_BUFFER_COUNT,
			presentationParameters.mode.width,
			presentationParameters.mode.height,
			fmt,
			DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH ) );
	}
	else if( wasFullScreen && presentationParameters.windowed )
	{
		// fullscreen -> windowed
		CR( m_swapChain->SetFullscreenState( FALSE, nullptr ) );

		CR( m_swapChain->ResizeTarget( &modeDesc ) );
		CR( m_swapChain->ResizeBuffers( BACK_BUFFER_COUNT,
			presentationParameters.mode.width,
			presentationParameters.mode.height,
			fmt,
			DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH ) );
	}
	else if( !wasFullScreen && !presentationParameters.windowed )
	{
		// windowed -> fullscreen
		CR( m_swapChain->ResizeTarget( &modeDesc ) );
		CR( m_swapChain->SetFullscreenState( TRUE, dxgiOutput ) );
		CR( m_swapChain->ResizeTarget( &modeDesc ) );
		CR( m_swapChain->ResizeBuffers( BACK_BUFFER_COUNT,
			presentationParameters.mode.width,
			presentationParameters.mode.height,
			fmt,
			DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH ) );
	}
	else
	{
		// fulscreen -> fullscreen (resolution change)
		CR( m_swapChain->ResizeTarget( &modeDesc ) );
		CR( m_swapChain->ResizeBuffers( BACK_BUFFER_COUNT,
			presentationParameters.mode.width,
			presentationParameters.mode.height,
			fmt,
			DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH ) );
	}

	m_syncInterval = presentationParameters.presentInterval & 0xf;

	std::vector<std::shared_ptr<RenderTargetViewDx12>> rtvs;
	std::vector<CComPtr<ID3D12Resource>> backBuffers;
	FORWARD_HR( TrinityALImpl::Tr2SwapChainAL::GetBackBuffers( this, backBuffers, rtvs, m_swapChain ) );

	m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	m_defaultBackBuffer.m_texture->AssignFromSwapChainDx12( backBuffers, rtvs, *this );
	m_defaultBackBuffer.m_texture->SetSwapChainBufferIndexDx12( m_currentBackBufferIndex );

	auto commandAllocator = m_commandAllocators[m_commandAllocatorIndex++ % m_commandAllocators.size()];
	commandAllocator->Reset();
	m_commandList->Reset( commandAllocator, nullptr );

	ResetDescriptorCaches();
	ResetRenderTargets();

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
		std::remove_if( begin( m_pendingPresents ), end( m_pendingPresents ), []( const PendingPresent& p )->bool { return !p.backBuffer.IsValid(); } ),
		end( m_pendingPresents ) );

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
			m_statsQueryFrameIndex = GetCurrentFrameIndexDx12();
		}

		if( m_statsStatus == STAT_END_ISSUED && GetCompletedFrameIndexDx12() >= m_statsQueryFrameIndex )
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

	CR( m_commandList->Close() );


	ID3D12CommandList* const commandLists[] = { m_commandList };
	m_commandQueue->ExecuteCommandLists( _countof( commandLists ), commandLists );
	m_frameFenceValues[m_currentBackBufferIndex] = SignalDx12();

	CR( m_swapChain->Present( m_syncInterval, 0 ) );
	for( auto it = begin( m_pendingPresents ); it != end( m_pendingPresents ); ++it )
	{
		it->swapChain->Present( it->presentInterval, 0 );
	}

	m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	WaitForFenceDx12( m_frameFenceValues[m_currentBackBufferIndex] );

	{
#if ENABLE_RELEASE_LATER_TAG && DUMP_RELEASE_LATER_TAGS
		std::vector<ReleasePair>& toRelease = m_pendingRelease[m_currentBackBufferIndex];
		for (size_t idx = 0; idx < toRelease.size(); ++idx)
		{
			char dbo[512];
			sprintf_s(dbo, "Releasing: %s\n", toRelease[idx].m_name.c_str());
			
			OutputDebugStringA(dbo);
			//CCP_LOG(dbo);

			toRelease[idx].m_object = nullptr;
		}
#endif
		m_pendingRelease[m_currentBackBufferIndex].clear();
	}

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
				IID_PPV_ARGS( &m_statsResult )
			) );
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

	return S_OK;
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
	m_completedFrameIndex = m_presentFence->GetCompletedValue();
	if( m_completedFrameIndex < value )
	{
		CR_RETURN_HR( m_presentFence->SetEventOnCompletion( value, m_presentFenceEvent ) );
		if( WaitForSingleObject( m_presentFenceEvent, 1000 ) == WAIT_TIMEOUT )
		{
			return E_FAIL;
		}
		m_completedFrameIndex = value;
	}
	return S_OK;
}

#if ENABLE_RELEASE_LATER_TAG
void Tr2PrimaryRenderContextAL::ReleaseLater( IUnknown* resource, const std::string& name)
{
	ReleasePair pair = { resource, name };
	m_pendingRelease[m_currentBackBufferIndex].push_back(pair);
}
#else
void Tr2PrimaryRenderContextAL::ReleaseLater(IUnknown* resource)
{
	m_pendingRelease[m_currentBackBufferIndex].push_back(resource);
}
#endif

uint64_t Tr2PrimaryRenderContextAL::GetCurrentFrameIndexDx12() const
{
	return m_fenceValue + 1;
}

uint64_t Tr2PrimaryRenderContextAL::GetCompletedFrameIndexDx12() const
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

	WaitForFenceDx12( SignalDx12() );
	m_pendingPresents.erase(
		std::remove_if( begin( m_pendingPresents ), end( m_pendingPresents ), []( const PendingPresent& p )->bool { return !p.backBuffer.IsValid(); } ),
		end( m_pendingPresents ) );
	for( auto it = begin( m_pendingRelease ); it != end( m_pendingRelease ); ++it )
	{
		it->clear();
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

ALResult Tr2PrimaryRenderContextAL::GetGpuStateMarker( Tr2RenderContextEnum::RenderContextStatus& status, std::string& marker ) const
{
	if( SUCCEEDED( m_immediateBuffer.GetMarker( marker ) ) )
	{
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

/** Create a ShaderResourceView */
HRESULT Tr2PrimaryRenderContextAL::CreateShaderResourceView(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc, std::shared_ptr<class ShaderResourceViewDx12>& srvView)
{
	const std::shared_ptr<GlobalDescriptorHeapAllocator>& allocator = m_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
	GlobalDescriptorHeapPage::DescriptorEntry* entry = allocator->Allocate();
	if (entry == nullptr)
		return E_OUTOFMEMORY;

	m_device->CreateShaderResourceView(resource, &desc, entry->m_offsetCPU);

	srvView = std::make_shared<ShaderResourceViewDx12>(allocator.get(), entry);
	return S_OK;
}

/** Create an UnorderedAccessView */
HRESULT Tr2PrimaryRenderContextAL::CreateUnorderedAccessView(ID3D12Resource* resource, ID3D12Resource* counterResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc, std::shared_ptr<class UnorderedAccessViewDx12>& uavView)
{
	const std::shared_ptr<GlobalDescriptorHeapAllocator>& allocator = m_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
	GlobalDescriptorHeapPage::DescriptorEntry* entry = allocator->Allocate();
	if (entry == nullptr)
		return E_OUTOFMEMORY;

	m_device->CreateUnorderedAccessView(resource, counterResource, &desc, entry->m_offsetCPU);

	uavView = std::make_shared<UnorderedAccessViewDx12>(allocator.get(), entry);
	return S_OK;
}

/** Create a SamplerState */
HRESULT Tr2PrimaryRenderContextAL::CreateSamplerState(const D3D12_SAMPLER_DESC& desc, std::shared_ptr<SamplerStateDx12>& samplerState)
{
	const std::shared_ptr<GlobalDescriptorHeapAllocator>& allocator = m_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER];
	GlobalDescriptorHeapPage::DescriptorEntry* entry = allocator->Allocate();
	if (entry == nullptr)
		return E_OUTOFMEMORY;

	m_device->CreateSampler(&desc, entry->m_offsetCPU);

	samplerState = std::make_shared<SamplerStateDx12>(allocator.get(), entry);
	return S_OK;
}

/** Create a RenderTargetView */
HRESULT Tr2PrimaryRenderContextAL::CreateRenderTargetView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC* desc, std::shared_ptr<RenderTargetViewDx12>& rtvView)
{
	const std::shared_ptr<GlobalDescriptorHeapAllocator>& allocator = m_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_RTV];
	GlobalDescriptorHeapPage::DescriptorEntry* entry = allocator->Allocate();
	if (entry == nullptr)
		return E_OUTOFMEMORY;

	m_device->CreateRenderTargetView(resource, desc, entry->m_offsetCPU);

	rtvView = std::make_shared<RenderTargetViewDx12>(allocator.get(), entry);
	return S_OK;
}

/** Create a DepthStencilView */
HRESULT Tr2PrimaryRenderContextAL::CreateDepthStencilView(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc, std::shared_ptr<DepthStencilViewDx12>& dsvView)
{
	const std::shared_ptr<GlobalDescriptorHeapAllocator>& allocator = m_allocators[D3D12_DESCRIPTOR_HEAP_TYPE_DSV];
	GlobalDescriptorHeapPage::DescriptorEntry* entry = allocator->Allocate();
	if (entry == nullptr)
		return E_OUTOFMEMORY;

	m_device->CreateDepthStencilView(resource, &desc, entry->m_offsetCPU);

	dsvView = std::make_shared<DepthStencilViewDx12>(allocator.get(), entry);
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

#endif