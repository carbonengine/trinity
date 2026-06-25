// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if ( TRINITY_PLATFORM == TRINITY_STUB )

#include "Tr2VideoAdapterInfoALStub.h"
#include "Tr2AdapterStructures.h"

using namespace Tr2RenderContextEnum;

ALResult Tr2VideoAdapterInfo::GetAdapterCount( unsigned& count )
{
	count = 1;
	return S_OK;
}

bool GetDeviceId( uint32_t& deviceId )
{
	deviceId = 0;
	return true;
}

ALResult Tr2VideoAdapterInfo::GetAdapterInfo( unsigned,
											  Tr2AdapterInfo& info )
{



	info.driver = "stub";
	info.description = L"Not an actual adapter.";
	info.deviceName = "stub";
	info.driverVersion = 2533352100662421;
	info.vendorID = 0;
	GetDeviceId( info.deviceID );
	info.subSystemID = 0;
	info.revision = 163;
	AdapterGuid id;
	id.data1 = 0;
	id.data2 = 0;
	id.data3 = 0;
	for( int i = 0; i < 8; i++ )
	{
		id.data4[i] = 0;
	}
	info.deviceIdentifier = id;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMonitor( unsigned,
												 void*& )
{
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterDisplayMode( unsigned,
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

ALResult Tr2VideoAdapterInfo::GetAdapterModeCount( unsigned,
												   Tr2RenderContextEnum::PixelFormat,
												   unsigned& count )
{
	count = 1;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMode( unsigned,
											  Tr2RenderContextEnum::PixelFormat,
											  unsigned,
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

ALResult Tr2VideoAdapterInfo::GetAdapterMaxTextureWidth( unsigned,
														 unsigned& maxWidth )
{
	maxWidth = 16384;
	return S_OK;
}

bool Tr2VideoAdapterInfo::SupportsBackBufferFormat( unsigned,
													Tr2RenderContextEnum::PixelFormat )
{
	return true;
}

bool Tr2VideoAdapterInfo::SupportsRenderTargetFormat( unsigned, Tr2RenderContextEnum::PixelFormat )
{
	return true;
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
