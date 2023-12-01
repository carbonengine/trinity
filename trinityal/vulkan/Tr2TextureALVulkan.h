////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN


#include "../include/Tr2TextureAL.h"
#include "../include/Tr2BitmapDimensions.h"
#include "../Tr2HalHelperStructures.h"

namespace TrinityALImpl
{
	class Tr2ResourceSetAL;
}

struct Tr2DisplayModeInfo;


namespace TrinityALImpl
{
	class Tr2TextureAL : public Tr2DeviceResourceAL<Tr2TextureAL>
	{
	public:
		Tr2TextureAL();
		~Tr2TextureAL();

		ALResult Create( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext );

		ALResult OpenShared( uintptr_t handle, Tr2GpuUsage::Type gpuUsage, Tr2PrimaryRenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}
		void Destroy();

		bool IsValid() const;
		Tr2ALMemoryType GetMemoryClass() const;
		const Tr2BitmapDimensions& GetDesc() const;
		const Tr2MsaaDesc& GetMsaaDesc() const;
		Tr2GpuUsage::Type GetGpuUsage() const;
		Tr2CpuUsage::Type GetCpuUsage() const;

		ALResult MapForReading( const Tr2TextureSubresource& region, const void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}
		void UnmapForReading( Tr2RenderContextAL& renderContext )
		{

		}
		ALResult MapForWriting( const Tr2TextureSubresource& region, void*& data, uint32_t& pitch, Tr2RenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}
		void UnmapForWriting( Tr2RenderContextAL& renderContext )
		{

		}

		ALResult UpdateSubresource( const Tr2TextureSubresource& region, const void* source, uint32_t pitch, uint32_t slicePitch, Tr2RenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}
		ALResult CopySubresourceRegion( const Tr2TextureSubresource& destSubresource, Tr2TextureAL& source, const Tr2TextureSubresource& sourceSubresource, Tr2RenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}
		ALResult GenerateMipMaps( Tr2RenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}
		ALResult Resolve( Tr2TextureAL& destination, Tr2RenderContextAL& renderContext )
		{
			return E_NOTIMPL;
		}
		uintptr_t GetSharedHandle() const
		{
			return 0;
		}

		ALResult AssignFromSwapChainVulkan( const std::vector<VkImage>& backBuffers, const Tr2DisplayModeInfo& mode, Tr2PrimaryRenderContextAL& renderContext );
		void SetCurrentImageVulkan( uint32_t index );
		VkImage GetImageVulkan() const;
		VkImageView GetImageView() const;
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

	private:
		std::vector<VkImage> m_images;
		std::vector<VkImageView> m_imageViews;
		VkDeviceMemory m_memory;
		Tr2PrimaryRenderContextAL* m_owner;
		uint32_t m_currentIndex;
		VkFormat m_format;

		Tr2BitmapDimensions m_desc;
		Tr2MsaaDesc m_msaa;
		Tr2CpuUsage::Type m_cpuUsage;
		Tr2GpuUsage::Type m_gpuUsage;

		friend class Tr2RenderContextAL;
		friend class TrinityALImpl::Tr2ResourceSetAL;
	};
}

#endif
