#include "StdAfx.h"

#if( TRINITY_PLATFORM == TRINITY_METAL )

#include "Tr2VideoAdapterInfoALMetal.h"
#include "Tr2AdapterStructures.h"
#import <CoreGraphics/CoreGraphics.h>
#import <IOKit/graphics/IOGraphicsLib.h>
#import <Metal/Metal.h>


using namespace Tr2RenderContextEnum;

namespace
{
	struct Display
	{
		CGDirectDisplayID displayID;
		std::string name;
		std::wstring description;
		uint32_t vendorID;
		uint32_t deviceID;
		uint32_t msaaSamples;
        
        std::vector<Tr2DisplayModeInfo> modes;
	};
	std::vector<Display> s_displays;

	void GetDisplayModes( CGDirectDisplayID display, std::vector<Tr2DisplayModeInfo>& modes )
	{
		const CFStringRef keys[] = { kCGDisplayShowDuplicateLowResolutionModes };
		const CFBooleanRef values[] = { kCFBooleanTrue };
		auto dict = CFDictionaryCreate(
									   nullptr,
									   (const void**)keys,
									   (const void**)values,
									   1,
									   &kCFCopyStringDictionaryKeyCallBacks,
									   &kCFTypeDictionaryValueCallBacks );
		
		auto allModes = CGDisplayCopyAllDisplayModes( display, dict );
		CFRelease( dict );

		auto count = CFArrayGetCount( allModes );
		
		for( CFIndex i = 0; i < count; ++i )
		{
			auto modeRef = (CGDisplayModeRef)CFArrayGetValueAtIndex( allModes, i );

			Tr2DisplayModeInfo mode;
			mode.format = PIXEL_FORMAT_B8G8R8X8_UNORM;
			mode.width  = (uint32_t)CGDisplayModeGetPixelWidth( modeRef );
			mode.height = (uint32_t)CGDisplayModeGetPixelHeight( modeRef );
			mode.refreshRateDenominator = 1;
			mode.refreshRateNumerator = 1;
			mode.scaling = DISPLAY_SCALING_UNSPECIFIED;
			mode.scanlineOrdering = SCANLINE_ORDER_UNSPECIFIED;

			auto found = std::find_if( begin( modes ), end( modes ), [&mode]( auto& other ) {
				return mode.width == other.width && mode.height == other.height;
			} );
			if( found == end( modes ) )
			{
				modes.push_back( mode );
			}
		}
		
		std::sort( begin( modes ), end( modes ), []( auto& m1, auto& m2 ) {
			if( m1.width == m2.width )
			{
				return m1.height > m2.height;
			}
			return m1.width > m2.width;
		} );
		CFRelease( allModes );
	}
	
	uint32_t GetEntryProperty( io_registry_entry_t entry, CFStringRef propertyName )
	{
		uint32_t value = 0;
		CFTypeRef property = IORegistryEntrySearchCFProperty( entry,
															kIOServicePlane,
															propertyName,
															kCFAllocatorDefault,
															kIORegistryIterateRecursively | kIORegistryIterateParents );
		if( property )
		{
			if( auto data = reinterpret_cast<const uint32_t*>( CFDataGetBytePtr( (CFDataRef)property ) ) )
			{
				value = *data;
			}
			CFRelease( property );
		}
		return value;
	}

	std::string ToString( NSString* string )
	{
		NSData* data = [string dataUsingEncoding:NSASCIIStringEncoding allowLossyConversion:YES];
		int32_t length = int32_t( [data length] );
		return std::string( reinterpret_cast<const char*>( [data bytes] ), length );
	}

