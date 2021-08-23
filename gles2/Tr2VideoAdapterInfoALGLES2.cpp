#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "Tr2VideoAdapterInfoALGLES2.h"
#include "Tr2AdapterStructures.h"

#ifndef _WIN32

ALResult Tr2VideoAdapterInfo::GetAdapterCount( unsigned& count )
{
	count = 1;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterInfo( unsigned adapterIndex, Tr2AdapterInfo& info )
{
	if( adapterIndex )
	{
		return E_INVALIDARG;
	}
	info.driver = "na";
	info.description = L"na";
	info.deviceName = "defaultDevice";
	info.driverVersion = 0;
	info.vendorID = 0;
	info.deviceID = 0;
	info.subSystemID = 0;
	info.revision = 0;
    memset( &info.deviceIdentifier, 0, sizeof( info.deviceIdentifier ) );
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMonitor( unsigned adapterIndex, void*& monitor )
{
	if( adapterIndex )
	{
		return E_INVALIDARG;
	}
#if defined(TRINITY_AL_MOBILE)
	auto display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
	if( display == EGL_NO_DISPLAY )
	{
		return E_FAIL;
	}
	monitor = display;
#else
	monitor = nullptr;
#endif
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterDisplayMode( unsigned adapterIndex, Tr2DisplayModeInfo& mode )
{
	if( adapterIndex )
	{
		return E_INVALIDARG;
	}
#if defined(TRINITY_AL_MOBILE)
	static EGLint width = 0;
	static EGLint height = 0;
	if( !width )
	{
		const EGLint attribs[] = {
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_BLUE_SIZE, 8,
			EGL_GREEN_SIZE, 8,
			EGL_RED_SIZE, 8,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL_NONE };
 
		InternalInitializeEgl();
		auto display = eglGetDisplay( EGL_DEFAULT_DISPLAY );

		EGLConfig config;
		EGLint numConfigs;
    
		if( !eglChooseConfig( display, attribs, &config, 1, &numConfigs ) )
		{
			return E_FAIL;
		}

		extern ANativeWindow* g_androidWindow;
		EGLSurface surface = eglCreateWindowSurface( display, config, g_androidWindow, nullptr );
		ON_BLOCK_EXIT( [&] { eglDestroySurface( display, surface ); } );
    
		if( !eglQuerySurface( display, surface, EGL_HEIGHT, &height ) )
		{
			return E_FAIL;
		}
		if( !eglQuerySurface( display, surface, EGL_WIDTH, &width ) )
		{
			return E_FAIL;
		}
	}
	mode.width = width;
	mode.height = height;
#else
	mode.width = 1024;
	mode.height = 768;
#endif
	mode.refreshRateNumerator = 1;
	mode.refreshRateDenominator = 60;
	mode.format = Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM;
	mode.scanlineOrdering = Tr2RenderContextEnum::SCANLINE_ORDER_UNSPECIFIED;
	mode.scaling = Tr2RenderContextEnum::DISPLAY_SCALING_UNSPECIFIED;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterModeCount( 
	unsigned adapterIndex,
	Tr2RenderContextEnum::PixelFormat backBufferFormat,
	unsigned& count )
{
	if( adapterIndex )
	{
		return E_INVALIDARG;
	}
	count = backBufferFormat == Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM ? 1 : 0;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMode( 
	unsigned adapterIndex,
	Tr2RenderContextEnum::PixelFormat backBufferFormat,
	unsigned modeIndex,
	Tr2DisplayModeInfo& mode )
{
	if( adapterIndex )
	{
		return E_INVALIDARG;
	}
	if( backBufferFormat != Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM )
	{
		return E_INVALIDARG;
	}
	if( modeIndex )
	{
		return E_INVALIDARG;
	}

	mode.width = 1024;
	mode.height = 768;
	mode.refreshRateNumerator = 1;
	mode.refreshRateDenominator = 60;
	mode.format = Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM;
	mode.scanlineOrdering = Tr2RenderContextEnum::SCANLINE_ORDER_UNSPECIFIED;
	mode.scaling = Tr2RenderContextEnum::DISPLAY_SCALING_UNSPECIFIED;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMsaaSupport( 
	unsigned adapterIndex,
	Tr2RenderContextEnum::PixelFormat format,
	bool windowed,
	unsigned msaaType,
	unsigned& msaaQuality )
{
	if( adapterIndex )
	{
		return E_INVALIDARG;
	}
	msaaQuality = 0;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMsaaSupport( 
	unsigned adapterIndex,
	Tr2RenderContextEnum::DepthStencilFormat format,
	bool windowed,
	unsigned msaaType,
	unsigned& msaaQuality )
{
	if( adapterIndex )
	{
		return E_INVALIDARG;
	}
	msaaQuality = 0;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterShaderVersion( 
	unsigned adapterIndex,
	unsigned& version )
{
	version = 3 << 8;
	return S_OK;
}

bool Tr2VideoAdapterInfo::SupportsBackBufferFormat( 
	unsigned adapterIndex,
	Tr2RenderContextEnum::PixelFormat backBufferFormat,
	bool windowed )
{
	if( adapterIndex )
	{
		return false;
	}
	return backBufferFormat == Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM;
}

bool Tr2VideoAdapterInfo::SupportsRenderTargetFormat( 
	unsigned adapterIndex,
	Tr2RenderContextEnum::PixelFormat backBufferFormat,
	Tr2RenderContextEnum::PixelFormat format,
	bool withAutoGenMipmap )
{
	return true;
}

bool Tr2VideoAdapterInfo::SupportsDepthStencilFormat( 
	unsigned adapterIndex,
	Tr2RenderContextEnum::PixelFormat backBufferFormat,
	Tr2RenderContextEnum::DepthStencilFormat format )
{
	return true;
}

bool Tr2VideoAdapterInfo::SupportsVertexTextureFormat( 
	unsigned adapterIndex,
	Tr2RenderContextEnum::PixelFormat backBufferFormat,
	Tr2RenderContextEnum::PixelFormat format )
{
	return true;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMaxTextureWidth( 
	unsigned adapterIndex,
	unsigned& maxWidth )
{
	maxWidth = 4096;
	return S_OK;
}

bool Tr2VideoAdapterInfo::AreAdaptersDifferent( unsigned adapter1, unsigned adapter2 )
{
	return false;
}

ALResult Tr2VideoAdapterInfo::RefreshData()
{
	return S_OK;
}

#else

extern bool g_usingEXDevice;

using namespace Tr2RenderContextEnum;

namespace
{


std::vector<std::pair<uint32_t, Tr2AdapterInfo>> s_devices;


bool GetHexIdFromDeviceId( const wchar_t* deviceId, uint32_t& deviceIdHex, const wchar_t* prefix )
{
	auto found = wcsstr( deviceId, prefix );
	if( !found )
	{
		return false;
	}
	return swscanf_s( found + wcslen( prefix ), L"%x", &deviceIdHex ) == 1;
}

void GetDeviceInfo( const DISPLAY_DEVICEW& device, Tr2AdapterInfo& info )
{
	info.description =  device.DeviceString;
	info.deviceName = CW2A( device.DeviceName );
	info.vendorID = 0;
	info.deviceID = 0;
	GetHexIdFromDeviceId( device.DeviceID, info.vendorID, L"VEN_" );
	GetHexIdFromDeviceId( device.DeviceID, info.deviceID, L"DEV_" );
}

void GetVideoDevices()
{
	if( !s_devices.empty() )
	{
		return;
	}
	uint32_t count = 0;
	while( true )
	{
		DISPLAY_DEVICEW device;
		device.cb = sizeof( device );
		if( !EnumDisplayDevicesW( nullptr, count, &device, 0 ) )
		{
			break;
		}
		if( ( device.StateFlags & DISPLAY_DEVICE_ACTIVE ) )
		{
			Tr2AdapterInfo info;
			GetDeviceInfo( device, info );
			if( ( device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE ) )
			{
				s_devices.insert( s_devices.begin(), std::make_pair( count, info ) );
			}
			else
			{
				s_devices.push_back( std::make_pair( count, info ) );
			}
		}
		++count;
	}
}

struct FindMonitorContext
{
	const wchar_t* device;
	HMONITOR monitor;
};

BOOL CALLBACK FindMonitor( HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData )
{
	MONITORINFOEXW info;
	info.cbSize = sizeof( info );
	if( !GetMonitorInfoW( hMonitor, &info ) )
	{
		return TRUE;
	}
	auto context = reinterpret_cast<FindMonitorContext*>( dwData );
	if( wcscmp( info.szDevice, context->device ) == 0 )
	{
		context->monitor = hMonitor;
		return FALSE;
	}
	return TRUE;
}

}

ALResult Tr2VideoAdapterInfo::GetAdapterCount( unsigned& count )
{
	GetVideoDevices();
	count = uint32_t( s_devices.size() );
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterInfo( unsigned adapterIndex,
											  Tr2AdapterInfo& info )
{
	GetVideoDevices();
	if( adapterIndex >= s_devices.size() )
	{
		return E_INVALIDARG;
	}
	info = s_devices[adapterIndex].second;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMonitor( unsigned adapterIndex,
												 void*& monitor )
{
	GetVideoDevices();
	if( adapterIndex >= s_devices.size() )
	{
		return E_INVALIDARG;
	}
	FindMonitorContext ctx;
	DISPLAY_DEVICEW device;
	device.cb = sizeof( device );
	if( !EnumDisplayDevicesW( nullptr, s_devices[adapterIndex].first, &device, 0 ) )
	{
		return E_FAIL;
	}
	ctx.device = device.DeviceName;
	ctx.monitor = nullptr;
	EnumDisplayMonitors( nullptr, nullptr, &FindMonitor, reinterpret_cast<LPARAM>( &ctx ) );
	if( !ctx.monitor )
	{
		return E_FAIL;
	}
	monitor = ctx.monitor;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterDisplayMode( unsigned adapterIndex,
													 Tr2DisplayModeInfo& mode )
{
	GetVideoDevices();
	if( adapterIndex >= s_devices.size() )
	{
		return E_INVALIDARG;
	}
	DISPLAY_DEVICEW device;
	device.cb = sizeof( device );
	if( !EnumDisplayDevicesW( nullptr, s_devices[adapterIndex].first, &device, 0 ) )
	{
		return E_FAIL;
	}

	DEVMODEW devmode;
	if( !EnumDisplaySettingsW( device.DeviceName, ENUM_CURRENT_SETTINGS, &devmode ) )
	{
		return E_FAIL;
	}
	mode.width = devmode.dmPelsWidth;
	mode.height = devmode.dmPelsHeight;
	mode.refreshRateNumerator = 1;
	mode.refreshRateDenominator = devmode.dmDisplayFrequency;
	mode.format = Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterModeCount( unsigned adapterIndex,
												   Tr2RenderContextEnum::PixelFormat backBufferFormat,
												   unsigned& count )
{
	if( backBufferFormat != Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM )
	{
		count = 0;
		return S_OK;
	}

	GetVideoDevices();
	if( adapterIndex >= s_devices.size() )
	{
		return E_INVALIDARG;
	}
	DISPLAY_DEVICEW device;
	device.cb = sizeof( device );
	if( !EnumDisplayDevicesW( nullptr, s_devices[adapterIndex].first, &device, 0 ) )
	{
		return E_FAIL;
	}

	count = 0;
	for( uint32_t i = 0; ; ++i )
	{
		DEVMODEW mode;
		if( !EnumDisplaySettingsW( device.DeviceName, i, &mode ) )
		{
			break;
		}
		if( mode.dmBitsPerPel == 32 )
		{
			++count;
		}
	}
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMode( unsigned adapterIndex,
											  Tr2RenderContextEnum::PixelFormat backBufferFormat,
											  unsigned modeIndex,
											  Tr2DisplayModeInfo& mode )
{
	if( backBufferFormat != Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM )
	{
		return E_INVALIDARG;
	}

	GetVideoDevices();
	if( adapterIndex >= s_devices.size() )
	{
		return E_INVALIDARG;
	}
	DISPLAY_DEVICEW device;
	device.cb = sizeof( device );
	if( !EnumDisplayDevicesW( nullptr, s_devices[adapterIndex].first, &device, 0 ) )
	{
		return E_FAIL;
	}

	for( uint32_t i = 0; ; ++i )
	{
		DEVMODEW devmode;
		if( !EnumDisplaySettingsW( device.DeviceName, i, &devmode ) )
		{
			break;
		}
		if( devmode.dmBitsPerPel == 32 )
		{
			if( modeIndex-- == 0 )
			{
				mode.width = devmode.dmPelsWidth;
				mode.height = devmode.dmPelsHeight;
				mode.refreshRateNumerator = 1;
				mode.refreshRateDenominator = devmode.dmDisplayFrequency;
				mode.format = Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM;
			}
		}
	}
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterShaderVersion( unsigned,
													   unsigned& version )
{
	version = 5 << 8;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMaxTextureWidth( unsigned,
														 unsigned& maxWidth )
{
	maxWidth = 4096;
	return S_OK;
}

bool Tr2VideoAdapterInfo::SupportsBackBufferFormat( unsigned adapterIndex,
													Tr2RenderContextEnum::PixelFormat backBufferFormat,
													bool )
{
	GetVideoDevices();
	if( adapterIndex >= s_devices.size() )
	{
		return false;
	}

	return backBufferFormat == Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM;
}

bool Tr2VideoAdapterInfo::SupportsRenderTargetFormat( unsigned adapterIndex,
													  Tr2RenderContextEnum::PixelFormat,
													  Tr2RenderContextEnum::PixelFormat,
													  bool )
{
	GetVideoDevices();
	if( adapterIndex >= s_devices.size() )
	{
		return false;
	}

	return true;
}

bool Tr2VideoAdapterInfo::SupportsDepthStencilFormat( unsigned adapterIndex,
													  Tr2RenderContextEnum::PixelFormat,
													  Tr2RenderContextEnum::DepthStencilFormat )
{
	GetVideoDevices();
	if( adapterIndex >= s_devices.size() )
	{
		return false;
	}

	return true;
}

bool Tr2VideoAdapterInfo::SupportsVertexTextureFormat( unsigned adapterIndex,
													   Tr2RenderContextEnum::PixelFormat,
													   Tr2RenderContextEnum::PixelFormat )
{
	GetVideoDevices();
	if( adapterIndex >= s_devices.size() )
	{
		return false;
	}

	return true;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMsaaSupport( unsigned adapterIndex,
													 Tr2RenderContextEnum::PixelFormat,
													 bool,
													 unsigned,
													 unsigned& msaaQuality )
{
	GetVideoDevices();
	if( adapterIndex >= s_devices.size() )
	{
		return E_INVALIDARG;
	}

	msaaQuality = 0;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMsaaSupport( unsigned adapterIndex,
													 Tr2RenderContextEnum::DepthStencilFormat,
													 bool,
													 unsigned,
													 unsigned& msaaQuality )
{
	GetVideoDevices();
	if( adapterIndex >= s_devices.size() )
	{
		return E_INVALIDARG;
	}

	msaaQuality = 0;
	return S_OK;
}

bool Tr2VideoAdapterInfo::AreAdaptersDifferent( unsigned adapter1,
												unsigned adapter2 )
{
	return adapter1 != adapter2;
}

ALResult Tr2VideoAdapterInfo::RefreshData()
{
	return S_OK;
}

#endif

#endif
