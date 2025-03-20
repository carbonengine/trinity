#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2VideoAdapterInfoALDx11.h"
#include "Tr2AdapterStructures.h"

using namespace Tr2RenderContextEnum;

namespace
{

//TODO does this survive a driver crash?

CComPtr<IDXGIFactory1> s_factory;


struct AdapterInfo
{
	CComPtr<IDXGIAdapter1> m_adapter;
	CComPtr<IDXGIOutput> m_output;

	DXGI_OUTPUT_DESC m_outputDesc;
	DXGI_ADAPTER_DESC1 m_desc;

	std::map<DXGI_FORMAT, std::vector<DXGI_MODE_DESC>> m_displayModes;

	size_t m_deviceInfoIndex;
};

struct DeviceInfo
{
	static const unsigned FORMAT_COUNT = 100;

	uint32_t m_formatSupport[FORMAT_COUNT];
};

std::vector<AdapterInfo> s_adapters;
std::vector<DeviceInfo> s_deviceInfo;

HRESULT GetFallbackMode( DXGI_MODE_DESC &desc )
{
	DEVMODE devMode;
	memset(&devMode, 0, sizeof(devMode));
	devMode.dmSize = sizeof(devMode);
	if (!EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &devMode))
	{
		return E_FAIL;
	}

	desc.Width = devMode.dmPelsWidth;
	desc.Height = devMode.dmPelsHeight;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.RefreshRate.Numerator = 0;
	desc.RefreshRate.Denominator = 1;
	desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	return S_OK;
}

ALResult PopulateDisplayModes( AdapterInfo& output, DXGI_FORMAT backBufferFormat )
{
	auto inserted = output.m_displayModes.insert( std::make_pair( backBufferFormat, std::vector<DXGI_MODE_DESC>() ) );
	auto& displayModes = inserted.first->second;
	if( !inserted.second )
	{
		return S_OK;
	}

	uint32_t count = 0;
	HRESULT hr = output.m_output->GetDisplayModeList( 
		backBufferFormat,
		0,
		&count,
		nullptr );
	if( hr == E_FAIL && backBufferFormat == DXGI_FORMAT_B8G8R8A8_UNORM && output.m_deviceInfoIndex == 0 )
	{
		CCP_LOGWARN( "Failed to get available display modes. Consider updating video driver." );
		displayModes.resize( 1 );
		return GetFallbackMode( displayModes[0] );
	}
	if ( hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE && backBufferFormat == DXGI_FORMAT_B8G8R8A8_UNORM )
	{
		CCP_LOGWARN( "Tr2VideoAdapterInfo: failed to enumerate display modes (running in remote desktop?)" );
		count = 1;
	}
	else if ( FAILED( hr ) || (count == 0) )
	{
		return hr;
	}

	CCP_ASSERT( count > 0 );

	displayModes.resize( count );
	hr = output.m_output->GetDisplayModeList( static_cast<DXGI_FORMAT>( backBufferFormat ),
		0,
		&count,
		&displayModes[0] );
	if ( hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE && backBufferFormat == DXGI_FORMAT_B8G8R8A8_UNORM && !displayModes.empty() )
	{
		return GetFallbackMode( displayModes[0] );
	}
	else if ( FAILED( hr ) )
	{
		return hr;
	}
	return S_OK;
}

ALResult InitializeDirect3D()
{
	s_adapters.resize( 0 );
	s_deviceInfo.resize( 0 );

	s_factory = nullptr;

	CR_RETURN_HR( CreateDXGIFactory1(
							__uuidof( IDXGIFactory1 ),
							(void**)( &s_factory ) ) );

	const D3D_FEATURE_LEVEL levelWanted = D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL levelSupported;
	uint32_t dwFlags = 0;

	if ( !s_factory )
	{
		return E_FAIL;
	}

	CComPtr<IDXGIAdapter1> adapterPtr;

	uint32_t adapterIndex = 0;
	while ( SUCCEEDED( s_factory->EnumAdapters1( adapterIndex++, &adapterPtr ) ) )
	{
		CComPtr<ID3D11Device> device;
		CComPtr<ID3D11DeviceContext> context;

		if ( FAILED( D3D11CreateDevice( adapterPtr,
			 D3D_DRIVER_TYPE_UNKNOWN,
			 0, dwFlags,
			 &levelWanted, 1,
			 D3D11_SDK_VERSION,
			 &device,
			 &levelSupported,
			 &context ) ) )
		{
			continue;
		}

		DXGI_ADAPTER_DESC1 adapterDesc;
		CR_RETURN_HR( adapterPtr->GetDesc1( &adapterDesc ) );

		CCP_LOG( "Found video adapter \"%S\"", adapterDesc.Description );

		DeviceInfo deviceInfo;
		memset( deviceInfo.m_formatSupport, 0, sizeof( deviceInfo.m_formatSupport ) );
		for ( unsigned i = 0; i < DeviceInfo::FORMAT_COUNT; ++i )
		{
			device->CheckFormatSupport( static_cast<DXGI_FORMAT>( i ), deviceInfo.m_formatSupport + i );
		}

		CComPtr<IDXGIOutput> outputPtr;
		uint32_t outputIndex = 0;
		bool hasValidOutputs = false;
		while ( SUCCEEDED( adapterPtr->EnumOutputs( outputIndex++, &outputPtr ) ) )
		{
			AdapterInfo adapter;
			adapter.m_adapter = adapterPtr;
			adapter.m_desc = adapterDesc;
			adapter.m_output = outputPtr;
			adapter.m_deviceInfoIndex = s_deviceInfo.size();

			CR_RETURN_HR( outputPtr->GetDesc( &adapter.m_outputDesc ) );

			CCP_LOG( "Found video adapter output \"%S\"", adapter.m_outputDesc.DeviceName );

			CR_RETURN_HR( PopulateDisplayModes( adapter, DXGI_FORMAT_B8G8R8A8_UNORM ) );
			if( !adapter.m_displayModes[DXGI_FORMAT_B8G8R8A8_UNORM].empty() )
			{
				hasValidOutputs = true;
				s_adapters.push_back( adapter );
			}
			outputPtr.Release();
		}
		if ( hasValidOutputs )
		{
			s_deviceInfo.push_back( deviceInfo );
		}

		adapterPtr.Release();
	}

	return S_OK;
}

}