	std::wstring ToWString( NSString* string )
	{
		NSData* data = [string dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
		int32_t length = int32_t( [data length] ) / sizeof( wchar_t );
		return std::wstring( reinterpret_cast<const wchar_t*>( [data bytes] ), length );
	}

	io_service_t IOServicePortFromCGDisplayID( CGDirectDisplayID displayID )
	{
		io_iterator_t iter;
		io_service_t serv, servicePort = 0;

		CFMutableDictionaryRef matching = IOServiceMatching( "IODisplayConnect" );
		if( IOServiceGetMatchingServices( kIOMasterPortDefault, matching, &iter ) )
		{
			return 0;
		}

		auto displayVendorID = CGDisplayVendorNumber( displayID );
		auto displayModelID = CGDisplayModelNumber( displayID );
		auto displaySerial = CGDisplaySerialNumber( displayID );

		while( ( serv = IOIteratorNext( iter ) ) != 0 )
		{
			CFIndex vendorID, productID, serialNumber;
			Boolean success;

			auto info = IODisplayCreateInfoDictionary( serv, kIODisplayOnlyPreferredName );
			ON_BLOCK_EXIT( [&] { CFRelease( info ); } );

			auto vendorIDRef = static_cast<CFNumberRef>( CFDictionaryGetValue( info, CFSTR( kDisplayVendorID ) ) );
			auto productIDRef = static_cast<CFNumberRef>( CFDictionaryGetValue( info, CFSTR( kDisplayProductID ) ) );
			// Serial number no longer supported
			//auto serialNumberRef = static_cast<CFNumberRef>( CFDictionaryGetValue( info, CFSTR( kDisplaySerialNumber ) ) );

			success  = CFNumberGetValue( vendorIDRef, kCFNumberCFIndexType, &vendorID );
			success &= CFNumberGetValue( productIDRef, kCFNumberCFIndexType, &productID );

			if( !success )
			{
				continue;
			}

			if( displayVendorID != (uint32_t)vendorID || displayModelID != (uint32_t)productID )
			{
				continue;
			}

			servicePort = serv;
			break;
		}

		IOObjectRelease( iter );
		return servicePort;
	}

	void RefreshDisplays()
	{
		s_displays.clear();

		id<MTLDevice> device = MTLCreateSystemDefaultDevice();
		if( !device )
		{
			return;
		}
		if( @available(macOS 10.15, *) )
		{
			if( ![device supportsFamily:MTLGPUFamilyMac1] )
			{
				return;
			}
		}

		uint32_t msaaSamples = 1;
		for( uint32_t i = 2; i < 32; ++i )
		{
			if( [device supportsTextureSampleCount:i] )
			{
				msaaSamples |= 1 << i;
			}
		}
		std::wstring deviceDescription = ToWString( [device name] );
		
		uint32_t vendorID = 0;
		uint32_t deviceID = 0;
		if( uint64_t regID = [device respondsToSelector: @selector(registryID)] ? device.registryID : 0 )
		{
			if( io_registry_entry_t entry = IOServiceGetMatchingService( kIOMasterPortDefault, IORegistryEntryIDMatching( regID ) ) )
			{
				io_registry_entry_t parent;
				if( IORegistryEntryGetParentEntry( entry, kIOServicePlane, &parent ) == kIOReturnSuccess )
				{
					vendorID = GetEntryProperty( parent, CFSTR( "vendor-id" ) );
					deviceID = GetEntryProperty( parent, CFSTR( "device-id" ) );
					IOObjectRelease( parent );
				}
				IOObjectRelease( entry );
			}
		}


		uint32_t displayCount;
		CGGetOnlineDisplayList(0, NULL, &displayCount);
		std::unique_ptr<CGDirectDisplayID[]> displays( new CGDirectDisplayID[displayCount] );
		CGGetOnlineDisplayList( displayCount, displays.get(), &displayCount );
		for( uint32_t i = 0; i < displayCount; ++i )
		{
			if( !CGDisplayIsActive( displays[i] ) )
			{
				continue;
			}

			Display display = { displays[i] };

			if( auto port = IOServicePortFromCGDisplayID( display.displayID ) )
			{
				auto info = IODisplayCreateInfoDictionary( port, kIODisplayOnlyPreferredName );
				if( auto names = static_cast<CFDictionaryRef>( CFDictionaryGetValue( info, CFSTR( kDisplayProductName ) ) ) )
				{
					CFStringRef value;
					if( CFDictionaryGetValueIfPresent( names, CFSTR( "en_US" ), (const void**)&value ) )
					{
						auto size = CFStringGetMaximumSizeForEncoding( CFStringGetLength(value), kCFStringEncodingWindowsLatin1 );
						std::unique_ptr<char[]> name( new char[size + 1] );
						CFStringGetCString( value, name.get(), size + 1, kCFStringEncodingWindowsLatin1 );
						display.name = name.get();
					}
				}
				CFRelease(info);
			}

			display.msaaSamples = msaaSamples;
			display.description = deviceDescription;
			display.vendorID = vendorID;
			display.deviceID = deviceID;
            
            GetDisplayModes( displays[i], display.modes );

			if( CGDisplayIsMain( displays[i] ) )
			{
				s_displays.insert( s_displays.begin(), display );
			}
			else
			{
				s_displays.push_back( display );
			}
		}
	}
}

ALResult Tr2VideoAdapterInfo::GetAdapterCount( unsigned& count )
{
	if( s_displays.empty() )
	{
		RefreshDisplays();
	}
	count = unsigned( s_displays.size() );
	return S_OK;
}

#define CHECK_ADAPTER                           \
    if( s_displays.empty() )                    \
    {                                           \
        RefreshDisplays();                      \
    }                                           \
    if( adapterIndex >= s_displays.size() )     \
    {                                           \
        return E_INVALIDARG;                    \
    }

ALResult Tr2VideoAdapterInfo::GetAdapterInfo( unsigned adapterIndex,
											  Tr2AdapterInfo& info )
{
	CHECK_ADAPTER;

	auto& display = s_displays[adapterIndex];
	id<MTLDevice> device = CGDirectDisplayCopyCurrentMetalDevice( display.displayID );

	info.driver = "";
	info.driverVersion = 0;
	info.deviceName = display.name;
	info.description = display.description;
	info.vendorID = display.vendorID;
	info.deviceID = display.deviceID;
	info.subSystemID = 0;
	info.revision = 0;
	AdapterGuid giud;
	giud.data1 = 0;
	giud.data2 = 0;
	giud.data3 = 0;
	for( int i = 0; i < 8; i++)
	{
		giud.data4[i] = 0;
	}
	info.deviceIdentifier = giud;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMonitor( unsigned adapterIndex,
												 void*& monitor )
{
	CHECK_ADAPTER;

	monitor = (void*)uintptr_t( s_displays[adapterIndex].displayID );
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterDisplayMode( unsigned adapterIndex,
													 Tr2DisplayModeInfo& mode )
{
	CHECK_ADAPTER;

	size_t w = 0;
	size_t h = 0;

	// Find native display mode (if available).
	bool nativeModeFound = false;
	CFArrayRef modes = CGDisplayCopyAllDisplayModes( s_displays[adapterIndex].displayID, NULL );

	for( CFIndex i = 0, n = CFArrayGetCount( modes ); i < n; ++i )
	{
		CGDisplayModeRef currentMode = (CGDisplayModeRef) CFArrayGetValueAtIndex( modes, i );
		uint32_t ioFlags = CGDisplayModeGetIOFlags( currentMode );

		if( ioFlags & kDisplayModeNativeFlag )
		{
			w = CGDisplayModeGetPixelWidth( currentMode );
			h = CGDisplayModeGetPixelHeight( currentMode );

			nativeModeFound = true;
			break;
		}
	}
	CFRelease( modes );

	// If native display mode is not available - use current (default) mode.
	if( !nativeModeFound )
	{
		CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode( s_displays[adapterIndex].displayID );
		if( !currentMode )
		{
			return E_FAIL;
		}

		w = CGDisplayModeGetPixelWidth( currentMode );
		h = CGDisplayModeGetPixelHeight( currentMode );

		CGDisplayModeRelease( currentMode );
	}

	mode.format = PIXEL_FORMAT_B8G8R8A8_UNORM;
	mode.width  = (uint32_t) w;
	mode.height = (uint32_t) h;
	mode.refreshRateDenominator = 1;
	mode.refreshRateNumerator = 1;
	mode.scaling = DISPLAY_SCALING_UNSPECIFIED;
	mode.scanlineOrdering = SCANLINE_ORDER_UNSPECIFIED;

	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterModeCount( unsigned adapterIndex,
												   Tr2RenderContextEnum::PixelFormat,
												   unsigned& count )
{
	CHECK_ADAPTER;

    count = unsigned( s_displays[adapterIndex].modes.size() );
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMode( unsigned adapterIndex,
											  Tr2RenderContextEnum::PixelFormat,
											  unsigned modeIndex,
											  Tr2DisplayModeInfo& mode )
{
	CHECK_ADAPTER;
    
    auto& modes = s_displays[adapterIndex].modes;

	if( modeIndex >= modes.size() )
	{
		return E_INVALIDARG;
	}
    
    mode = modes[modeIndex];
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterShaderVersion( unsigned adapterIndex,
													   unsigned& version )
{
	CHECK_ADAPTER;

	version = 4294902528;
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMaxTextureWidth( unsigned adapterIndex,
														 unsigned& maxWidth )
{
	CHECK_ADAPTER;

	maxWidth = 16384;
	return S_OK;
}

bool Tr2VideoAdapterInfo::SupportsBackBufferFormat( unsigned adapterIndex,
													Tr2RenderContextEnum::PixelFormat backBufferFormat,
													bool )
{
	CHECK_ADAPTER;

	return backBufferFormat == PIXEL_FORMAT_B8G8R8A8_UNORM;
}

bool Tr2VideoAdapterInfo::SupportsRenderTargetFormat( unsigned adapterIndex,
													  Tr2RenderContextEnum::PixelFormat,
													  Tr2RenderContextEnum::PixelFormat format,
													  bool withAutoGenMipmap )
{
	CHECK_ADAPTER;
	switch( format )
	{
	case Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R8_SNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R8_UINT:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R8_SINT:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R16_UNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R16_SNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R16_SINT:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R16_FLOAT:

	case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_UNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_SNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_UINT:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_SINT:

	case Tr2RenderContextEnum::PIXEL_FORMAT_R32_UINT:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R32_SINT:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT:

	case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_UNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_SNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_UINT:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_SINT:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_FLOAT:

	case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_UNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_SNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_UINT:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_SINT:

	case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB:

	case Tr2RenderContextEnum::PIXEL_FORMAT_R10G10B10A2_UNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R10G10B10A2_UINT:

	case Tr2RenderContextEnum::PIXEL_FORMAT_R11G11B10_FLOAT:

	case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32_SINT:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32_UINT:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32_FLOAT:

	case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_SINT:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_UINT:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_FLOAT:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_UNORM:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_SNORM:

	case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_SINT:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_UINT:
	case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT:
		return true;
	default:
		return false;
	}
	return true;
}

bool Tr2VideoAdapterInfo::SupportsDepthStencilFormat( unsigned adapterIndex,
													  Tr2RenderContextEnum::PixelFormat,
													  Tr2RenderContextEnum::DepthStencilFormat format )
{
	CHECK_ADAPTER;

	return true;
}

bool Tr2VideoAdapterInfo::SupportsVertexTextureFormat( unsigned adapterIndex,
													   Tr2RenderContextEnum::PixelFormat backBufferFormat,
													   Tr2RenderContextEnum::PixelFormat format )
{
	CHECK_ADAPTER;

	return true;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMsaaSupport( unsigned adapterIndex,
													 Tr2RenderContextEnum::PixelFormat,
													 bool,
													 unsigned msaaType,
													 unsigned& msaaQuality )
{
	CHECK_ADAPTER;

	if( msaaType >= 32 )
	{
		msaaQuality = 0;
		return S_OK;
	}
	if( ( s_displays[adapterIndex].msaaSamples & (1 << msaaType ) ) != 0 )
	{
		msaaQuality = 1;
	}
	else
	{
		msaaQuality = 0;
	}
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMsaaSupport( unsigned adapterIndex,
													 Tr2RenderContextEnum::DepthStencilFormat format,
													 bool windowed,
													 unsigned msaaType,
													 unsigned& msaaQuality )
{
	CHECK_ADAPTER;

	if( msaaType >= 32 )
	{
		msaaQuality = 0;
		return S_OK;
	}
	if( ( s_displays[adapterIndex].msaaSamples & (1 << msaaType ) ) != 0 )
	{
		msaaQuality = 1;
	}
	else
	{
		msaaQuality = 0;
	}
	return S_OK;
}

bool Tr2VideoAdapterInfo::AreAdaptersDifferent( unsigned adapter1,
												unsigned adapter2 )
{
	if ( adapter1 == adapter2 )
	{
		return false;
	}
	if ( adapter1 >= s_displays.size() || adapter2 >= s_displays.size() )
	{
		return true;
	}

	id<MTLDevice> device1 = CGDirectDisplayCopyCurrentMetalDevice( s_displays[adapter1].displayID );
	id<MTLDevice> device2 = CGDirectDisplayCopyCurrentMetalDevice( s_displays[adapter2].displayID );

	return device1 != device2;
}

ALResult Tr2VideoAdapterInfo::RefreshData()
{
	RefreshDisplays();
	return S_OK;
}

#endif
