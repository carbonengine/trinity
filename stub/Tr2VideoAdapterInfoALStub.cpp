#include "StdAfx.h"

bool g_wantsEXDevice = false;

#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "Tr2VideoAdapterInfoALStub.h"

#ifdef WIN32
extern bool g_usingEXDevice;
extern std::vector<HANDLE> g_D3DCreatedHeaps;
#endif

using namespace Tr2RenderContextEnum;

ALResult Tr2VideoAdapterInfo::GetAdapterCount( unsigned& count )
{
	count = 1;
	return S_OK;
}

bool GetDeviceId( uint32_t& deviceId )
{
#ifdef WIN32
	DISPLAY_DEVICE dd;
	dd.cb = sizeof( DISPLAY_DEVICE );

	EnumDisplayDevices( nullptr, 0, &dd, 0 );

	const char* deviceIdPrefix = "DEV_";

	auto found = strstr( dd.DeviceID, deviceIdPrefix );
	if( !found )
	{
		return false;
	}
	return sscanf_s( found + strlen( deviceIdPrefix ), "%x", &deviceId ) == 1;
#else
    deviceId = 0;
    return true;
#endif
}

ALResult Tr2VideoAdapterInfo::GetAdapterInfo( unsigned adapterIndex,
											  Tr2AdapterInfo& info )
{
	
	
	
	info.driver = "stub";
	info.description = L"Not an actual adapter.";
	info.deviceName = "stub";
	info.driverVersion = 2533352100662421;
	info.vendorID = 0;
	GetDeviceId(info.deviceID);
	info.subSystemID = 0;
	info.revision = 163;
	AdapterGuid id;
	id.data1 = 0;
	id.data2 = 0;
	id.data3 = 0;
	for( int i = 0; i < 8; i++)
	{
		id.data4[i] = 0;
	}
	info.deviceIdentifier = id;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMonitor( unsigned adapterIndex,
												 void*& monitor )
{
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterDisplayMode( unsigned adapterIndex,
													 Tr2DisplayModeInfo& mode )
{
	mode.format = PIXEL_FORMAT_B8G8R8A8_UNORM;
	mode.width = 800;
	mode.height = 600;
	mode.refreshRateDenominator = 1;
	mode.refreshRateNumerator = 1;
	mode.scaling = DISPLAY_SCALING_UNSPECIFIED;
	mode.scanlineOrdering = SCANLINE_ORDER_UNSPECIFIED;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterModeCount( unsigned adapterIndex,
												   Tr2RenderContextEnum::PixelFormat backBufferFormat,
												   unsigned& count )
{
	count = 1;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMode( unsigned adapterIndex,
											  Tr2RenderContextEnum::PixelFormat backBufferFormat,
											  unsigned modeIndex,
											  Tr2DisplayModeInfo& mode )
{
	mode.format = PIXEL_FORMAT_B8G8R8X8_UNORM;
	mode.height = 1200;
	mode.refreshRateDenominator = 59;
	mode.refreshRateNumerator = 1;
	mode.scaling = DISPLAY_SCALING_UNSPECIFIED;
	mode.scanlineOrdering = SCANLINE_ORDER_UNSPECIFIED;
	mode.width = 1920;
	
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterShaderVersion( unsigned adapterIndex,
													   unsigned& version )
{
	version = 4294902528;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMaxTextureWidth( unsigned adapterIndex,
														 unsigned& maxWidth )
{
	maxWidth = 16384;
	return S_OK;
}

bool Tr2VideoAdapterInfo::SupportsBackBufferFormat( unsigned adapterIndex,
													Tr2RenderContextEnum::PixelFormat backBufferFormat,
													bool windowed )
{
	return true;
}

bool Tr2VideoAdapterInfo::SupportsRenderTargetFormat( unsigned adapterIndex,
													  Tr2RenderContextEnum::PixelFormat backBufferFormat,
													  Tr2RenderContextEnum::PixelFormat format,
													  bool withAutoGenMipmap )
{
	return true;
}

bool Tr2VideoAdapterInfo::SupportsDepthStencilFormat( unsigned adapterIndex,
													  Tr2RenderContextEnum::PixelFormat backBufferFormat,
													  Tr2RenderContextEnum::DepthStencilFormat format )
{
	return true;
}

bool Tr2VideoAdapterInfo::SupportsVertexTextureFormat( unsigned adapterIndex,
													   Tr2RenderContextEnum::PixelFormat backBufferFormat,
													   Tr2RenderContextEnum::PixelFormat format )
{
	return true;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMsaaSupport( unsigned adapterIndex,
													 Tr2RenderContextEnum::PixelFormat format,
													 bool windowed,
													 unsigned msaaType,
													 unsigned& msaaQuality )
{
	msaaQuality = 0;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMsaaSupport( unsigned adapterIndex,
													 Tr2RenderContextEnum::DepthStencilFormat format,
													 bool windowed,
													 unsigned msaaType,
													 unsigned& msaaQuality )
{
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
