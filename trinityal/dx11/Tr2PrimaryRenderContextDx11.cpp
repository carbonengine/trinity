#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2PrimaryRenderContextDx11.h"
#include "ITr2RenderContextEvents.h"
#include "Tr2AdapterStructures.h"
#include "Tr2VideoAdapterInfoALDx11.h"
#include "Tr2BufferALDx11.h"
#include "Tr2TextureALDx11.h"

#include "ALLog.h"


using namespace Tr2RenderContextEnum;

bool g_gatherPipelineStatistics = false;
extern bool g_requestDeviceDebugLayer;

CCP_STATS_DECLARE( dx11IAVertices, "Trinity/AL/Pipeline/IAVertices", false, CST_COUNTER_HIGH, "Number of vertices read by input assembler" );
CCP_STATS_DECLARE( dx11IAPrimitives, "Trinity/AL/Pipeline/IAPrimitives", false, CST_COUNTER_HIGH, "Number of primitives read by input assembler" );
CCP_STATS_DECLARE( dx11VSInvocations, "Trinity/AL/Pipeline/VSInvocations", false, CST_COUNTER_HIGH, "Number of times a vertex shader was invoked" );
CCP_STATS_DECLARE( dx11GSInvocations, "Trinity/AL/Pipeline/GSInvocations", false, CST_COUNTER_HIGH, "Number of times a geometry shader was invoked" );
CCP_STATS_DECLARE( dx11GSPrimitives, "Trinity/AL/Pipeline/GSPrimitives", false, CST_COUNTER_HIGH, "Number of primitives output by a geometry shader" );
CCP_STATS_DECLARE( dx11CInvocations, "Trinity/AL/Pipeline/CInvocations", false, CST_COUNTER_HIGH, "Number of primitives that were sent to the rasterizer" );
CCP_STATS_DECLARE( dx11CPrimitives, "Trinity/AL/Pipeline/CPrimitives", false, CST_COUNTER_HIGH, "Number of primitives that were rendered" );
CCP_STATS_DECLARE( dx11PSInvocations, "Trinity/AL/Pipeline/PSInvocations", false, CST_COUNTER_HIGH, "Number of times a pixel shader was invoked" );
CCP_STATS_DECLARE( dx11HSInvocations, "Trinity/AL/Pipeline/HSInvocations", false, CST_COUNTER_HIGH, "Number of times a hull shader was invoked" );
CCP_STATS_DECLARE( dx11DSInvocations, "Trinity/AL/Pipeline/DSInvocations", false, CST_COUNTER_HIGH, "Number of times a domain shader was invoked" );
CCP_STATS_DECLARE( gpuFrameTime, "Trinity/AL/GpuFrameTime", false, CST_TIME, "Time spent on GPU processing a frame" );

namespace Tr2RenderContextImpl {
	struct NullContext;
	extern NullContext s_nullContext;
}

Tr2PrimaryRenderContextAL::Tr2PrimaryRenderContextAL()
	: m_usingEXDevice( false ), 
	m_vsyncInterval( 0 ), 
	m_adapterVendorId( 0 ), 
	m_deviceStatisticsQueryEmpty( false )
{
	m_context.Attach( (ID3D11DeviceContext*)&Tr2RenderContextImpl::s_nullContext );
	m_defaultBackBuffer.m_texture = std::make_shared<TrinityALImpl::Tr2TextureAL>();
}

Tr2PrimaryRenderContextAL::~Tr2PrimaryRenderContextAL()
{
	if( m_upscalingTechnique )
	{
		m_upscalingTechnique->Destroy(*this);
	}
	Destroy();
}

void Tr2PrimaryRenderContextAL::Destroy()
{
	m_samplerStateFactory.Clear();

	// People say we need to switch to windowed mode before destroying a device
	if( m_swapChain )
	{
		m_swapChain->SetFullscreenState( FALSE, nullptr );
	}

	m_memory.Reset();

	m_defaultBackBuffer.m_texture->Destroy();

	m_usingEXDevice		= false;
	m_deviceStatistics	= nullptr;

	m_d3dDevice11		= nullptr;
	m_swapChain			= nullptr;
	m_dxgiFactory		= nullptr;
	m_dxgiOutput		= nullptr;
	//m_context			= nullptr;
	if( m_aftermathContext )
	{
		GFSDK_Aftermath_ReleaseContextHandle( reinterpret_cast<GFSDK_Aftermath_ContextHandle>( m_aftermathContext ) );
		m_aftermathContext = nullptr;
	}
	m_context.Attach( (ID3D11DeviceContext*)&Tr2RenderContextImpl::s_nullContext );
	
	m_zeroVertexBuffer = Tr2BufferAL();

	m_adapterVendorId = 0;
}

