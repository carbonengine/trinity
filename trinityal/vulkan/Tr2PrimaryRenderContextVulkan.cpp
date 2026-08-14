#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "Tr2PrimaryRenderContextVulkan.h"
#include "Tr2VideoAdapterInfoALVulkan.h"
#include "ALLog.h"
#include "VkResult.h"
#include "ITr2RenderContextEvents.h"
#include "Tr2AdapterStructures.h"
#include "UtilitiesVulkan.h"
#include "Tr2TextureALVulkan.h"

namespace
{
	bool FindPresentableQueues( VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t& graphicsQueue, uint32_t& presentQueue )
	{
		graphicsQueue = 0xffffffff;
		presentQueue = 0xffffffff;

		std::vector<VkQueueFamilyProperties> queue_family_properties;
		TrinityALImpl::QueryArrayNoFail( &vkGetPhysicalDeviceQueueFamilyProperties, device, queue_family_properties );
		if( queue_family_properties.empty() )
		{
			return false;
		}

		std::vector<VkBool32> presentSupport( queue_family_properties.size() );

		for( uint32_t j = 0; j < uint32_t( queue_family_properties.size() ); ++j )
		{
			vkGetPhysicalDeviceSurfaceSupportKHR( device, j, surface, &presentSupport[j] );

			if( ( queue_family_properties[j].queueCount > 0 ) && ( queue_family_properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT ) )
			{
				if( graphicsQueue == 0xffffffff )
				{
					graphicsQueue = j;
				}
				if( presentSupport[j] )
				{
					graphicsQueue = j;
					presentQueue = j;
					return true;
				}
			}
		}
		if( graphicsQueue == 0xffffffff )
		{
			return false;
		}
		for( uint32_t i = 0; i < uint32_t( queue_family_properties.size() ); ++i )
		{
			if( presentSupport[i] )
			{
				presentQueue = i;
				return true;
			}
		}
		return false;
	}

	uint32_t GetSwapChainNumImages( VkSurfaceCapabilitiesKHR &surfaceCapabilities ) 
	{
		// Set of images defined in a swap chain may not always be available for application to render to:
		// One may be displayed and one may wait in a queue to be presented
		// If application wants to use more images at the same time it must ask for more images
		uint32_t count = surfaceCapabilities.minImageCount + 1;
		if( ( surfaceCapabilities.maxImageCount > 0 ) && ( count > surfaceCapabilities.maxImageCount ) ) 
		{
			count = surfaceCapabilities.maxImageCount;
		}
		return count;
	}

