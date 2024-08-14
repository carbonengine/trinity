////////////////////////////////////////////////////////////
//
//    Created:   September 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "Tr2VideoAdapters.h"


BLUE_DEFINE( Tr2VideoAdapters );
BLUE_DEFINE( Tr2VideoAdapter );
BLUE_DEFINE( Tr2DisplayMode );
BLUE_DEFINE( Tr2VideoDriver );


const Be::ClassInfo* Tr2VideoAdapter::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2VideoAdapter, "" )
		MAP_INTERFACE( Tr2VideoAdapter )
		
		MAP_ATTRIBUTE( "index", m_index, "Video adapter index", Be::READ )
		MAP_ATTRIBUTE( "driver", m_info.driver, "Video driver names", Be::READ )
		MAP_ATTRIBUTE( "description", m_info.description, "Human-readable adapter name", Be::READ )
		MAP_ATTRIBUTE( "deviceName", m_info.deviceName, "Adapter device name", Be::READ )
		MAP_ATTRIBUTE( "driverVersion", m_info.driverVersion, "Video driver version", Be::READ )
		MAP_ATTRIBUTE( "vendorID", m_info.vendorID, "Video adapter vendor ID", Be::READ )
		MAP_ATTRIBUTE( "deviceID", m_info.deviceID, "Video adapter device ID", Be::READ )
		MAP_ATTRIBUTE( "subSystemID", m_info.subSystemID, "Video adapter sub-system ID", Be::READ )
		MAP_ATTRIBUTE( "revision", m_info.subSystemID, "", Be::READ )
		MAP_PROPERTY_READONLY( "deviceIdentifier", GetDeviceIdentifierString, "Device identifier GUID as a string" )
		MAP_METHOD_AND_WRAP( "GetDriverInfo", GetDriverInfo, "Driver information for this adapter" )
	EXPOSURE_END()
}

const Be::ClassInfo* Tr2DisplayMode::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2DisplayMode, "" )
		MAP_INTERFACE( Tr2DisplayMode )
		
		MAP_ATTRIBUTE( "width", m_mode.width, "Back buffer width", Be::READ )
		MAP_ATTRIBUTE( "height", m_mode.height, "Back buffer height", Be::READ )
		MAP_ATTRIBUTE( "refreshRateNumerator", m_mode.refreshRateNumerator, "Refresh rate numenator", Be::READ )
		MAP_ATTRIBUTE( "refreshRateDenominator", m_mode.refreshRateDenominator, "Refresh rate denumenator", Be::READ )
		MAP_ATTRIBUTE_WITH_CHOOSER( "format", m_mode.format, "Back buffer format", Be::READ | Be::ENUM, Tr2RenderContextEnum_PixelFormat_Chooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "scanlineOrdering", m_mode.scanlineOrdering, "Scanline ordering", Be::READ, Tr2RenderContextEnum_ScanlineOrdering_Chooser )
		MAP_ATTRIBUTE_WITH_CHOOSER( "scaling", m_mode.scaling, "Video scaling mode", Be::READ, Tr2RenderContextEnum_DisplayScaling_Chooser )
	EXPOSURE_END()
}

const Be::ClassInfo* Tr2VideoDriver::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2VideoDriver, "" )
		MAP_INTERFACE( Tr2VideoDriver )
		
		MAP_ATTRIBUTE( "driverVersion", m_info.driverVersion, "Driver version as a 64 bit number", Be::READ )
		MAP_ATTRIBUTE( "driverVersionString", m_info.driverVersionString, "Driver version as a string", Be::READ )
		MAP_ATTRIBUTE( "driverVendor", m_info.driverVendor, "Driver vendor company", Be::READ )
		MAP_ATTRIBUTE( "driverDate", m_info.driverDate, "Driver release date", Be::READ )
		MAP_ATTRIBUTE( "isOptimus", m_info.isOptimus, "Is the machine running Optimus (Intel-nVidia mix)", Be::READ )
		MAP_ATTRIBUTE( "isAmdDynamicSwitchable", m_info.isAmdDynamicSwitchable, "Is the machine running AMD Dynamic Switchable (Intel-AMD mix)", Be::READ )
	EXPOSURE_END()
}

const Be::ClassInfo* Tr2VideoAdapters::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2VideoAdapters, "" )
		MAP_INTERFACE( Tr2VideoAdapters )

		MAP_ATTRIBUTE( "DEFAULT_ADAPTER", DEFAULT_ADAPTER, "Index of default video adapter", Be::READ )
		
		MAP_METHOD_AND_WRAP( 
			"GetAdapterCount", 
			GetAdapterCount, 
			"Returns number of video adapters in the system" )
		MAP_METHOD_AND_WRAP( 
			"GetAdapterInfo", 
			GetAdapterInfo, 
			"Returns video adapter information (as a Tr2VideoAdapter)."
			"\n"
			"\n:param idx: Video adapter index" )
		MAP_METHOD_AND_WRAP( 
			"GetCurrentDisplayMode", 
			GetCurrentDisplayMode, 
			"Returns current display mode information (as a Tr2DisplayMode) for video adapter."
			"\n"
			"\n:param idx: Video adapter index" )
		MAP_METHOD_AND_WRAP( 
			"GetDisplayModeCount", 
			GetDisplayModeCount, 
			"Returns number of supported display modes for video adapter and given back buffer format."
			"\n"
			"\n:param idx: Video adapter index"
			"\n:param format: Back buffer format (member of trinity.PIXEL_FORMAT)" )
		MAP_METHOD_AND_WRAP( 
			"GetDisplayMode", 
			GetDisplayMode, 
			"Returns display mode information for video adapter and given back buffer format."
			"\n"
			"\n:param idx: Video adapter index"
			"\n:param format: Back buffer format (member of trinity.PIXEL_FORMAT)"
			"\n:param modeIndex: Display mode index" )

		MAP_METHOD_AND_WRAP( 
			"SupportsBackBufferFormat", 
			SupportsBackBufferFormat, 
			"Returns if the video adapter supports given back buffer format when running"
			"\nin fullscreen or windowed mode."
			"\n"
			"\n:param idx: Video adapter index"
			"\n:param backBufferFormat: Back buffer format (member of trinity.PIXEL_FORMAT)" )
		MAP_METHOD_AND_WRAP( 
			"SupportsRenderTargetFormat", 
			SupportsRenderTargetFormat, 
			"Returns if the video adapter supports given render target format when running"
			"\nwith the given back buffer format."
			"\n"
			"\n:param idx: Video adapter index"
			"\n:param format: Render target format (member of trinity.PIXEL_FORMAT)" )
		MAP_METHOD_AND_WRAP( 
			"GetMaxTextureSize", 
			GetMaxTextureSize, 
			"Returns maximum texture size (width or height) supported by video adapter."
			"\n"
			"\n:param idx: Video adapter index" )
		MAP_METHOD_AND_WRAP( 
			"Refresh", 
			RefreshData, 
			"Refreshes adapter information." )
	EXPOSURE_END()
}