namespace {
	DXGI_FORMAT SafeConvertD3DBackBufferFormat( PixelFormat bbFormat )
	{
		DXGI_FORMAT out = static_cast<DXGI_FORMAT>( bbFormat );
		if( out == DXGI_FORMAT_B8G8R8X8_UNORM )
		{
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		}
		if( out == PIXEL_FORMAT_UNKNOWN )
		{
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		}
		return out;
	}
}

ALResult Tr2PrimaryRenderContextAL::CreateDevice(	uint32_t  adapter, 
												Tr2WindowHandle  hFocusWindow, 
												const Tr2PresentParametersAL& pp )
{

	const bool isWindowless = (hFocusWindow == 0) && pp.software;

	DXGI_SWAP_CHAIN_DESC sd;
	memset( &sd, 0, sizeof( sd ) );

	sd.BufferCount	= 1;
	sd.OutputWindow = Tr2WindowHandle( pp.outputWindow );
	if( !sd.OutputWindow )
	{
		sd.OutputWindow = hFocusWindow;
	}
	sd.Windowed		= true;
	sd.SwapEffect	= DXGI_SWAP_EFFECT( pp.swapEffect );
	sd.Flags        = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	
	sd.SampleDesc.Count = std::max( pp.msaaType, 1u );
	sd.SampleDesc.Quality = pp.msaaQuality;
	
	sd.BufferDesc.Width  = pp.mode.width;
	sd.BufferDesc.Height = pp.mode.height;
	sd.BufferDesc.Format = SafeConvertD3DBackBufferFormat( pp.mode.format );
	sd.BufferDesc.RefreshRate.Numerator = sd.BufferDesc.RefreshRate.Denominator = 0;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING( pp.mode.scaling );
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER( pp.mode.scanlineOrdering );
	
	sd.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;	
	
	uint32_t dwFlags = 0;

	if( g_requestDeviceDebugLayer )
	{
		dwFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}

	const D3D_FEATURE_LEVEL levelWanted = D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL levelSupported;
	CComPtr<IDXGIAdapter1> adapterPtr;

	if( !isWindowless ) 
	{
	
		if( FAILED( Tr2VideoAdapterInfo::GetVideoAdapterDX11( adapter, &adapterPtr, &m_dxgiOutput ) ) )
		{
			return E_FAIL;
		}

		m_adapterVendorId = 0;
		DXGI_ADAPTER_DESC1 desc1;
		if( SUCCEEDED( adapterPtr->GetDesc1( &desc1 ) ) )
		{
			CCP_AL_LOG( "DX11 creating device for adapter %s", (LPCTSTR)CW2A( desc1.Description ) );
			m_adapterVendorId = uint32_t( desc1.VendorId );
		}
	}

	if( m_aftermathContext )
	{
		GFSDK_Aftermath_ReleaseContextHandle( reinterpret_cast<GFSDK_Aftermath_ContextHandle>( m_aftermathContext ) );
		m_aftermathContext = nullptr;
	}
	
	m_context.Release();
	
	D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_UNKNOWN;
	if( pp.software )
	{
		CCP_AL_LOG( "D3D Creating WARP Device" );
		// can't NULL the adapter
		driverType = D3D_DRIVER_TYPE_WARP;
	}

	CCP_AL_LOG( "DX11: driverType: %i", driverType );

	HRESULT HR = 0;
	
	if( isWindowless ) 
	{
		CCP_AL_LOG( "DX11: Creating device without a swap chain" );

		HR = D3D11CreateDevice(
			NULL,
			driverType,
			0,
			dwFlags,
			&levelWanted,
			1,
			D3D11_SDK_VERSION,
			&m_d3dDevice11,
			&levelSupported,
			&m_context );
	} else 	{
		HR = D3D11CreateDeviceAndSwapChain(
				pp.software ? NULL : adapterPtr,
				driverType,
				0,
				dwFlags,
				&levelWanted,
				1,
				D3D11_SDK_VERSION,
				&sd,
				&m_swapChain,
				&m_d3dDevice11,
				&levelSupported,
				&m_context );
	}
	

	if( SUCCEEDED( HR ) )		
	{
		if( m_upscalingTechnique )
		{
			m_upscalingTechnique->AttachToDevice( m_d3dDevice11 );
		}
		//Tr2Streamline::Attach( m_d3dDevice11 );
		CCP_AL_LOG( "DX11: device created succesfully" );
	}
	else		
	{			
		m_swapChain   = nullptr;
		m_d3dDevice11 = nullptr;			
		m_context.Attach( (ID3D11DeviceContext*)&Tr2RenderContextImpl::s_nullContext );

		if( g_requestDeviceDebugLayer )
		{
			// Try once again without DEVICE_DEBUG flag for people without DirectX SDK
			dwFlags &= ~D3D11_CREATE_DEVICE_DEBUG;

			if( isWindowless ) 
			{
				HR = D3D11CreateDevice(
					NULL,
					driverType,
					0,
					dwFlags,
					&levelWanted,
					1,
					D3D11_SDK_VERSION,
					&m_d3dDevice11,
					&levelSupported,
					&m_context );
			} else 	{
				HR = D3D11CreateDeviceAndSwapChain(
						pp.software ? NULL : adapterPtr,
						driverType,
						0,
						dwFlags,
						&levelWanted,
						1,
						D3D11_SDK_VERSION,
						&sd,
						&m_swapChain,
						&m_d3dDevice11,
						&levelSupported,
						&m_context );
			}

			if( SUCCEEDED( HR ) )
			{
				CCP_AL_LOGWARN( "DX11: Created device without DEVICE_DEBUG flag, no error logging will be available" );
				if( m_upscalingTechnique )
				{
					m_upscalingTechnique->AttachToDevice( m_d3dDevice11 );
				}
			}
			else
			{
				CCP_AL_LOG( "DX11: device creation failed, error 0x%08x", HR );
			}
		}
		else
		{
			CCP_AL_LOG( "DX11: device creation failed, error 0x%08x", HR );
		}
	}

	if( !m_d3dDevice11 || !m_context || (!isWindowless && !m_swapChain) )
	{
		CCP_AL_LOGERR( "Failed to D3D11CreateDeviceAndSwapChain" );
		m_swapChain		= nullptr;
		m_d3dDevice11	= nullptr;	
		m_dxgiOutput	= nullptr;
		m_context.Attach( (ID3D11DeviceContext*)&Tr2RenderContextImpl::s_nullContext );
		return E_FAIL;
	}

	m_aftermathContext = nullptr;
	if( ( dwFlags & D3D11_CREATE_DEVICE_DEBUG ) == 0 )
	{
		auto amResult = GFSDK_Aftermath_DX11_Initialize( GFSDK_Aftermath_Version_API, GFSDK_Aftermath_FeatureFlags_Maximum, m_d3dDevice11 );
		switch( amResult )
		{
		case GFSDK_Aftermath_Result_Success:
			CCP_AL_LOG( "Initialized nVidia Aftermath" );
			break;
		case GFSDK_Aftermath_Result_FAIL_VersionMismatch:
			CCP_AL_LOGERR( "Failed to initialize nVidia Aftermath: API/ABI version mismatch" );
			break;
		case GFSDK_Aftermath_Result_FAIL_InvalidAdapter:
			CCP_AL_LOG( "Failed to initialize nVidia Aftermath: unsupported GPU" );
			break;
		case GFSDK_Aftermath_Result_FAIL_InvalidParameter:
			CCP_AL_LOGERR( "Failed to initialize nVidia Aftermath: invalid parameter" );
			break;
		case GFSDK_Aftermath_Result_FAIL_NvApiIncompatible:
			CCP_AL_LOGERR( "Failed to initialize nVidia Aftermath: incompatible NvAPI DLL" );
			break;
		case GFSDK_Aftermath_Result_FAIL_DriverInitFailed:
			CCP_AL_LOG( "Failed to initialize nVidia Aftermath: failed to initialize in the driver" );
			break;
		case GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported:
			CCP_AL_LOG( "Failed to initialize nVidia Aftermath: unsupported driver version" );
			break;
		default:
			CCP_AL_LOG( "Failed to initialize nVidia Aftermath" );
			break;
		}
		if( GFSDK_Aftermath_SUCCEED( amResult ) )
		{
			amResult = GFSDK_Aftermath_DX11_CreateContextHandle( m_context, reinterpret_cast<GFSDK_Aftermath_ContextHandle*>( &m_aftermathContext ) );
			if( GFSDK_Aftermath_SUCCEED( amResult ) )
			{
				CCP_AL_LOG( "Initialized nVidia Aftermath for the primary context" );
			}
			else
			{
				CCP_AL_LOG( "Failed to initialize nVidia Aftermath for the primary context" );
			}
		}
	}

	// Disable Windows support for alt-enter fullscreen
	if( adapterPtr ) 
	{
		adapterPtr->GetParent(__uuidof(IDXGIFactory), (void **)&m_dxgiFactory);

		if( m_dxgiFactory )
		{
			m_dxgiFactory->MakeWindowAssociation( Tr2WindowHandle( pp.outputWindow ), DXGI_MWA_NO_WINDOW_CHANGES );
		}
	}
	
	if( !isWindowless && !pp.windowed )
	{
		CR( m_swapChain->ResizeTarget( &sd.BufferDesc ) );

		CR( m_swapChain->ResizeBuffers(	sd.BufferCount, 
										sd.BufferDesc.Width,
										sd.BufferDesc.Height,
										sd.BufferDesc.Format,
										DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH ) );

		CR( m_swapChain->SetFullscreenState( TRUE, m_dxgiOutput ) );
	}

	float zeroData[4] = { 0.f, 0.f, 0.f, 0.f };
	HR = m_zeroVertexBuffer.Create( 4 * sizeof( float ), 1, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, zeroData, *this );
	if( FAILED( HR ) )
	{
		CCP_AL_LOGERR( "Failed to create zero vertex buffer, %d", HR );
		return HR;
	}

	uint32_t zero = 0;
	m_context->IASetVertexBuffers(	VERTEX_BUFFER_ZERO_STREAM_RESERVED, 
									1, &m_zeroVertexBuffer.m_buffer->m_buffer, 
									&zero, &zero );

	if( !isWindowless ) 
	{
		CR_RETURN_HR( CreateBackBuffers( pp ) );
	}

	if( dwFlags & D3D11_CREATE_DEVICE_DEBUG )
	{
		// Only allow D3D11 errors to be logged
		CComQIPtr<ID3D11InfoQueue> queue( m_d3dDevice11 );
		if( queue )
		{
			D3D11_MESSAGE_SEVERITY severity[] = 
			{
				D3D11_MESSAGE_SEVERITY_CORRUPTION,
				D3D11_MESSAGE_SEVERITY_ERROR,
			};
			
			D3D11_INFO_QUEUE_FILTER filter;
			memset( &filter, 0, sizeof( filter ) );
			filter.AllowList.NumSeverities = 2;
			filter.AllowList.pSeverityList = severity;
			queue->PushRetrievalFilter( &filter );
		}
	}

	m_vsyncInterval = pp.presentInterval;

	Tr2RenderContextAL::m_secondarySwapChain = m_swapChain;
	Tr2RenderContextAL::m_secondaryDevice11 = m_d3dDevice11;
	Tr2RenderContextAL::m_context = m_context;
	Tr2RenderContextAL::m_secondaryDefaultBackBuffer = m_defaultBackBuffer;
	Tr2RenderContextAL::m_boundDepthStencil = Tr2TextureAL();

	m_frameTimer.Create( *this );

	if( m_events )
	{
		m_events->OnContextCreated( *this );
	}
	
	return S_OK;
}