	VkSurfaceFormatKHR GetSwapChainFormat( std::vector<VkSurfaceFormatKHR> &surfaceFormats ) 
	{
		// If the list contains only one entry with undefined format
		// it means that there are no preferred surface formats and any can be chosen
		if( ( surfaceFormats.size() == 1 ) && ( surfaceFormats[0].format == VK_FORMAT_UNDEFINED ) ) 
		{
			VkSurfaceFormatKHR fmt = { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
			return fmt;
		}

		// Check if list contains most widely used R8 G8 B8 A8 format
		// with nonlinear color space
		for( auto surfaceFormat = begin( surfaceFormats ); surfaceFormat != end( surfaceFormats ); ++surfaceFormat )
		{
			if( surfaceFormat->format == VK_FORMAT_R8G8B8A8_UNORM ) 
			{
				return *surfaceFormat;
			}
		}

		// Return the first format from the list
		return surfaceFormats[0];
	}

	VkExtent2D GetSwapChainExtent( VkSurfaceCapabilitiesKHR &surfaceCapabilities ) 
	{
		// Special value of surface extent is width == height == -1
		// If this is so we define the size by ourselves but it must fit within defined confines
		if( surfaceCapabilities.currentExtent.width == -1 ) 
		{
			VkExtent2D swap_chain_extent = { 640, 480 };
			if( swap_chain_extent.width < surfaceCapabilities.minImageExtent.width ) 
			{
				swap_chain_extent.width = surfaceCapabilities.minImageExtent.width;
			}
			if( swap_chain_extent.height < surfaceCapabilities.minImageExtent.height ) 
			{
				swap_chain_extent.height = surfaceCapabilities.minImageExtent.height;
			}
			if( swap_chain_extent.width > surfaceCapabilities.maxImageExtent.width ) 
			{
				swap_chain_extent.width = surfaceCapabilities.maxImageExtent.width;
			}
			if( swap_chain_extent.height > surfaceCapabilities.maxImageExtent.height ) 
			{
				swap_chain_extent.height = surfaceCapabilities.maxImageExtent.height;
			}
			return swap_chain_extent;
		}

		// Most of the cases we define size of the swap_chain images equal to current window's size
		return surfaceCapabilities.currentExtent;
	}

	VkImageUsageFlags GetSwapChainUsageFlags( VkSurfaceCapabilitiesKHR &surface_capabilities ) 
	{
		// Color attachment flag must always be supported
		// We can define other usage flags but we always need to check if they are supported
		if( surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT ) 
		{
			return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		return static_cast<VkImageUsageFlags>( -1 );
	}

	VkSurfaceTransformFlagBitsKHR GetSwapChainTransform( VkSurfaceCapabilitiesKHR &surface_capabilities ) 
	{
		// Sometimes images must be transformed before they are presented (i.e. due to device's orienation
		// being other than default orientation)
		// If the specified transform is other than current transform, presentation engine will transform image
		// during presentation operation; this operation may hit performance on some platforms
		// Here we don't want any transformations to occur so if the identity transform is supported use it
		// otherwise just use the same transform as current transform
		if( surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ) 
		{
			return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}
		else 
		{
			return surface_capabilities.currentTransform;
		}
	}

	VkPresentModeKHR GetSwapChainPresentMode( Tr2RenderContextEnum::PresentInterval interval, std::vector<VkPresentModeKHR> &present_modes ) 
	{
		if( interval == Tr2RenderContextEnum::PRESENT_INTERVAL_IMMEDIATE )
		{
			if( std::find( begin( present_modes ), end( present_modes ), VK_PRESENT_MODE_IMMEDIATE_KHR ) != end( present_modes ) )
			{
				return VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
			if( std::find( begin( present_modes ), end( present_modes ), VK_PRESENT_MODE_MAILBOX_KHR ) != end( present_modes ) )
			{
				return VK_PRESENT_MODE_MAILBOX_KHR;
			}
		}
		if( std::find( begin( present_modes ), end( present_modes ), VK_PRESENT_MODE_FIFO_KHR ) != end( present_modes ) )
		{
			return VK_PRESENT_MODE_FIFO_KHR;
		}
		return static_cast<VkPresentModeKHR>( -1 );
	}
}

Tr2PrimaryRenderContextAL::FrameData::FrameData()
	:commandBuffer( VK_NULL_HANDLE ),
	imageAvailableSemaphore( VK_NULL_HANDLE ),
	finishedRenderingSemaphore( VK_NULL_HANDLE ),
	fence( VK_NULL_HANDLE )
{
}


Tr2PrimaryRenderContextAL::Tr2PrimaryRenderContextAL()
	:m_events( nullptr ),
	m_device( VK_NULL_HANDLE ),
	m_physicalDevice( VK_NULL_HANDLE ),
	m_graphicsQueue( VK_NULL_HANDLE ),
	m_presentQueue( VK_NULL_HANDLE ),
	m_surface( VK_NULL_HANDLE ),
	m_swapChain( VK_NULL_HANDLE ),
	m_commandPool( VK_NULL_HANDLE ),
	m_currentImage( 0 ),
	m_zeroBuffer( VK_NULL_HANDLE ),
	m_zeroBufferMemory( VK_NULL_HANDLE )
{
	m_defaultBackBuffer.m_texture = std::make_shared<TrinityALImpl::Tr2TextureAL>();
}

Tr2PrimaryRenderContextAL::~Tr2PrimaryRenderContextAL()
{
	Destroy();
}

ALResult Tr2PrimaryRenderContextAL::CreateDevice(
	uint32_t adapter,
	Tr2WindowHandle  focusWindow,
	const Tr2PresentParametersAL& presentationParameters )
{
	Destroy();

	VkInstance instance;
	FORWARD_HR( TrinityALImpl::GetVulkanInstance( instance ) );
	TrinityALImpl::VulkanDeviceInfo physicalDevice;
	FORWARD_HR( TrinityALImpl::GetPhysicalDevice( adapter, physicalDevice ) );

	const bool isWindowless = ( focusWindow == 0 ) && presentationParameters.software;

	VkDevice device = VK_NULL_HANDLE;
	ON_BLOCK_EXIT( [=] { if( device != VK_NULL_HANDLE ) vkDestroyDevice( device, nullptr ); } );
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	ON_BLOCK_EXIT( [=] { if( surface != VK_NULL_HANDLE ) vkDestroySurfaceKHR( instance, surface, nullptr ); } );
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	ON_BLOCK_EXIT( [=] { if( swapChain != VK_NULL_HANDLE ) vkDestroySwapchainKHR( device, swapChain, nullptr ); } );
	std::vector<VkImage> backBuffers;

	uint32_t graphicsQueue = physicalDevice.graphicsQueue;
	uint32_t presentQueue = graphicsQueue;

	if( !isWindowless )
	{
#if defined(VK_USE_PLATFORM_WIN32_KHR)

		VkWin32SurfaceCreateInfoKHR surfacecreateInfo = {
			VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			nullptr,
			0,
			GetModuleHandle( nullptr ),
			focusWindow
		};

		CR_RETURN_HR( Vk2Al( vkCreateWin32SurfaceKHR( instance, &surfacecreateInfo, nullptr, &surface ) ) );
#else
		static_assert( false, "Define swapchain creation for this platform here" );
#endif
		if( !FindPresentableQueues( physicalDevice.device, surface, graphicsQueue, presentQueue ) )
		{
			CCP_AL_LOGERR( "Could not find graphics queues for the selected device" );
			return E_FAIL;
		}
	}

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::vector<float> queuePriorities;
	queuePriorities.push_back( 1 );

	VkDeviceQueueCreateInfo graphicsQueueInfo = {
		VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		nullptr,
		0,
		graphicsQueue,
		uint32_t( queuePriorities.size() ),
		&queuePriorities[0]
	};
	queueCreateInfos.push_back( graphicsQueueInfo );
	if( graphicsQueue != presentQueue )
	{
		VkDeviceQueueCreateInfo presentQueueInfo = {
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			nullptr,
			0,
			presentQueue,
			uint32_t( queuePriorities.size() ),
			&queuePriorities[0]
		};
		queueCreateInfos.push_back( presentQueueInfo );
	}

	const char* extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_MAINTENANCE1_EXTENSION_NAME
	};

	VkDeviceCreateInfo device_create_info = {
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		nullptr,
		0,
		uint32_t( queueCreateInfos.size() ),
		&queueCreateInfos[0],
		0,
		nullptr,
		_countof( extensions ),
		extensions,
		nullptr
	};

	CR_RETURN_HR( Vk2Al( vkCreateDevice( physicalDevice.device, &device_create_info, nullptr, &device ) ) );

	if( !isWindowless )
	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		CR_RETURN_HR( Vk2Al( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( physicalDevice.device, surface, &surfaceCapabilities ) ) );

		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		FORWARD_HR( TrinityALImpl::QueryArrayNotEmpty( &vkGetPhysicalDeviceSurfaceFormatsKHR, physicalDevice.device, surface, surfaceFormats ) );

		std::vector<VkPresentModeKHR> presentModes;
		FORWARD_HR( TrinityALImpl::QueryArrayNotEmpty( &vkGetPhysicalDeviceSurfacePresentModesKHR, physicalDevice.device, surface, presentModes ) );

		uint32_t desired_number_of_images = GetSwapChainNumImages( surfaceCapabilities );
		VkSurfaceFormatKHR desired_format = GetSwapChainFormat( surfaceFormats );
		VkExtent2D desired_extent = { presentationParameters.mode.width, presentationParameters.mode.height };
		VkImageUsageFlags desired_usage = GetSwapChainUsageFlags( surfaceCapabilities );
		VkSurfaceTransformFlagBitsKHR desired_transform = GetSwapChainTransform( surfaceCapabilities );
		VkPresentModeKHR desired_present_mode = GetSwapChainPresentMode( presentationParameters.presentInterval, presentModes );

		if( static_cast<int>( desired_usage ) == -1 ) {
			return E_FAIL;
		}
		if( static_cast<int>( desired_present_mode ) == -1 ) {
			return E_FAIL;
		}

		VkSwapchainCreateInfoKHR swapChainCreateInfo = {
			VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			nullptr,
			0,
			surface,
			desired_number_of_images,
			desired_format.format,
			desired_format.colorSpace,
			desired_extent,
			1,
			desired_usage,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr,
			desired_transform,
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			desired_present_mode,
			VK_TRUE,
			VK_NULL_HANDLE
		};

		CR_RETURN_HR( Vk2Al( vkCreateSwapchainKHR( device, &swapChainCreateInfo, nullptr, &swapChain ) ) );

		VkSemaphoreCreateInfo semaphoreInfo = {
			VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			nullptr,
			0
		};

		CR_RETURN_HR( TrinityALImpl::QueryArray( &vkGetSwapchainImagesKHR, device, swapChain, backBuffers ) );
	}


	VkCommandPoolCreateInfo cmd_pool_create_info = {
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		nullptr,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		presentQueue
	};

	VkCommandPool commandPool = VK_NULL_HANDLE;
	ON_BLOCK_EXIT( [=] {if( commandPool != VK_NULL_HANDLE ) vkDestroyCommandPool( device, commandPool, nullptr ); } );
	CR_RETURN_HR( Vk2Al( vkCreateCommandPool( device, &cmd_pool_create_info, nullptr, &commandPool ) ) );

	FrameData frameData[VIRTUAL_FRAMES];

	for( size_t i = 0; i < VIRTUAL_FRAMES; ++i )
	{
		VkCommandBufferAllocateInfo cmd_buffer_allocate_info = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			nullptr,
			commandPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1
		};
		CR_RETURN_HR( Vk2Al( vkAllocateCommandBuffers( device, &cmd_buffer_allocate_info, &frameData[i].commandBuffer ) ) );
		if( !isWindowless )
		{
			VkSemaphoreCreateInfo semaphoreInfo = {
				VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
				nullptr,
				0
			};

			CR_RETURN_HR( Vk2Al( vkCreateSemaphore( device, &semaphoreInfo, nullptr, &frameData[i].imageAvailableSemaphore ) ) );
			CR_RETURN_HR( Vk2Al( vkCreateSemaphore( device, &semaphoreInfo, nullptr, &frameData[i].finishedRenderingSemaphore ) ) );
		}
		else
		{
			frameData[i].imageAvailableSemaphore = VK_NULL_HANDLE;
			frameData[i].finishedRenderingSemaphore = VK_NULL_HANDLE;
		}

		VkFenceCreateInfo fenceCreateInfo = {
			VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			nullptr,
			VK_FENCE_CREATE_SIGNALED_BIT
		};

		CR_RETURN_HR( Vk2Al( vkCreateFence( device, &fenceCreateInfo, nullptr, &frameData[i].fence ) ) );
	}

