////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "../ALResult.h"
#include "../Tr2RenderContextEnum.h"


struct Tr2AdapterInfo;
struct Tr2DisplayModeInfo;


class Tr2VideoAdapterInfo
{
public:
	static const unsigned DEFAULT_ADAPTER = 0;

	static ALResult GetAdapterCount( unsigned& count );
	static ALResult GetAdapterInfo( unsigned adapterIndex, Tr2AdapterInfo& info );

	static ALResult GetAdapterMonitor( unsigned adapterIndex, void*& monitor );
	static ALResult GetAdapterDisplayMode( unsigned adapterIndex, Tr2DisplayModeInfo& mode );
	static ALResult GetAdapterModeCount( unsigned adapterIndex,
		Tr2RenderContextEnum::PixelFormat backBufferFormat,
		unsigned& count );
	static ALResult GetAdapterMode( unsigned adapterIndex,
		Tr2RenderContextEnum::PixelFormat backBufferFormat,
		unsigned modeIndex,
		Tr2DisplayModeInfo& mode );
	static ALResult GetAdapterMsaaSupport( unsigned adapterIndex,
		Tr2RenderContextEnum::PixelFormat format,
		unsigned msaaType,
		unsigned& msaaQuality );
	static bool SupportsBackBufferFormat( unsigned adapterIndex,
		Tr2RenderContextEnum::PixelFormat backBufferFormat );
	static bool SupportsRenderTargetFormat( unsigned adapterIndex Tr2RenderContextEnum::PixelFormat format );
	static ALResult GetAdapterMaxTextureWidth( unsigned adapterIndex, unsigned& maxWidth );
	static bool AreAdaptersDifferent( unsigned adapter1, unsigned adapter2 );

	static ALResult RefreshData();
};

namespace TrinityALImpl
{
	struct VulkanDeviceInfo
	{
		VkPhysicalDevice device;
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;
		uint32_t graphicsQueue;
	};

	ALResult GetPhysicalDevice( unsigned adapterIndex, VulkanDeviceInfo& device );
	ALResult GetVulkanInstance( VkInstance& instance );
}

#endif