#define	CHECK_INIT	\
	if( !s_factory )							\
	{											\
		CR_RETURN_HR( InitializeDirect3D() );	\
	}

#define CHECK_VALID_ADAPTER	\
	if( adapterIndex >= s_adapters.size() )		\
	{											\
		return E_INVALIDARG;					\
	}

#define	CHECK_INIT_BOOL	\
	if( !s_factory && !SUCCEEDED( InitializeDirect3D() ) )	\
	{														\
		return false;										\
	}

#define CHECK_VALID_ADAPTER_BOOL	\
	if( adapterIndex >= s_adapters.size() )		\
	{											\
		return false;							\
	}

ALResult Tr2VideoAdapterInfo::GetAdapterCount( uint32_t& count )
{
	CHECK_INIT;

	count = static_cast<uint32_t>( s_adapters.size() );
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterInfo( unsigned adapterIndex,
											  Tr2AdapterInfo& info )
{
	CHECK_INIT;
	CHECK_VALID_ADAPTER;

	const auto& desc = s_adapters[adapterIndex].m_desc;
	const auto& outp = s_adapters[adapterIndex].m_outputDesc;

	info.driver = "";
	info.description = desc.Description;
	info.deviceName = CW2A( outp.DeviceName );
	info.driverVersion = 0;
	info.vendorID = desc.VendorId;
	info.deviceID = desc.DeviceId;
	info.subSystemID = desc.SubSysId;
	info.revision = desc.Revision;
	memcpy(info.luid, &desc.AdapterLuid, sizeof(LUID));
	memset( &info.deviceIdentifier, 0, sizeof( info.deviceIdentifier ) );

	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMonitor( unsigned adapterIndex,
												 void*& monitor )
{
	CHECK_INIT;
	CHECK_VALID_ADAPTER;

	monitor = s_adapters[adapterIndex].m_outputDesc.Monitor;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterDisplayMode( unsigned adapterIndex,
													 Tr2DisplayModeInfo& mode )
{
	CHECK_INIT;
	CHECK_VALID_ADAPTER;

	MONITORINFOEX monitorInfo;
	monitorInfo.cbSize = sizeof( MONITORINFOEX );
	if( !GetMonitorInfo( s_adapters[adapterIndex].m_outputDesc.Monitor, &monitorInfo ) )
	{
		return E_FAIL;
	}

	const auto & mm = s_adapters[adapterIndex].m_displayModes[DXGI_FORMAT_B8G8R8A8_UNORM][0];
	mode.format = static_cast<PixelFormat>( mm.Format );
	mode.refreshRateDenominator = mm.RefreshRate.Denominator;
	mode.refreshRateNumerator = mm.RefreshRate.Numerator;
	mode.scaling = static_cast<DisplayScaling>( mm.Scaling );
	mode.scanlineOrdering = static_cast<ScanlineOrdering>( mm.ScanlineOrdering );

	DEVMODE devMode;
	memset( &devMode, 0, sizeof( devMode ) );
	devMode.dmSize = sizeof( devMode );
	if ( !EnumDisplaySettings( monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &devMode ) )
	{
		return E_FAIL;
	}
	mode.width = devMode.dmPelsWidth;
	mode.height = devMode.dmPelsHeight;

	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterModeCount( unsigned adapterIndex,
												   Tr2RenderContextEnum::PixelFormat backBufferFormat,
												   unsigned& count )
{
	CHECK_INIT;
	CHECK_VALID_ADAPTER;

	count = 0;

	auto & output = s_adapters[adapterIndex];
	CR_RETURN_HR( PopulateDisplayModes( output, static_cast<DXGI_FORMAT>( backBufferFormat ) ) );

	count = unsigned( output.m_displayModes[static_cast<DXGI_FORMAT>( backBufferFormat )].size() );
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMode( unsigned adapterIndex,
											  Tr2RenderContextEnum::PixelFormat backBufferFormat,
											  unsigned modeIndex,
											  Tr2DisplayModeInfo& mode )
{
	CHECK_INIT;
	CHECK_VALID_ADAPTER;

	auto & output = s_adapters[adapterIndex];
	CR_RETURN_HR( PopulateDisplayModes( output, static_cast<DXGI_FORMAT>( backBufferFormat ) ) );
	auto& displayModes = output.m_displayModes[static_cast<DXGI_FORMAT>( backBufferFormat )];

	if ( modeIndex >= displayModes.size() )
	{
		return E_INVALIDARG;
	}

	auto & mm = displayModes[modeIndex];

	mode.format = static_cast<PixelFormat>( mm.Format );
	mode.height = mm.Height;
	mode.refreshRateDenominator = mm.RefreshRate.Denominator;
	mode.refreshRateNumerator = mm.RefreshRate.Numerator;
	mode.scaling = static_cast<DisplayScaling>( mm.Scaling );
	mode.scanlineOrdering = static_cast<ScanlineOrdering>( mm.ScanlineOrdering );
	mode.width = mm.Width;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMaxTextureWidth( unsigned/*adapterIndex*/,
														 unsigned& maxWidth )
{
	maxWidth = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
	return S_OK;
}

bool Tr2VideoAdapterInfo::SupportsBackBufferFormat( unsigned adapterIndex,
													Tr2RenderContextEnum::PixelFormat backBufferFormat )
{
	CHECK_INIT_BOOL;
	CHECK_VALID_ADAPTER_BOOL;

	if ( backBufferFormat >= (unsigned)DeviceInfo::FORMAT_COUNT )
	{
		return false;
	}

	uint32_t flags = s_deviceInfo[s_adapters[adapterIndex].m_deviceInfoIndex].m_formatSupport[backBufferFormat];

	// or should it be D3D11_FORMAT_SUPPORT_DISPLAY?
	if ( ( flags & D3D11_FORMAT_SUPPORT_RENDER_TARGET ) == 0 )
	{
		return false;
	}

	return true;
}

bool Tr2VideoAdapterInfo::SupportsRenderTargetFormat( unsigned adapterIndex, Tr2RenderContextEnum::PixelFormat format )
{
	CHECK_INIT_BOOL;
	CHECK_VALID_ADAPTER_BOOL;

	if ( format >= (unsigned)DeviceInfo::FORMAT_COUNT )
	{
		return false;
	}

	uint32_t flags = s_deviceInfo[s_adapters[adapterIndex].m_deviceInfoIndex].m_formatSupport[format];

	if ( ( flags & D3D11_FORMAT_SUPPORT_RENDER_TARGET ) == 0 )
	{
		return false;
	}

	return true;
}

unsigned log2( unsigned int x )
{
	unsigned ans = 0;
	while ( x >>= 1 )
	{
		ans++;
	}
	return ans;
}

ALResult Tr2VideoAdapterInfo::RefreshData()
{
	return InitializeDirect3D();
}

bool Tr2VideoAdapterInfo::AreAdaptersDifferent( unsigned adapter1,
												unsigned adapter2 )
{
	if ( adapter1 == adapter2 )
	{
		return false;
	}
	if ( adapter1 >= s_adapters.size() || adapter2 >= s_adapters.size() )
	{
		return true;
	}

	return s_adapters[adapter1].m_adapter != s_adapters[adapter2].m_adapter;
}

// --------------------------------------------------------------------------------------
// Description:
//   Method returns DX11 adapter and output interfaces for the given adapterIndex. This
//   method is platform-specific and is used by Tr2PimaryRenderContext.
// Arguments:
//   adapterIndex - Adapter index
//   adapter - (out) DX11 adapter interface
//   output - (out) DX11 output interface
// Return Value:
//   HRESULT of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapterInfo::GetVideoAdapterDX11( unsigned adapterIndex,
												   IDXGIAdapter1** adapter,
												   IDXGIOutput** output )
{
	if ( !s_factory )
	{
		CR_RETURN_HR( InitializeDirect3D() );
	}
	if ( adapterIndex >= s_adapters.size() )
	{
		return E_INVALIDARG;
	}
	*adapter = s_adapters[adapterIndex].m_adapter;
	( *adapter )->AddRef();
	*output = s_adapters[adapterIndex].m_output;
	( *output )->AddRef();
	return S_OK;
}

#endif
