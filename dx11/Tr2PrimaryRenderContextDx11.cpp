#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2PrimaryRenderContextDx11.h"
#include "ITr2RenderContextEvents.h"
#include "Tr2RenderTargetALDx11.h"
#include "Tr2AdapterStructures.h"
#include "Tr2VideoAdapterInfoALDx11.h"

#include "ALLog.h"


using namespace Tr2RenderContextEnum;

bool g_gatherDX11Statistics = false;
extern bool g_requestDeviceDebugLayer;

CCP_STATS_DECLARE( dx11IAVertices, "Trinity/AL/dx11/IAVertices", false, CST_COUNTER_HIGH, "Number of vertices read by input assembler" );
CCP_STATS_DECLARE( dx11IAPrimitives, "Trinity/AL/dx11/IAPrimitives", false, CST_COUNTER_HIGH, "Number of primitives read by input assembler" );
CCP_STATS_DECLARE( dx11VSInvocations, "Trinity/AL/dx11/VSInvocations", false, CST_COUNTER_HIGH, "Number of times a vertex shader was invoked" );
CCP_STATS_DECLARE( dx11GSInvocations, "Trinity/AL/dx11/GSInvocations", false, CST_COUNTER_HIGH, "Number of times a geometry shader was invoked" );
CCP_STATS_DECLARE( dx11GSPrimitives, "Trinity/AL/dx11/GSPrimitives", false, CST_COUNTER_HIGH, "Number of primitives output by a geometry shader" );
CCP_STATS_DECLARE( dx11CInvocations, "Trinity/AL/dx11/CInvocations", false, CST_COUNTER_HIGH, "Number of primitives that were sent to the rasterizer" );
CCP_STATS_DECLARE( dx11CPrimitives, "Trinity/AL/dx11/CPrimitives", false, CST_COUNTER_HIGH, "Number of primitives that were rendered" );
CCP_STATS_DECLARE( dx11PSInvocations, "Trinity/AL/dx11/PSInvocations", false, CST_COUNTER_HIGH, "Number of times a pixel shader was invoked" );
CCP_STATS_DECLARE( dx11HSInvocations, "Trinity/AL/dx11/HSInvocations", false, CST_COUNTER_HIGH, "Number of times a hull shader was invoked" );
CCP_STATS_DECLARE( dx11DSInvocations, "Trinity/AL/dx11/DSInvocations", false, CST_COUNTER_HIGH, "Number of times a domain shader was invoked" );

namespace Tr2RenderContextImpl {
	struct NullContext;
	extern NullContext s_nullContext;
}

Tr2PrimaryRenderContextAL::Tr2PrimaryRenderContextAL()
	: m_usingEXDevice( false )
	, m_vsyncInterval( 0 ),
	m_adapterVendorId( 0 ),
	m_defaultBackBuffer( new Tr2RenderTargetAL() )
{
	m_context.Attach( (ID3D11DeviceContext*)&Tr2RenderContextImpl::s_nullContext );
}

Tr2PrimaryRenderContextAL::~Tr2PrimaryRenderContextAL()
{
	Destroy();
}