ALResult Tr2PrimaryRenderContextAL::SetPresentParameters( unsigned adapter, const Tr2PresentParametersAL& presentationParameters )
{
	if( !m_swapChain )
	{
		return E_FAIL;
	}

	CComPtr<IDXGIOutput> dxgiOutput;
	CComPtr<IDXGIAdapter1> adapterPtr;
	if( FAILED( Tr2VideoAdapterInfo::GetVideoAdapterDX11( adapter, &adapterPtr, &dxgiOutput ) ) )
	{
		return E_FAIL;
	}

	if( !presentationParameters.windowed && dxgiOutput != m_dxgiOutput )
	{
		// If we are switching between two monitors in fullscreen mode it seems we first should
		// go windowed and then fullscreen on another monitor, otherwise DXGI behaves funny.
		CR( m_swapChain->SetFullscreenState( FALSE, nullptr ) );
	}
	else if( presentationParameters.windowed )
	{
		CR( m_swapChain->SetFullscreenState( FALSE, nullptr ) );
	}

	m_dxgiOutput = dxgiOutput;

	DXGI_FORMAT fmt = SafeConvertD3DBackBufferFormat( presentationParameters.mode.format );

	DXGI_MODE_DESC modeDesc;
	modeDesc.Width = presentationParameters.mode.width;
	modeDesc.Height = presentationParameters.mode.height;
	modeDesc.RefreshRate.Numerator = modeDesc.RefreshRate.Denominator = 0;
	modeDesc.Format = fmt;
	modeDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER( presentationParameters.mode.scanlineOrdering );
	modeDesc.Scaling = DXGI_MODE_SCALING( presentationParameters.mode.scaling );

	m_defaultBackBuffer.m_texture->Destroy();

	CR( m_swapChain->ResizeBuffers(	presentationParameters.backBufferCount, 
												presentationParameters.mode.width,
												presentationParameters.mode.height,
												fmt,
												DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH ) );

	if( !presentationParameters.windowed )
	{
		CR( m_swapChain->ResizeTarget( &modeDesc ) );
		CR( m_swapChain->SetFullscreenState( TRUE, dxgiOutput ) );
	}
	else
	{
		CR( m_swapChain->SetFullscreenState( FALSE, nullptr ) );
	}

	m_vsyncInterval = presentationParameters.presentInterval;

	m_frameTimer.Create( *this );

	return CreateBackBuffers( presentationParameters );
}

