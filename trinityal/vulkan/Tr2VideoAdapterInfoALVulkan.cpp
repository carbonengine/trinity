#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "Tr2VideoAdapterInfoALVulkan.h"
#include "Tr2AdapterStructures.h"
#include "VkResult.h"
#include "ALLog.h"
#include "UtilitiesVulkan.h"

#include <dxgi.h>

extern bool g_requestDeviceDebugLayer;

namespace
{
	static const char* s_requiredInstanceExtensions[] = {
		VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XCB_KHR)
		VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
		VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#endif
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
	};
	static const char* s_requiredDeviceExtensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	VkInstance s_instance = nullptr;

	template <size_t Size>
	const char* CheckExtensions( const std::vector<VkExtensionProperties>& available, const char* ( &required )[Size] )
	{
		for( auto name = std::begin( required ); name != std::end( required ); ++name )
		{
			bool found = false;
			for( auto ext = begin( available ); ext != end( available ); ++ext )
			{
				if( strcmp( *name, ext->extensionName ) == 0 )
				{
					found = true;
					break;
				}
			}
			if( !found )
			{
				return *name;
			}
		}
		return nullptr;
	}

	const char* s_validationLayers[] = {
		"VK_LAYER_KHRONOS_validation",
		"VK_LAYER_LUNARG_core_validation",
		"VK_LAYER_LUNARG_standard_validation",
		"VK_LAYER_LUNARG_parameter_validation",
	};

	void GetValidationLayers( std::vector<const char*>& validationLayers )
	{
		validationLayers.clear();

		if( !g_requestDeviceDebugLayer )
		{
			return;
		}

		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

		std::vector<VkLayerProperties> availableLayers( layerCount );
		vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

		for( auto it = std::begin( s_validationLayers ); it != std::end( s_validationLayers ); ++it )
		{
			auto found = std::find_if( begin( availableLayers ), end( availableLayers ), [&]( const VkLayerProperties& prop ) { return strcmp( prop.layerName, *it ) == 0; } );
			if( found != end( availableLayers ) )
			{
				validationLayers.push_back( *it );
			}
		}
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL ReportVulkanMessage(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objectType,
		uint64_t object,
		size_t location,
		int32_t messageCode,
		const char* pLayerPrefix,
		const char* pMessage,
		void* pUserData )
	{
#ifdef _WIN32
		OutputDebugString( pMessage );
		OutputDebugString( "\n" );
#endif
		if( ( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) != 0 )
		{
			CCP_AL_LOGERR( "Vulkan validation error: %s", pMessage );
		}
		return VK_FALSE;
	}

	ALResult PopulateDeviceInfo( std::vector<TrinityALImpl::VulkanDeviceInfo>& s_devices )
	{
		if( !s_instance )
		{
			std::vector<VkExtensionProperties> availableExtensions;
			FORWARD_HR( TrinityALImpl::QueryArray( &vkEnumerateInstanceExtensionProperties, nullptr, availableExtensions ) );

			if( auto failed = CheckExtensions( availableExtensions, s_requiredInstanceExtensions ) )
			{
				CCP_AL_LOGERR( "Vulkan required extension \"%s\" is not supported", failed );
				return E_FAIL;
			}

			VkApplicationInfo applicationInfo = {
				VK_STRUCTURE_TYPE_APPLICATION_INFO,
				nullptr,
				nullptr,
				VK_MAKE_VERSION( 1, 0, 0 ),
				"Trinity",
				VK_MAKE_VERSION( 1, 0, 0 ),
				VK_MAKE_VERSION( 1,0,0 )
			};

			std::vector<const char*> validationLayers;
			GetValidationLayers( validationLayers );

			VkInstanceCreateInfo instanceCreateInfo = {
				VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
				nullptr,
				0,
				&applicationInfo,
				uint32_t( validationLayers.size() ),
				validationLayers.empty() ? nullptr : validationLayers.data(),
				_countof( s_requiredInstanceExtensions ),
				s_requiredInstanceExtensions
			};


			CR_RETURN_HR( Vk2Al( vkCreateInstance( &instanceCreateInfo, nullptr, &s_instance ) ) );

			if( g_requestDeviceDebugLayer )
			{
				PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT =
					reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>( vkGetInstanceProcAddr( s_instance, "vkCreateDebugReportCallbackEXT" ) );

				VkDebugReportCallbackCreateInfoEXT callbackCreateInfo = {
					VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
					nullptr,
					VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
					&ReportVulkanMessage,
					nullptr
				};

				VkDebugReportCallbackEXT callback;
				Vk2Al( vkCreateDebugReportCallbackEXT( s_instance, &callbackCreateInfo, nullptr, &callback ) );
			}
		}

		std::vector<VkPhysicalDevice> physicalDevices;
		FORWARD_HR( TrinityALImpl::QueryArrayNotEmpty( &vkEnumeratePhysicalDevices, s_instance, physicalDevices ) );

		VkPhysicalDevice selected_physical_device = VK_NULL_HANDLE;
		uint32_t selected_queue_family_index = UINT32_MAX;
		for( uint32_t i = 0; i < uint32_t( physicalDevices.size() ); ++i )
		{
			TrinityALImpl::VulkanDeviceInfo info;
			info.device = physicalDevices[i];

			vkGetPhysicalDeviceProperties( physicalDevices[i], &info.properties );
			vkGetPhysicalDeviceFeatures( physicalDevices[i], &info.features );

			uint32_t version = VK_VERSION_MAJOR( info.properties.apiVersion );

			if( version < 1 || info.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU )
			{
				continue;
			}

			std::vector<VkQueueFamilyProperties> queue_family_properties;
			TrinityALImpl::QueryArrayNoFail( &vkGetPhysicalDeviceQueueFamilyProperties, physicalDevices[i], queue_family_properties );
			if( queue_family_properties.empty() )
			{
				continue;
			}

			bool foundGraphicsQueue = false;
			for( uint32_t j = 0; j < uint32_t( queue_family_properties.size() ); ++j )
			{
				if( ( queue_family_properties[j].queueCount > 0 ) && ( queue_family_properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT ) ) 
				{
					info.graphicsQueue = j;
					foundGraphicsQueue = true;
					break;
				}
			}
			if( !foundGraphicsQueue )
			{
				continue;
			}

			std::vector<VkExtensionProperties> availableExtensions;
			if( FAILED( TrinityALImpl::QueryArray( &vkEnumerateDeviceExtensionProperties, physicalDevices[i], nullptr, availableExtensions ) ) )
			{
				continue;
			}

			if( CheckExtensions( availableExtensions, s_requiredDeviceExtensions ) )
			{
				continue;
			}


			s_devices.push_back( info );
		}
		return S_OK;
	}
}



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
		static const unsigned MAX_SAMPLE_COUNT = 8;

		TrinityALImpl::VulkanDeviceInfo vulkanDevice;

		uint32_t m_formatSupport[FORMAT_COUNT];
		uint8_t m_qualityLevels[FORMAT_COUNT][MAX_SAMPLE_COUNT];
	};

	std::vector<AdapterInfo> s_adapters;
	std::vector<DeviceInfo> s_deviceInfo;

	HRESULT GetFallbackMode( DXGI_MODE_DESC &desc )
	{
		DEVMODE devMode;
		memset( &devMode, 0, sizeof( devMode ) );
		devMode.dmSize = sizeof( devMode );
		if( !EnumDisplaySettings( 0, ENUM_CURRENT_SETTINGS, &devMode ) )
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
		if( hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE && backBufferFormat == DXGI_FORMAT_B8G8R8A8_UNORM )
		{
			CCP_LOGWARN( "Tr2VideoAdapterInfo: failed to enumerate display modes (running in remote desktop?)" );
			count = 1;
		}
		else if( FAILED( hr ) || ( count == 0 ) )
		{
			return hr;
		}

		CCP_ASSERT( count > 0 );

		displayModes.resize( count );
		hr = output.m_output->GetDisplayModeList( static_cast<DXGI_FORMAT>( backBufferFormat ),
			0,
			&count,
			&displayModes[0] );
		if( hr == DXGI_ERROR_NOT_CURRENTLY_AVAILABLE && backBufferFormat == DXGI_FORMAT_B8G8R8A8_UNORM && !displayModes.empty() )
		{
			return GetFallbackMode( displayModes[0] );
		}
		else if( FAILED( hr ) )
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

		std::vector<TrinityALImpl::VulkanDeviceInfo> vulkanDevices;
		FORWARD_HR( PopulateDeviceInfo( vulkanDevices ) );


		HANDLE heapsBefore[256];
		uint32_t countBefore = ::GetProcessHeaps( 256, heapsBefore );
		( countBefore );

		CR_RETURN_HR( CreateDXGIFactory1(
			__uuidof( IDXGIFactory1 ),
			(void**)( &s_factory ) ) );

		uint32_t dwFlags = 0;

		if( !s_factory )
		{
			return E_FAIL;
		}

		CComPtr<IDXGIAdapter1> adapterPtr;

		uint32_t adapterIndex = 0;
		while( SUCCEEDED( s_factory->EnumAdapters1( adapterIndex++, &adapterPtr ) ) )
		{

			DXGI_ADAPTER_DESC1 adapterDesc;
			CR_RETURN_HR( adapterPtr->GetDesc1( &adapterDesc ) );

			//static_assert( sizeof( adapterDesc.AdapterLuid ) == VK_UUID_SIZE, "fuck" );

			CCP_LOG( "Found video adapter \"%S\"", adapterDesc.Description );

			DeviceInfo deviceInfo;
			memset( deviceInfo.m_formatSupport, 0, sizeof( deviceInfo.m_formatSupport ) );
			memset( deviceInfo.m_qualityLevels, 0, sizeof( deviceInfo.m_qualityLevels ) );
			deviceInfo.vulkanDevice.device = nullptr;

			for( uint32_t i = 0; i < uint32_t( vulkanDevices.size() ); ++i )
			{
				auto& vulkanDevice = vulkanDevices[i].properties;
				if( vulkanDevice.vendorID == adapterDesc.VendorId && vulkanDevice.deviceID == adapterDesc.DeviceId )
				{
					deviceInfo.vulkanDevice = vulkanDevices[i];
					break;
				}
			}

			if( !deviceInfo.vulkanDevice.device )
			{
				continue;
			}

			CComPtr<IDXGIOutput> outputPtr;
			uint32_t outputIndex = 0;
			bool hasValidOutputs = false;
			while( SUCCEEDED( adapterPtr->EnumOutputs( outputIndex++, &outputPtr ) ) )
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
			if( hasValidOutputs )
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

	auto & desc = s_adapters[adapterIndex].m_desc;
	auto & outp = s_adapters[adapterIndex].m_outputDesc;

	info.driver = "";
	info.description = desc.Description;
	info.deviceName = CW2A( outp.DeviceName );
	info.driverVersion = 0;
	info.vendorID = desc.VendorId;
	info.deviceID = desc.DeviceId;
	info.subSystemID = desc.SubSysId;
	info.revision = desc.Revision;
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

	auto & mm = s_adapters[adapterIndex].m_displayModes[DXGI_FORMAT_B8G8R8A8_UNORM][0];
	mode.format = static_cast<PixelFormat>( mm.Format );
	mode.refreshRateDenominator = mm.RefreshRate.Denominator;
	mode.refreshRateNumerator = mm.RefreshRate.Numerator;
	mode.scaling = static_cast<DisplayScaling>( mm.Scaling );
	mode.scanlineOrdering = static_cast<ScanlineOrdering>( mm.ScanlineOrdering );
#if 0
	// the above is probably the same for everything, but the dimensions are just whatever happens
	// to be in slot [0], so that's a guess.  TODO: format?
	mode.height = mm.Height;
	mode.width = mm.Width;
#else
	DEVMODE devMode;
	memset( &devMode, 0, sizeof( devMode ) );
	devMode.dmSize = sizeof( devMode );
	if( !EnumDisplaySettings( 0, ENUM_CURRENT_SETTINGS, &devMode ) )
	{
		return E_FAIL;
	}
	mode.width = devMode.dmPelsWidth;
	mode.height = devMode.dmPelsHeight;
#endif

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

	if( modeIndex >= displayModes.size() )
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
	maxWidth = 4096;
	return S_OK;
}

bool Tr2VideoAdapterInfo::SupportsBackBufferFormat( unsigned adapterIndex,
	Tr2RenderContextEnum::PixelFormat backBufferFormat )
{
	CHECK_INIT_BOOL;
	CHECK_VALID_ADAPTER_BOOL;

	if( backBufferFormat >= (unsigned)DeviceInfo::FORMAT_COUNT )
	{
		return false;
	}

	uint32_t flags = s_deviceInfo[s_adapters[adapterIndex].m_deviceInfoIndex].m_formatSupport[backBufferFormat];

	// or should it be D3D11_FORMAT_SUPPORT_DISPLAY?
	//if( ( flags & D3D11_FORMAT_SUPPORT_RENDER_TARGET ) == 0 )
	//{
	//	return false;
	//}

	return true;
}

bool Tr2VideoAdapterInfo::SupportsRenderTargetFormat( unsigned adapterIndex, Tr2RenderContextEnum::PixelFormat format )
{
	CHECK_INIT_BOOL;
	CHECK_VALID_ADAPTER_BOOL;

	if( format >= (unsigned)DeviceInfo::FORMAT_COUNT )
	{
		return false;
	}

	uint32_t flags = s_deviceInfo[s_adapters[adapterIndex].m_deviceInfoIndex].m_formatSupport[format];

	//if( ( flags & D3D11_FORMAT_SUPPORT_RENDER_TARGET ) == 0 )
	//{
	//	return false;
	//}

	//if( withAutoGenMipmap && ( ( flags & D3D11_FORMAT_SUPPORT_MIP_AUTOGEN ) == 0 ) )
	//{
	//	return false;
	//}

	return true;
}

unsigned log2( unsigned int x )
{
	unsigned ans = 0;
	while( x >>= 1 )
	{
		ans++;
	}
	return ans;
}

ALResult Tr2VideoAdapterInfo::GetAdapterMsaaSupport( unsigned adapterIndex,
	Tr2RenderContextEnum::PixelFormat format,
	unsigned msaaType,
	unsigned& msaaQuality )
{
	CHECK_INIT;
	CHECK_VALID_ADAPTER;

	if( msaaType <= 1 )
	{
		msaaQuality = 0;
		return S_OK;
	}

	if( format >= (unsigned)DeviceInfo::FORMAT_COUNT || msaaType >= (unsigned)DeviceInfo::MAX_SAMPLE_COUNT )
	{
		return E_INVALIDARG;
	}

	msaaQuality = s_deviceInfo[s_adapters[adapterIndex].m_deviceInfoIndex].m_qualityLevels[format][msaaType - 2];
	return S_OK;
}

ALResult Tr2VideoAdapterInfo::RefreshData()
{
	return InitializeDirect3D();
}

bool Tr2VideoAdapterInfo::AreAdaptersDifferent( unsigned adapter1,
	unsigned adapter2 )
{
	if( adapter1 == adapter2 )
	{
		return false;
	}
	if( adapter1 >= s_adapters.size() || adapter2 >= s_adapters.size() )
	{
		return true;
	}

	return s_adapters[adapter1].m_adapter != s_adapters[adapter2].m_adapter;
}

namespace TrinityALImpl
{

	ALResult GetVulkanInstance( VkInstance& instance )
	{
		CHECK_INIT;
		instance = s_instance;
		return S_OK;
	}

	ALResult GetPhysicalDevice( unsigned adapterIndex, VulkanDeviceInfo& device )
	{
		CHECK_INIT;
		CHECK_VALID_ADAPTER;
		device = s_deviceInfo[s_adapters[adapterIndex].m_deviceInfoIndex].vulkanDevice;
		return S_OK;
	}
}

#endif