void Tr2PrimaryRenderContextAL::Destroy()
{
	// People say we need to switch to windowed mode before destroying a device
	if( m_swapChain )
	{
		m_swapChain->SetFullscreenState( FALSE, nullptr );
	}

	if( m_defaultBackBuffer )
	{
		m_defaultBackBuffer->Destroy();
	}

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
	
	m_defaultDepthStencil.Destroy();
	m_zeroVertexBuffer.Destroy();

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
	sd.BufferDesc.RefreshRate.Numerator = pp.mode.refreshRateNumerator;
	sd.BufferDesc.RefreshRate.Denominator = pp.mode.refreshRateDenominator;
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
	HR = m_zeroVertexBuffer.Create( sizeof( zeroData ), USAGE_IMMUTABLE, zeroData, *this );
	if( FAILED( HR ) )
	{
		CCP_AL_LOGERR( "Failed to create zero vertex buffer, %d", HR );
		return HR;
	}

	uint32_t zero = 0;
	m_context->IASetVertexBuffers(	VERTEX_BUFFER_ZERO_STREAM_RESERVED, 
									1, &m_zeroVertexBuffer.m_buffer.p, 
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

	m_vsyncInterval = pp.presentInterval & 0xf;

	Tr2RenderContextAL::m_secondarySwapChain = m_swapChain;
	Tr2RenderContextAL::m_secondaryDevice11 = m_d3dDevice11;
	Tr2RenderContextAL::m_context = m_context;
	Tr2RenderContextAL::m_secondaryDefaultBackBuffer = m_defaultBackBuffer;
	Tr2RenderContextAL::m_boundDepthStencil = &m_defaultDepthStencil;

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
	m_dxgiOutput = dxgiOutput;

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

	if( m_defaultBackBuffer )
	{
		m_defaultBackBuffer->Destroy();
	}
	m_defaultDepthStencil.Destroy();

	CR( m_swapChain->ResizeBuffers(	presentationParameters.backBufferCount, 
												presentationParameters.mode.width,
												presentationParameters.mode.height,
												fmt,
												DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH ) );

	if( !presentationParameters.windowed )
	{
		CR( m_swapChain->SetFullscreenState( TRUE, dxgiOutput ) );
		modeDesc.RefreshRate.Numerator = modeDesc.RefreshRate.Denominator = 0;
		CR( m_swapChain->ResizeTarget( &modeDesc ) );
	}
	else
	{
		CR( m_swapChain->SetFullscreenState( FALSE, nullptr ) );
	}

	m_vsyncInterval = presentationParameters.presentInterval & 0xf;

	return CreateBackBuffers( presentationParameters );
}

PixelFormat Tr2PrimaryRenderContextAL::GetBackBufferFormat() const
{
	return m_defaultBackBuffer ? m_defaultBackBuffer->GetFormat() : PIXEL_FORMAT_UNKNOWN;
}

ALResult Tr2PrimaryRenderContextAL::CreateBackBuffers( const Tr2PresentParametersAL& presentationParameters )
{
	if( m_defaultBackBuffer )
	{
		m_defaultBackBuffer->Destroy();
	}
	m_defaultDepthStencil.Destroy();

	CComPtr<ID3D11Texture2D> pBackBuffer;
	HRESULT HR = m_swapChain	? m_swapChain->GetBuffer( 0, __uuidof (ID3D11Texture2D), (LPVOID*)&pBackBuffer ) 
								: E_FAIL;
	if( FAILED(HR) )
	{
		CCP_AL_LOGERR( "Failed to GetBuffer, %d", HR );
		return HR;
	}

	HR = m_defaultBackBuffer ? m_defaultBackBuffer->Attach( pBackBuffer, *this ) : E_FAIL;
	if( FAILED( HR ) )
	{
		CCP_AL_LOGERR( "Failed to attach to device back buffer, %d", HR );
		return HR;
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

	CR_RETURN_HR( m_defaultDepthStencil.Create(		presentationParameters.mode.width, 
													presentationParameters.mode.height, 
													DSFMT_D24S8, 0, 0, *this ) );

	m_context->OMSetRenderTargets(	1, 
									&m_defaultBackBuffer->m_RTV.p, 
									m_defaultDepthStencil.m_depthStencilView );
	m_renderStateEmulation.m_fragmentOpSettings.m_renderTargetSize[0] = viewport.Width ? 1.f / viewport.Width : 0.f;
	m_renderStateEmulation.m_fragmentOpSettings.m_renderTargetSize[1] = viewport.Height ? -1.f / viewport.Height : 0.f;
	m_dirtyFlag = 0xffffffff;

	return S_OK;
}

ALResult Tr2PrimaryRenderContextAL::Present()
{
#if AL_TACK_RESOURCE_USAGE && TRACK_AL_RESOURCES
	extern uint64_t g_trackCurrentFrame;
	++g_trackCurrentFrame;
#endif

	if( g_gatherDX11Statistics )
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
		else if( m_deviceStatistics )
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

bool Tr2PrimaryRenderContextAL::IsSupportedRenderTargetFormat( PixelFormat /*format*/, bool /*withAutoGenMipmap*/ )
{
	return true;	//TODO? 
}

#endif	//DX11?