PixelFormat Tr2PrimaryRenderContextAL::GetBackBufferFormat() const
{
	return m_defaultBackBuffer.GetDesc().GetFormat();
}

ALResult Tr2PrimaryRenderContextAL::CreateBackBuffers( const Tr2PresentParametersAL& presentationParameters )
{
	m_defaultBackBuffer.m_texture->Destroy();

	CComPtr<ID3D11Texture2D> pBackBuffer;
	HRESULT HR = m_swapChain	? m_swapChain->GetBuffer( 0, __uuidof (ID3D11Texture2D), (LPVOID*)&pBackBuffer ) 
								: E_FAIL;
	if( FAILED(HR) )
	{
		CCP_AL_LOGERR( "Failed to GetBuffer, %d", HR );
		return HR;
	}

	HR = m_defaultBackBuffer.m_texture->Attach( pBackBuffer, *this );
	if( FAILED( HR ) )
	{
		CCP_AL_LOGERR( "Failed to attach to device back buffer, %d", HR );
		return HR;
	}
	if( m_defaultBackBuffer.IsValid() )
	{
		m_memory.Set( Tr2MemoryCounterAL::TEXTURE, m_defaultBackBuffer.GetDesc(), m_defaultBackBuffer.GetMsaaDesc() );
	}
	
	D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX	= 0;
    viewport.TopLeftY	= 0;
	viewport.Width		= (float)presentationParameters.mode.width;
	viewport.Height		= (float)presentationParameters.mode.height;
	viewport.MinDepth   = 0;
	viewport.MaxDepth   = 1.0f;

	m_context->RSSetViewports( 1, &viewport );

	Tr2RenderContextAL::m_boundDepthStencil = Tr2TextureAL();

	SetRenderTarget( m_defaultBackBuffer );

	m_dirtyFlag.flags = 0xffffffff;

	return S_OK;
}