	vkGetDeviceQueue( device, graphicsQueue, 0, &m_graphicsQueue );
	vkGetDeviceQueue( device, presentQueue, 0, &m_presentQueue );

	m_device = device;
	m_physicalDevice = physicalDevice.device;
	m_physicalDeviceProperties = physicalDevice.properties;
	m_surface = surface;
	m_swapChain = swapChain;
	for( size_t i = 0; i < VIRTUAL_FRAMES; ++i )
	{
		m_frameData[i] = frameData[i];
	}

	m_defaultBackBuffer.m_texture->AssignFromSwapChainVulkan( backBuffers, presentationParameters.mode, *this );

	m_commandPool = commandPool;
	m_frameIndex = 0;

	device = VK_NULL_HANDLE;
	surface = VK_NULL_HANDLE;
	swapChain = VK_NULL_HANDLE;
	backBuffers.clear();
	commandPool = VK_NULL_HANDLE;

	m_owner = this;

	BeginFrame();

	{
		TrinityALImpl::CreateBuffer( m_zeroBuffer, m_zeroBufferMemory, 4, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *this );
	}

	if( m_events )
	{
		m_events->OnContextCreated( *this );
	}

	return S_OK;
}

void Tr2PrimaryRenderContextAL::Destroy()
{
	Tr2RenderContextAL::Destroy();

	m_samplerStateFactory.Clear();

	if( m_device != VK_NULL_HANDLE )
	{
		vkDeviceWaitIdle( m_device );
	}

	for( auto it = begin( m_pipelines ); it != end( m_pipelines ); ++it )
	{
		vkDestroyPipeline( m_device, it->second, nullptr );
	}
	m_pipelines.clear();

	for( auto it = begin( m_renderPasses ); it != end( m_renderPasses ); ++it )
	{
		vkDestroyRenderPass( m_device, it->second, nullptr );
	}
	m_renderPasses.clear();

	m_defaultBackBuffer.m_texture->Destroy();

	if( m_zeroBuffer )
	{
		vkDestroyBuffer( m_device, m_zeroBuffer, nullptr );
		m_zeroBuffer = VK_NULL_HANDLE;
		vkFreeMemory( m_device, m_zeroBufferMemory, nullptr );
		m_zeroBufferMemory = VK_NULL_HANDLE;
	}
	for( size_t i = 0; i < VIRTUAL_FRAMES; ++i )
	{
		if( m_frameData[i].commandBuffer != VK_NULL_HANDLE )
		{
			vkFreeCommandBuffers( m_device, m_commandPool, 1, &m_frameData[i].commandBuffer );
			m_frameData[i].commandBuffer = VK_NULL_HANDLE;
		}
		if( m_frameData[i].fence != VK_NULL_HANDLE )
		{
			vkDestroyFence( m_device, m_frameData[i].fence, nullptr );
			m_frameData[i].fence = VK_NULL_HANDLE;
		}
		if( m_frameData[i].finishedRenderingSemaphore != VK_NULL_HANDLE )
		{
			vkDestroySemaphore( m_device, m_frameData[i].finishedRenderingSemaphore, nullptr );
			m_frameData[i].finishedRenderingSemaphore = VK_NULL_HANDLE;
		}
		if( m_frameData[i].imageAvailableSemaphore != VK_NULL_HANDLE )
		{
			vkDestroySemaphore( m_device, m_frameData[i].imageAvailableSemaphore, nullptr );
			m_frameData[i].imageAvailableSemaphore = VK_NULL_HANDLE;
		}

		for( auto it = begin( m_frameData[i].pendingDestroys ); it != end( m_frameData[i].pendingDestroys ); ++it )
		{
			( *it->destroyFunction )( m_device, it->object, nullptr );
		}
		m_frameData[i].pendingDestroys.clear();
	}

	if( m_commandPool != VK_NULL_HANDLE )
	{
		vkDestroyCommandPool( m_device, m_commandPool, nullptr );
		m_commandPool = VK_NULL_HANDLE;
	}

	if( m_swapChain != VK_NULL_HANDLE )
	{
		vkDestroySwapchainKHR( m_device, m_swapChain, nullptr );
		m_swapChain = VK_NULL_HANDLE;
	}
	if( m_surface != VK_NULL_HANDLE )
	{
		VkInstance instance;
		TrinityALImpl::GetVulkanInstance( instance );
		vkDestroySurfaceKHR( instance, m_surface, nullptr );
		m_surface = VK_NULL_HANDLE;
	}
	if( m_device != VK_NULL_HANDLE )
	{
		vkDestroyDevice( m_device, nullptr );
		m_device = VK_NULL_HANDLE;
	}
	m_graphicsQueue = VK_NULL_HANDLE;
	m_presentQueue = VK_NULL_HANDLE;
	m_physicalDevice = VK_NULL_HANDLE;
}

