////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once


#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "../Tr2MemoryCounterAL.h"
#include "../include/Tr2RenderContextAL.h"
#include "../include/Tr2CapsAL.h"
#include "../include/Tr2SamplerStateAL.h"
#include "../include/Tr2TextureAL.h"


struct Tr2PresentParametersAL;

class Tr2PrimaryRenderContextAL : public Tr2RenderContextAL
{
public:
	Tr2PrimaryRenderContextAL();
	~Tr2PrimaryRenderContextAL();

	ALResult CreateDevice( uint32_t adapter, Tr2WindowHandle focusWindow, const Tr2PresentParametersAL& presentationParameters );
	void Destroy();

	ALResult SetPresentParameters( unsigned adapter, const Tr2PresentParametersAL& pPresentationParameters )
	{
		return E_NOTIMPL;
	}

	const Tr2CapsAL& GetCaps() const
	{
		return m_caps;
	}
		
	ALResult Present();

	bool IsValid() const;

	Tr2RenderContextEnum::PixelFormat GetBackBufferFormat() const
	{
		return Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN;
	}
	
	static const uint32_t SHADER_TYPE_MASK = 
		( 1 << Tr2RenderContextEnum::VERTEX_SHADER ) |
		( 1 << Tr2RenderContextEnum::PIXEL_SHADER ) |
		( 1 << Tr2RenderContextEnum::COMPUTE_SHADER ) |
		( 1 << Tr2RenderContextEnum::GEOMETRY_SHADER ) |
		( 1 << Tr2RenderContextEnum::HULL_SHADER ) |
		( 1 << Tr2RenderContextEnum::DOMAIN_SHADER );

	template <typename T>
	void DestroyLaterVulkan( T object, void( VKAPI_CALL *destroyFunction )( VkDevice, T, const VkAllocationCallbacks* ) )
	{
		if( !object )
		{
			return;
		}
		PendingDestroy pd = { (VkBuffer)object, (DestroyFunction)destroyFunction };
		m_frameData[m_frameIndex].pendingDestroys.push_back( pd );
	}

	template <typename T>
	void DestroyLaterVulkan( const std::vector<T> objects, void( VKAPI_CALL *destroyFunction )( VkDevice, T, const VkAllocationCallbacks* ) )
	{
		for( auto it = begin( objects ); it != end( objects ); ++it )
		{
			this->DestroyLaterVulkan( *it, destroyFunction );
		}
	}

	VkBuffer GetZeroBufferVulkan() const;
public:
	Tr2TextureAL m_defaultBackBuffer;
		
	ITr2RenderContextEvents* m_events;

public:
	TrinityALImpl::Tr2SamplerStateALFactory m_samplerStateFactory;

	Tr2TextureAL&			GetDefaultBackBuffer()
	{
		return m_defaultBackBuffer;
	}
private:
	Tr2PrimaryRenderContextAL( const Tr2PrimaryRenderContextAL& ) /* = delete */;
	Tr2PrimaryRenderContextAL& operator=( const Tr2PrimaryRenderContextAL& ) /* = delete */;

	typedef void( VKAPI_CALL *DestroyFunction )( VkDevice, VkBuffer, const VkAllocationCallbacks* );

	struct PendingDestroy
	{
		VkBuffer object;
		DestroyFunction destroyFunction;
	};

	struct FrameData 
	{
		//VkFramebuffer framebuffer;
		VkCommandBuffer commandBuffer;
		VkSemaphore imageAvailableSemaphore;
		VkSemaphore finishedRenderingSemaphore;
		VkFence fence;
		std::vector<PendingDestroy> pendingDestroys;

		FrameData();
	};

	static const uint32_t VIRTUAL_FRAMES = 3;
	FrameData m_frameData[VIRTUAL_FRAMES];
	uint32_t m_frameIndex;

	ALResult BeginFrame();

	Tr2CapsAL m_caps;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	VkSwapchainKHR m_swapChain;
	VkSurfaceKHR m_surface;
	uint32_t m_currentImage;

	VkCommandPool m_commandPool;

	VkBuffer m_zeroBuffer;
	VkDeviceMemory m_zeroBufferMemory;
public:
	VkDevice m_device;
	VkPhysicalDevice m_physicalDevice;
	VkPhysicalDeviceProperties m_physicalDeviceProperties;

	std::map<unsigned, VkRenderPass> m_renderPasses;
	std::map<unsigned, VkPipeline> m_pipelines;
};

#endif