ALResult Tr2PrimaryRenderContextAL::Present()
{
	m_frameTimer.End( *this );
	auto frameTime = m_frameTimer.GetTime( *this );
	if( frameTime > 0 )
	{
#if CCP_STATS_ENABLED
		g_ccpStatistics_gpuFrameTime.Set( frameTime );
#endif
	}

	if( g_gatherPipelineStatistics )
	{
		if( !m_deviceStatistics )
		{
			D3D11_QUERY_DESC queryDesc;
			queryDesc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
			queryDesc.MiscFlags = 0;
			m_d3dDevice11->CreateQuery( &queryDesc, &m_deviceStatistics );
			m_context->Begin( m_deviceStatistics );
			m_deviceStatisticsQueryEmpty = false;
		}
		else
		{
			if( !m_deviceStatisticsQueryEmpty )
			{
				m_context->End( m_deviceStatistics );
				m_deviceStatisticsQueryEmpty = true;
			}

			D3D11_QUERY_DATA_PIPELINE_STATISTICS GPUProbeStats;
			if( m_deviceStatisticsQueryEmpty && 
				m_context->GetData( m_deviceStatistics, &GPUProbeStats, sizeof( GPUProbeStats ), 0 ) == S_OK )
			{
				CCP_STATS_SET( dx11IAVertices, GPUProbeStats.IAVertices );
				CCP_STATS_SET( dx11IAPrimitives, GPUProbeStats.IAPrimitives );
				CCP_STATS_SET( dx11VSInvocations, GPUProbeStats.VSInvocations );
				CCP_STATS_SET( dx11GSInvocations, GPUProbeStats.GSInvocations );
				CCP_STATS_SET( dx11GSPrimitives, GPUProbeStats.GSPrimitives );
				CCP_STATS_SET( dx11CInvocations, GPUProbeStats.CInvocations );
				CCP_STATS_SET( dx11CPrimitives, GPUProbeStats.CPrimitives );
				CCP_STATS_SET( dx11PSInvocations, GPUProbeStats.PSInvocations );
				CCP_STATS_SET( dx11HSInvocations, GPUProbeStats.HSInvocations );
				CCP_STATS_SET( dx11DSInvocations, GPUProbeStats.DSInvocations );

				m_context->Begin( m_deviceStatistics );
				m_deviceStatisticsQueryEmpty = false;
			}
		}
	}

	if( m_swapChain )
	{
		m_swapChain->Present( m_vsyncInterval, 0 );
	} else {
		m_context->Flush();
	}


	CComQIPtr<ID3D11InfoQueue> queue( m_d3dDevice11 );
	if( queue )
	{
		static std::vector<char> buffer;
		UINT64 count = queue->GetNumStoredMessagesAllowedByRetrievalFilter();
		for( UINT64 i = 0; i < count; ++i )
		{
			SIZE_T messageLength;
			if( SUCCEEDED( queue->GetMessage( i, nullptr, &messageLength ) ) )
			{
				if( buffer.size() < messageLength )
				{
					buffer.resize( messageLength );
				}
				D3D11_MESSAGE* message = (D3D11_MESSAGE*)&buffer[0];
				if( SUCCEEDED( queue->GetMessage( i, message, &messageLength ) ) )
				{
					switch( message->Severity )
					{
					case D3D11_MESSAGE_SEVERITY_CORRUPTION:
					case D3D11_MESSAGE_SEVERITY_ERROR:
						CCP_AL_LOGERR( "%s", message->pDescription );
						break;
					case D3D11_MESSAGE_SEVERITY_WARNING:
						CCP_AL_LOGWARN( "%s", message->pDescription );
						break;
					case D3D11_MESSAGE_SEVERITY_INFO:
						CCP_AL_LOGWARN( "%s", message->pDescription );
						break;
					}
				}
			}
		}
		queue->ClearStoredMessages();
	}

	m_frameTimer.Begin( *this );
	SetRenderTarget( m_defaultBackBuffer );

	return S_OK;
}

