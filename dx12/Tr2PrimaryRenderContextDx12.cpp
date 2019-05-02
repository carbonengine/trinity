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


extern bool g_requestDeviceDebugLayer;


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

			D3D12_INFO_QUEUE_FILTER filter;
			memset( &filter, 0, sizeof( filter ) );
			filter.AllowList.NumSeverities = 2;
			filter.AllowList.pSeverityList = severity;
			CR( queue->PushRetrievalFilter( &filter ) );
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
		swapChainDesc.Flags = 0;


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
			fullscreen.RefreshRate.Numerator = presentationParameters.mode.refreshRateNumerator;
			fullscreen.RefreshRate.Denominator = presentationParameters.mode.refreshRateDenominator;
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
		CR_RETURN_HR( dxgiFactory->MakeWindowAssociation( wnd, DXGI_MWA_NO_ALT_ENTER ) );
		return S_OK;
	}

	ALResult GetBackBuffers(
		std::vector<CComPtr<ID3D12Resource>>& backBuffers,
		CComPtr<ID3D12DescriptorHeap>& descriptorHeap,
		ID3D12Device* device,
		IDXGISwapChain1* swapChain )
	{
		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
		descriptorHeapDesc.NumDescriptors = BACK_BUFFER_COUNT * 2;
		descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

		CR_RETURN_HR( device->CreateDescriptorHeap( &descriptorHeapDesc, IID_PPV_ARGS( &descriptorHeap ) ) );

		auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle( descriptorHeap->GetCPUDescriptorHandleForHeapStart() );

		DXGI_SWAP_CHAIN_DESC scDesc;
		swapChain->GetDesc( &scDesc );

		for( uint32_t i = 0; i < BACK_BUFFER_COUNT; ++i )
		{
			CComPtr<ID3D12Resource> backBuffer;
			CR_RETURN_HR( swapChain->GetBuffer( i, IID_PPV_ARGS( &backBuffer ) ) );
			backBuffers.push_back( backBuffer );

			device->CreateRenderTargetView( backBuffer, nullptr, rtvHandle );
			rtvHandle.ptr += rtvDescriptorSize;

			D3D12_RENDER_TARGET_VIEW_DESC rtv = { DXGI_FORMAT( Tr2RenderContextEnum::MakeSrgb( Tr2RenderContextEnum::PixelFormat( scDesc.BufferDesc.Format ) ) ), D3D12_RTV_DIMENSION_TEXTURE2D };
			rtv.Texture2D.MipSlice = rtv.Texture2D.PlaneSlice = 0;

			device->CreateRenderTargetView( backBuffer, &rtv, rtvHandle );
			rtvHandle.ptr += rtvDescriptorSize;
		}
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
	m_commandAllocatorIndex( 0 )
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

	CComPtr<ID3D12DescriptorHeap> descriptorHeap;
	std::vector<CComPtr<ID3D12Resource>> backBuffers;
	FORWARD_HR( GetBackBuffers( backBuffers, descriptorHeap, device, swapChain ) );

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
	CComPtr<ID3D12GraphicsCommandList> commandList;
	CR_RETURN_HR( device->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[currentBackBufferIndex], nullptr, IID_PPV_ARGS( &commandList ) ) );
	CR_RETURN_HR( commandList->Close() );

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
	m_commandList = commandList;

	m_presentFence = fence;
	m_presentFenceEvent = ::CreateEvent( NULL, FALSE, FALSE, NULL );

	m_fenceValue = 1;
	m_frameFenceValues.clear();
	m_frameFenceValues.resize( backBufferCount, 0 );
	m_pendingRelease.resize( backBufferCount );

	m_syncInterval = presentationParameters.presentInterval & 0xf;

	m_defaultBackBuffer.m_texture->AssignFromSwapChainDx12( backBuffers, descriptorHeap, device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV ), *this );

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

	if( m_events )
	{
		m_events->OnContextCreated( *this );
	}

	m_boundRenderTargets[0] = m_defaultBackBuffer;
	m_defaultBackBuffer.m_texture->SetSwapChainBufferIndexDx12( m_currentBackBufferIndex );

	auto commandAllocator = m_commandAllocators[m_commandAllocatorIndex++ % m_commandAllocators.size()];
	commandAllocator->Reset();
	m_commandList->Reset( commandAllocator, nullptr );

	{
		auto barrier = TrinityALImpl::Transition( m_defaultBackBuffer.m_texture->GetResourceDx12(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET );
		m_commandList->ResourceBarrier( 1, &barrier );
		auto handle = m_defaultBackBuffer.m_texture->GetRtvDescriptorHandleDx12( m_srgbWriteEnable ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR );
		m_commandList->OMSetRenderTargets( 1, &handle, FALSE, nullptr );
	}

	SetViewport( Tr2Viewport( m_defaultBackBuffer.GetWidth(), m_defaultBackBuffer.GetHeight() ) );
	D3D12_RECT rect = {0, 0, m_defaultBackBuffer.GetWidth(), m_defaultBackBuffer.GetHeight() };
	m_commandList->RSSetScissorRects( 1, &rect );
	m_dirtyPso = true;
	m_ownerDevice = this;

	m_renderTargetCount = 1;
	std::fill( std::begin( m_renderTargetFormats ), std::end( m_renderTargetFormats ), Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN );
	m_renderTargetFormats[0] = m_defaultBackBuffer.GetFormat();
	m_depthStencilFormat = Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN;
	m_sampleDesc = m_defaultBackBuffer.GetMsaaDesc();

	m_genMipsResources = new TrinityALImpl::GenerateMipsResources( m_device );

	m_shadowCB.Create( uint32_t( sizeof( Tr2FragmentOpSettings ) ), *this );

	return S_OK;
}

