////////////////////////////////////////////////////////////
//
//    Created:   September 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "Tr2VideoAdapters.h"
#include "TriSettingsRegistrar.h"
#include "../trinityal/Tr2DriverUtilities.h"

#ifdef _WIN32
std::vector<HANDLE> g_D3DCreatedHeaps;
#endif


using namespace Tr2RenderContextEnum;

Tr2VideoAdapters::Tr2VideoAdapters()
	:DEFAULT_ADAPTER( Tr2VideoAdapterInfo::DEFAULT_ADAPTER ) 
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Query number of display adapters (video cards) available.
// Arguments:
//   count - (out) Number of adapters available
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapters::GetAdapterCount( unsigned& count )
{
	return Tr2VideoAdapterInfo::GetAdapterCount( count );
}

// --------------------------------------------------------------------------------------
// Description:
//   Get display adapter properties.
// Arguments:
//   adapterIndex - index of display adapter (from 0 to GetAdapterCount)
//   info - (out) Tr2VideoAdapter object with adapter properties
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapters::GetAdapterInfo( unsigned adapterIndex,
										   Tr2VideoAdapter** info )
{
	*info = nullptr;
	Tr2AdapterInfo adapterInfo;
	CR_RETURN_HR( Tr2VideoAdapterInfo::GetAdapterInfo( adapterIndex, adapterInfo ) );
	Tr2VideoAdapterPtr adapter;
	adapter.CreateInstance();
	adapter->m_index = adapterIndex;
	adapter->m_info = adapterInfo;
	*info = adapter.Detach();
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Get current display mode for given display adapter.
// Arguments:
//   adapterIndex - index of display adapter (from 0 to GetAdapterCount)
//   mode - (out) Tr2DisplayMode object with display mode properties
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapters::GetCurrentDisplayMode( unsigned adapterIndex,
												  Tr2DisplayMode** mode )
{
	*mode = nullptr;
	Tr2DisplayModeInfo info;
	CR_RETURN_HR( Tr2VideoAdapterInfo::GetAdapterDisplayMode( adapterIndex, info ) );
	Tr2DisplayModePtr result;
	result.CreateInstance();
	result->m_mode = info;
	*mode = result.Detach();
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Get number of available display modes supported by display adapter for the given 
//   back buffer format.
// Arguments:
//   adapterIndex - index of display adapter (from 0 to GetAdapterCount)
//   backBufferFormat - back buffer format
//   count - (out) number of display modes supported
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapters::GetDisplayModeCount( unsigned adapterIndex,
												int /*Tr2RenderContextEnum::PixelFormat*/ backBufferFormat,
												unsigned& count )
{
	return Tr2VideoAdapterInfo::GetAdapterModeCount( adapterIndex, PixelFormat( backBufferFormat ), count );
}

// --------------------------------------------------------------------------------------
// Description:
//   Get display mode properties.
// Arguments:
//   adapterIndex - index of display adapter (from 0 to GetAdapterCount)
//   backBufferFormat - back buffer format
//   modeIndex - display mode index (from 0 to GetDisplayModeCount)
//   mode - (out) Tr2DisplayMode object with display mode properties
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapters::GetDisplayMode( unsigned adapterIndex,
										   int /*Tr2RenderContextEnum::PixelFormat*/ backBufferFormat,
										   unsigned modeIndex,
										   Tr2DisplayMode** mode )
{
	*mode = nullptr;
	Tr2DisplayModeInfo info;
	CR_RETURN_HR( Tr2VideoAdapterInfo::GetAdapterMode( adapterIndex, PixelFormat( backBufferFormat ), modeIndex, info ) )
	;
	Tr2DisplayModePtr result;
	result.CreateInstance();
	result->m_mode = info;
	*mode = result.Detach();
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Query if display adapter supports given back buffer format.
// Arguments:
//   adapterIndex - index of display adapter (from 0 to GetAdapterCount)
//   backBufferFormat - back buffer format
//   windowed - if the application runs in windowed or fullscreen mode
// Return Value:
//   true if adapter supports given back buffer format
//   false otherwise
// --------------------------------------------------------------------------------------
bool Tr2VideoAdapters::SupportsBackBufferFormat( unsigned adapterIndex,
												 int /*Tr2RenderContextEnum::PixelFormat*/ backBufferFormat )
{
	return Tr2VideoAdapterInfo::SupportsBackBufferFormat( adapterIndex, PixelFormat( backBufferFormat ) );
}

// --------------------------------------------------------------------------------------
// Description:
//   Query if display adapter supports given render target format.
// Arguments:
//   adapterIndex - index of display adapter (from 0 to GetAdapterCount)
//   backBufferFormat - back buffer format
//   format - render target format
//   windowed - if the application runs in windowed or fullscreen mode
// Return Value:
//   true if adapter supports given render target format
//   false otherwise
// --------------------------------------------------------------------------------------
bool Tr2VideoAdapters::SupportsRenderTargetFormat( unsigned adapterIndex,
												   int /*Tr2RenderContextEnum::PixelFormat*/ format )
{
	return Tr2VideoAdapterInfo::SupportsRenderTargetFormat( adapterIndex, PixelFormat( format ) );
}

// --------------------------------------------------------------------------------------
// Description:
//   Query maximum texture size (with or height) in pixels.
// Arguments:
//   adapterIndex - index of display adapter (from 0 to GetAdapterCount)
//   maxWidth - (out) maximum texture size
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapters::GetMaxTextureSize( unsigned adapterIndex,
											  unsigned& maxWidth )
{
	return Tr2VideoAdapterInfo::GetAdapterMaxTextureWidth( adapterIndex, maxWidth );
}

// --------------------------------------------------------------------------------------
// Description:
//   Refreshes any cached information about video adapters.
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapters::RefreshData()
{
	return Tr2VideoAdapterInfo::RefreshData();
}

// --------------------------------------------------------------------------------------
// Description:
//   Helper function to return device identifier GUID as a string.
// Return Value:
//   Device identifier GUID as a string
// --------------------------------------------------------------------------------------
std::string Tr2VideoAdapter::GetDeviceIdentifierString() const
{
#ifdef _WIN32
	LPOLESTR guidstr;
	if ( SUCCEEDED( StringFromCLSID( *reinterpret_cast<const GUID*>( &m_info.deviceIdentifier ), &guidstr ) ) )
	{
		CW2A str( guidstr );
		CoTaskMemFree( guidstr );
		return ( const char* )str;
	}
#else
    char buffer[128];
    snprintf(
            buffer,
             128,
            "%08x-%04x-%04x-%02xhh%02xhh-%02xhh%02xhh%02xhh%02xhh%02xhh%02xhh",
            unsigned( m_info.deviceIdentifier.data1 ),
            unsigned( m_info.deviceIdentifier.data2 ),
            unsigned( m_info.deviceIdentifier.data3 ),
            m_info.deviceIdentifier.data4[0],
            m_info.deviceIdentifier.data4[1],
            m_info.deviceIdentifier.data4[2],
            m_info.deviceIdentifier.data4[3],
            m_info.deviceIdentifier.data4[4],
            m_info.deviceIdentifier.data4[5],
            m_info.deviceIdentifier.data4[6],
            m_info.deviceIdentifier.data4[7] );
    return buffer;
#endif
	return "";
}

// --------------------------------------------------------------------------------------
// Description:
//   Get driver information for the display adapter.
// Arguments:
//   info - (out) Tr2VideoDriver object with video driver properties
// Return Value:
//   ALResult code of operation
// --------------------------------------------------------------------------------------
ALResult Tr2VideoAdapter::GetDriverInfo( Tr2VideoDriver** info )
{
	*info = nullptr;
	
	Tr2VideoDriverInfo driverInfo;
	FORWARD_HR( Tr2DriverUtilities::GetDriverVersion( m_info.deviceID, driverInfo ) );

	Tr2VideoDriverPtr result;
	result.CreateInstance();
	result->m_info = driverInfo;
	*info = result.Detach();
	return S_OK;
}