bool Tr2PrimaryRenderContextAL::IsValid() const
{
	return m_device != VK_NULL_HANDLE;
}

ALResult Tr2PrimaryRenderContextAL::Present()
{
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}

	if( m_renderPass )
	{
		vkCmdEndRenderPass( m_commandBuffer );
		m_renderPass = VK_NULL_HANDLE;
	}

	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	VkImageMemoryBarrier barrier = {
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		nullptr,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_MEMORY_READ_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		m_defaultBackBuffer.m_texture->GetImageVulkan(),
		subresourceRange
	};
	vkCmdPipelineBarrier( m_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier );

	CR_RETURN_HR( Vk2Al( vkEndCommandBuffer( m_commandBuffer ) ) );

	VkPipelineStageFlags waitMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	VkSubmitInfo submitInfo = {
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		nullptr,
		1,
		&m_frameData[m_frameIndex].imageAvailableSemaphore,
		&waitMask,
		1,
		&m_commandBuffer,
		1,
		&m_frameData[m_frameIndex].finishedRenderingSemaphore
	};
	CR_RETURN_HR( Vk2Al( vkQueueSubmit( m_presentQueue, 1, &submitInfo, m_frameData[m_frameIndex].fence ) ) );

	VkPresentInfoKHR presentInfo = {
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		nullptr,
		1,
		&m_frameData[m_frameIndex].finishedRenderingSemaphore,
		1,
		&m_swapChain,
		&m_currentImage,
		nullptr
	};
	CR_RETURN_HR( Vk2Al( vkQueuePresentKHR( m_presentQueue, &presentInfo ) ) );

	FORWARD_HR( BeginFrame() );

	return S_OK;
}