void Tr2PrimaryRenderContextAL::Destroy()
{
	if( IsValid() )
	{
		WaitForFenceDx12( SignalDx12() );
	}

	Tr2RenderContextAL::Destroy();

	m_shadowCB.Destroy();

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

	m_commandList = nullptr;
	m_commandAllocators.clear();

	m_commandQueue = nullptr;

	if( m_swapChain )
	{
		CR( m_swapChain->SetFullscreenState( FALSE, nullptr ) );
	}
	m_swapChain = nullptr;

	m_device = nullptr;

	if( m_presentFenceEvent )
	{
		CloseHandle( m_presentFenceEvent );
		m_presentFenceEvent = nullptr;
	}

	m_fenceValue = 0;
	m_frameFenceValues.clear();

	m_rootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	m_pendingRelease.clear();
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
	modeDesc.RefreshRate.Numerator = presentationParameters.mode.refreshRateNumerator;
	modeDesc.RefreshRate.Denominator = presentationParameters.mode.refreshRateDenominator;
	modeDesc.Format = fmt;
	modeDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER( presentationParameters.mode.scanlineOrdering );
	modeDesc.Scaling = DXGI_MODE_SCALING( presentationParameters.mode.scaling );

	CR( m_swapChain->ResizeTarget( &modeDesc ) );

	m_defaultBackBuffer.m_texture->Destroy();

	WaitForFenceDx12( SignalDx12() );
	for( auto it = begin( m_pendingRelease ); it != end( m_pendingRelease ); ++it )
	{
		it->clear();
	}

	auto value = m_frameFenceValues[m_currentBackBufferIndex];
	m_frameFenceValues.clear();
	m_frameFenceValues.resize( BACK_BUFFER_COUNT, value );

	if( !presentationParameters.windowed )
	{
		CR( m_swapChain->SetFullscreenState( TRUE, dxgiOutput ) );
	}
	else
	{
		CR( m_swapChain->SetFullscreenState( FALSE, nullptr ) );
	}

	CR( m_swapChain->ResizeBuffers( BACK_BUFFER_COUNT,
		presentationParameters.mode.width,
		presentationParameters.mode.height,
		fmt,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH ) );

	m_syncInterval = presentationParameters.presentInterval & 0xf;

	CComPtr<ID3D12DescriptorHeap> descriptorHeap;
	std::vector<CComPtr<ID3D12Resource>> backBuffers;
	FORWARD_HR( GetBackBuffers( backBuffers, descriptorHeap, m_device, m_swapChain ) );

	m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	m_defaultBackBuffer.m_texture->AssignFromSwapChainDx12( backBuffers, descriptorHeap, m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV ), *this );
	m_defaultBackBuffer.m_texture->SetSwapChainBufferIndexDx12( m_currentBackBufferIndex );

	auto commandAllocator = m_commandAllocators[m_commandAllocatorIndex++ % m_commandAllocators.size()];
	commandAllocator->Reset();
	m_commandList->Reset( commandAllocator, nullptr );

	{
		auto barrier = TrinityALImpl::Transition( m_defaultBackBuffer.m_texture->GetResourceDx12(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET );
		m_commandList->ResourceBarrier( 1, &barrier );
		auto handle = m_defaultBackBuffer.m_texture->GetRtvDescriptorHandleDx12( m_srgbWriteEnable ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR );
		m_commandList->OMSetRenderTargets( 1, &handle, FALSE, nullptr );
	}

	SetViewport( Tr2Viewport( m_defaultBackBuffer.GetWidth(), m_defaultBackBuffer.GetHeight() ) );
	D3D12_RECT rect = {0, 0, m_defaultBackBuffer.GetWidth(), m_defaultBackBuffer.GetHeight() };
	m_commandList->RSSetScissorRects( 1, &rect );
	m_dirtyPso = true;

	m_renderTargetCount = 1;
	std::fill( std::begin( m_renderTargetFormats ), std::end( m_renderTargetFormats ), Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN );
	m_renderTargetFormats[0] = m_srgbWriteEnable ? Tr2RenderContextEnum::MakeSrgb( m_defaultBackBuffer.GetFormat() ) : m_defaultBackBuffer.GetFormat();
	m_depthStencilFormat = Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN;
	m_sampleDesc = m_defaultBackBuffer.GetMsaaDesc();

	return S_OK;
}