bool Tr2PrimaryRenderContextAL::IsValid()
{
	return m_d3dDevice11 != nullptr && m_context.p != (ID3D11DeviceContext*)&Tr2RenderContextImpl::s_nullContext;
}

const Tr2CapsAL& Tr2PrimaryRenderContextAL::GetCaps() const
{
	return m_caps;
}


Tr2UpscalingAL::Result Tr2PrimaryRenderContextAL::EnableUpscaling( Tr2UpscalingAL::Technique tech, Tr2UpscalingAL::Setting setting, bool frameGeneration, uint32_t adapter )
{
	if( tech == Tr2UpscalingAL::Technique::NONE )
	{
		if( m_upscalingTechnique )
		{
			m_upscalingTechnique->Destroy( *this );
			m_upscalingTechnique = nullptr;
		}
		return Tr2UpscalingAL::Result::OK;
	}

	// find the technique
	auto supportedTechnique = std::find( TrinityALImpl::AVAILABLE_UPSCALING_TECHNIQUES.begin(), TrinityALImpl::AVAILABLE_UPSCALING_TECHNIQUES.end(), tech );
	if( supportedTechnique == TrinityALImpl::AVAILABLE_UPSCALING_TECHNIQUES.end() )
	{
		m_upscalingTechnique = nullptr;
		return Tr2UpscalingAL::Result::TECHNIQUE_NOT_SUPPORTED;
	}

	m_upscalingTechnique = TrinityALImpl::CreateUpscalingTechnique( *this, tech, setting, frameGeneration, adapter );
	if( m_upscalingTechnique == nullptr )
	{
		return Tr2UpscalingAL::Result::TECHNIQUE_NOT_SUPPORTED;
	}
	auto result = m_upscalingTechnique->Setup();
	if( result != Tr2UpscalingAL::Result::OK )
	{
		delete m_upscalingTechnique;
		m_upscalingTechnique = nullptr;
	}
	return result;
}