ALResult Tr2PrimaryRenderContextAL::BeginFrame()
{

	m_frameIndex = ( m_frameIndex + 1 ) % VIRTUAL_FRAMES;
	CR( Vk2Al( vkWaitForFences( m_device, 1, &m_frameData[m_frameIndex].fence, VK_FALSE, 1000000000 ) ) );
	vkResetFences( m_device, 1, &m_frameData[m_frameIndex].fence );

	for( auto it = begin( m_frameData[m_frameIndex].pendingDestroys ); it != end( m_frameData[m_frameIndex].pendingDestroys ); ++it )
	{
		( *it->destroyFunction )( m_device, it->object, nullptr );
	}
	m_frameData[m_frameIndex].pendingDestroys.clear();


	CR_RETURN_HR( Vk2Al( vkAcquireNextImageKHR( m_device, m_swapChain, UINT64_MAX, m_frameData[m_frameIndex].imageAvailableSemaphore, VK_NULL_HANDLE, &m_currentImage ) ) );
	m_defaultBackBuffer.m_texture->SetCurrentImageVulkan( m_currentImage );
	m_commandBuffer = m_frameData[m_frameIndex].commandBuffer;

	VkCommandBufferBeginInfo cmd_buffer_begin_info = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		nullptr
	};

	CR_RETURN_HR( Vk2Al( vkBeginCommandBuffer( m_commandBuffer, &cmd_buffer_begin_info ) ) );

	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	VkImageMemoryBarrier barrier = {
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		nullptr,
		VK_ACCESS_MEMORY_READ_BIT,
		VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		m_defaultBackBuffer.m_texture->GetImageVulkan(),
		subresourceRange
	};
	vkCmdPipelineBarrier( m_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier );

	SetRenderTarget( m_defaultBackBuffer );

	return S_OK;
}

VkBuffer Tr2PrimaryRenderContextAL::GetZeroBufferVulkan() const
{
	return m_zeroBuffer;
}
#endif