const Tr2CapsAL& Tr2PrimaryRenderContextAL::GetCaps() const
{
	return m_caps;
}

ALResult Tr2PrimaryRenderContextAL::Present()
{
	{
		auto barrier = TrinityALImpl::Transition( m_defaultBackBuffer.m_texture->GetResourceDx12(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT );
		m_commandList->ResourceBarrier( 1, &barrier );
	}
	CR( m_commandList->Close() );

	ID3D12CommandList* const commandLists[] = { m_commandList };
	m_commandQueue->ExecuteCommandLists( _countof( commandLists ), commandLists );
	m_frameFenceValues[m_currentBackBufferIndex] = SignalDx12();

	CR( m_swapChain->Present( m_syncInterval, 0 ) );

	m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	WaitForFenceDx12( m_frameFenceValues[m_currentBackBufferIndex] );
	m_pendingRelease[m_currentBackBufferIndex].clear();

	m_defaultBackBuffer.m_texture->SetSwapChainBufferIndexDx12( m_currentBackBufferIndex );

	auto commandAllocator = m_commandAllocators[m_commandAllocatorIndex++ % m_commandAllocators.size()];
	commandAllocator->Reset();
	m_commandList->Reset( commandAllocator, nullptr );

	{
		auto barrier = TrinityALImpl::Transition( m_defaultBackBuffer.m_texture->GetResourceDx12(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET );
		m_commandList->ResourceBarrier( 1, &barrier );
		auto handle = m_defaultBackBuffer.m_texture->GetRtvDescriptorHandleDx12( m_srgbWriteEnable ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR );
		m_commandList->OMSetRenderTargets( 1, &handle, FALSE, nullptr );
	}

	SetViewport( Tr2Viewport( m_defaultBackBuffer.GetWidth(), m_defaultBackBuffer.GetHeight() ) );
	D3D12_RECT rect = {0, 0, m_defaultBackBuffer.GetWidth(), m_defaultBackBuffer.GetHeight() };
	m_commandList->RSSetScissorRects( 1, &rect );

	m_renderTargetCount = 1;
	std::fill( std::begin( m_renderTargetFormats ), std::end( m_renderTargetFormats ), Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN );
	m_renderTargetFormats[0] = m_srgbWriteEnable ? Tr2RenderContextEnum::MakeSrgb( m_defaultBackBuffer.GetFormat() ) : m_defaultBackBuffer.GetFormat();
	m_depthStencilFormat = Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN;
	m_sampleDesc = m_defaultBackBuffer.GetMsaaDesc();

	m_dirtyPso = true;

	return S_OK;
}

Tr2RenderContextEnum::PixelFormat Tr2PrimaryRenderContextAL::GetBackBufferFormat() const
{
	return m_defaultBackBuffer.GetFormat();
}

uint64_t Tr2PrimaryRenderContextAL::SignalDx12()
{
	uint64_t fenceValueForSignal = ++m_fenceValue;
	CR( m_commandQueue->Signal( m_presentFence, fenceValueForSignal ) );
	return fenceValueForSignal;
}

ALResult Tr2PrimaryRenderContextAL::WaitForFenceDx12( uint64_t value )
{
	if( m_presentFence->GetCompletedValue() < value )
	{
		CR_RETURN_HR( m_presentFence->SetEventOnCompletion( value, m_presentFenceEvent ) );
		if( WaitForSingleObject( m_presentFenceEvent, 500 ) == WAIT_TIMEOUT )
		{
			return E_FAIL;
		}
	}
	return S_OK;
}

void Tr2PrimaryRenderContextAL::ReleaseLater( IUnknown* resource )
{
	m_pendingRelease[m_currentBackBufferIndex].push_back( resource );
}

uint64_t Tr2PrimaryRenderContextAL::GetCurrentFrameIndexDx12() const
{
	return m_fenceValue;
}

uint32_t Tr2PrimaryRenderContextAL::GetBackBufferCountDx12() const
{
	return uint32_t( m_pendingRelease.size() );
}

bool Tr2PrimaryRenderContextAL::IsFrameCompletedDx12( uint64_t frameIndex ) const
{
	return m_presentFence->GetCompletedValue() >= frameIndex;
}

void Tr2PrimaryRenderContextAL::OnShaderProgramDestroyedDx12( Tr2ShaderProgramAL* sp )
{
	for( auto it = begin( m_pipelineStates ); it != end( m_pipelineStates ); )
	{
		if( it->second.shaderProgram == sp )
		{
			ReleaseLater( it->second.pipelineState );
			it = m_pipelineStates.erase( it );
		}
		else
		{
			++it;
		}
	}
}

void Tr2PrimaryRenderContextAL::OnVertexLayoutDestroyedDx12( Tr2VertexLayoutAL* vl )
{
	for( auto it = begin( m_pipelineStates ); it != end( m_pipelineStates ); )
	{
		if( it->second.vertexLayout == vl )
		{
			ReleaseLater( it->second.pipelineState );
			it = m_pipelineStates.erase( it );
		}
		else
		{
			++it;
		}
	}
}

ALResult Tr2PrimaryRenderContextAL::FlushDx12( Tr2RenderContextAL& renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}

	CR_RETURN_HR( renderContext.m_commandList->Close() );
	ID3D12CommandList* const commandLists[] = { renderContext.m_commandList };
	m_commandQueue->ExecuteCommandLists( _countof( commandLists ), commandLists );

	auto commandAllocator = m_commandAllocators[m_commandAllocatorIndex++ % m_commandAllocators.size()];
	commandAllocator->Reset();
	renderContext.m_commandList->Reset( commandAllocator, nullptr );
	return S_OK;
}

ALResult Tr2PrimaryRenderContextAL::FlushAndSyncDx12( Tr2RenderContextAL& renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}

	CR_RETURN_HR( renderContext.m_commandList->Close() );
	ID3D12CommandList* const commandLists[] = { renderContext.m_commandList };
	m_commandQueue->ExecuteCommandLists( _countof( commandLists ), commandLists );

	WaitForFenceDx12( SignalDx12() );

	auto commandAllocator = m_commandAllocators[m_commandAllocatorIndex++ % m_commandAllocators.size()];
	commandAllocator->Reset();
	renderContext.m_commandList->Reset( commandAllocator, nullptr );
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
#endif