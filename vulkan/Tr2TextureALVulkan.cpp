#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "Tr2TextureALVulkan.h"
#include "Tr2AdapterStructures.h"


namespace TrinityALImpl
{

	Tr2TextureAL::Tr2TextureAL()
		:m_currentIndex( 0 ),
		m_cpuUsage( Tr2CpuUsage::NONE ),
		m_gpuUsage( Tr2GpuUsage::NONE )
	{
	}

	Tr2TextureAL::~Tr2TextureAL()
	{
		Destroy();
	}

	void Tr2TextureAL::Destroy()
	{
		m_images.clear();
		m_currentIndex = 0;
		m_cpuUsage = Tr2CpuUsage::NONE;
		m_gpuUsage = Tr2GpuUsage::NONE;
	}

	bool Tr2TextureAL::IsValid() const
	{
		return !m_images.empty();
	}

	Tr2ALMemoryType Tr2TextureAL::GetMemoryClass()
	{
		return AL_MEMORY_MANAGED;
	}

	const Tr2BitmapDimensions& Tr2TextureAL::GetDesc() const
	{
		return m_desc;
	}

	const Tr2MsaaDesc& Tr2TextureAL::GetMsaaDesc() const
	{
		return m_msaa;
	}

	Tr2GpuUsage::Type Tr2TextureAL::GetGpuUsage() const
	{
		return m_gpuUsage;
	}

	Tr2CpuUsage::Type Tr2TextureAL::GetCpuUsage() const
	{
		return m_cpuUsage;
	}


	void Tr2TextureAL::AssignFromSwapChainVulkan( const std::vector<VkImage>& backBuffers, const Tr2DisplayModeInfo& mode, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		m_images = backBuffers;
		m_desc = Tr2BitmapDimensions( mode.width, mode.height, 1, mode.format );
		m_msaa = Tr2MsaaDesc();
		m_gpuUsage = Tr2GpuUsage::RENDER_TARGET;
	}

	void Tr2TextureAL::SetCurrentImageVulkan( uint32_t index )
	{
		m_currentIndex = index;
	}

	VkImage Tr2TextureAL::GetImageVulkan() const
	{
		return m_images[m_currentIndex];
	}
}

#endif