Tr2UpscalingContextAL* Tr2PrimaryRenderContextAL::GetUpscalingContext( uint32_t displayWidth, uint32_t displayHeight )
{
	if( m_upscalingTechnique == nullptr )
	{
		return nullptr;
	}

	return m_upscalingTechnique->GetContext( *this, displayWidth, displayHeight );
}

Tr2UpscalingContextAL* Tr2PrimaryRenderContextAL::CreateUpscalingContext( uint32_t displayWidth, uint32_t displayHeight, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat )
{
	if( m_upscalingTechnique == nullptr )
	{
		return nullptr;
	}

	return m_upscalingTechnique->CreateContext( *this, displayWidth, displayHeight, sourceFormat, depthFormat );
}

void Tr2PrimaryRenderContextAL::GetUpscalingSetup( Tr2UpscalingAL::Technique& technique, Tr2UpscalingAL::Setting& setting, bool& framegeneration )
{
	if( m_upscalingTechnique )
	{
		m_upscalingTechnique->GetState( technique, setting, framegeneration );
		return;
	}
	technique = Tr2UpscalingAL::Technique::NONE;
	setting = Tr2UpscalingAL::Setting::NATIVE;
	framegeneration = false;
}

Tr2UpscalingAL::UpscalingInfo Tr2PrimaryRenderContextAL::GetUpscalingInfo( uint32_t displayWidth, uint32_t displayHeight )
{
	auto context = GetUpscalingContext( displayWidth, displayHeight );
	Tr2UpscalingAL::UpscalingInfo info = Tr2UpscalingAL::UpscalingInfo();

	if( context != nullptr )
	{
		info.upscalingAmount = context->GetUpscalingAmount();
		info.mipLevelBias = context->GetMipLevelBias();
		info.temporal = context->IsTemporal();
		context->GetJitter( info.jitterX, info.jitterY );
		context->GetRenderDimensions( info.renderWidth, info.renderHeight );
		m_upscalingTechnique->GetState( info.technique, info.setting, info.frameGeneration );
	}
	return info;
}

void Tr2PrimaryRenderContextAL::MarkFrameEvent( Tr2RenderContextEnum::FrameEvent frameEvent )
{
	if( m_upscalingTechnique )
	{
		m_upscalingTechnique->MarkFrameEvent( *this, frameEvent );
	}
}

std::vector<std::tuple<Tr2UpscalingAL::Technique, uint32_t, bool>> Tr2PrimaryRenderContextAL::GetSupportedUpscalingTechniques( uint32_t adapter )
{
	std::vector<std::tuple<Tr2UpscalingAL::Technique, uint32_t, bool>> supportedTechniques;
	for( auto& technique : TrinityALImpl::AVAILABLE_UPSCALING_TECHNIQUES )
	{
		auto tech = TrinityALImpl::CreateUpscalingTechnique( *this, technique, Tr2UpscalingAL::Setting::NATIVE, false, adapter );
		if( tech )
		{
			uint32_t allSettings = 0;

			for( auto& setting : tech->GetAvailableSettings() )
			{
				allSettings |= setting;
			}

			supportedTechniques.push_back( { technique, allSettings, tech->SupportsFrameGeneration() } );
		}
	}
	return supportedTechniques;
}

#endif	//DX11?
