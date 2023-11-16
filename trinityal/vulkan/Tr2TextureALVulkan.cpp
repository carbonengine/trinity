#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "Tr2TextureALVulkan.h"
#include "Tr2AdapterStructures.h"
#include "Tr2PrimaryRenderContextVulkan.h"
#include "VkResult.h"
#include "UtilitiesVulkan.h"


namespace
{

	ALResult CheckCreationFlags( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage )
	{
		if( HasBufferFlags( gpuUsage ) )
		{
			return E_INVALIDARG;
		}

		if( msaa.samples > 1 )
		{
			if( HasFlag( gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
			{
				return E_INVALIDARG;
			}
			if( cpuUsage != Tr2CpuUsage::NONE )
			{
				return E_INVALIDARG;
			}
			if( desc.GetType() != Tr2RenderContextEnum::TEX_TYPE_2D )
			{
				return E_INVALIDARG;
			}
		}
		if( desc.GetType() != Tr2RenderContextEnum::TEX_TYPE_2D )
		{
			if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_CUBE )
			{
				if( desc.GetArraySize() != 6 )
				{
					return E_INVALIDARG;
				}
			}
			else if( desc.GetArraySize() > 1 )
			{
				return E_INVALIDARG;
			}
		}
		if( desc.GetType() != Tr2RenderContextEnum::TEX_TYPE_2D && HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			return E_INVALIDARG;
		}
		if( msaa.samples > 1 && desc.GetTrueMipCount() > 1 )
		{
			return E_INVALIDARG;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) && HasFlag( cpuUsage, Tr2CpuUsage::WRITE ) )
		{
			return E_INVALIDARG;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) && cpuUsage != Tr2CpuUsage::NONE )
		{
			return E_INVALIDARG;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) && desc.GetTrueMipCount() > 1 )
		{
			return E_INVALIDARG;
		}
		if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_3D && cpuUsage != Tr2CpuUsage::NONE )
		{
			return E_INVALIDARG;
		}
		if( HasFlag( cpuUsage, Tr2CpuUsage::READ ) && HasFlag( cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
		{
			return E_INVALIDARG;
		}
		return S_OK;
	}
}

namespace TrinityALImpl
{

	Tr2TextureAL::Tr2TextureAL()
		:m_owner( nullptr ),
		m_memory( VK_NULL_HANDLE ),
		m_currentIndex( 0 ),
		m_cpuUsage( Tr2CpuUsage::NONE ),
		m_gpuUsage( Tr2GpuUsage::NONE ),
		m_format( VK_FORMAT_UNDEFINED )
	{
	}

	Tr2TextureAL::~Tr2TextureAL()
	{
		Destroy();
	}

	ALResult Tr2TextureAL::Create( const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, Tr2GpuUsage::Type gpuUsage, Tr2CpuUsage::Type cpuUsage, Tr2SubresourceData* initialData, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}

		FORWARD_HR( CheckCreationFlags( desc, msaa, gpuUsage, cpuUsage ) );

		if( !IsWritable( gpuUsage ) && !HasFlag( cpuUsage, Tr2CpuUsage::WRITE ) && !initialData )
		{
			return E_INVALIDARG;
		}

		VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		if( HasFlag( gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{
			usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::RENDER_TARGET ) )
		{
			usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::DEPTH_STENCIL ) )
		{
			usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}
		if( HasFlag( gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			usage |= VK_IMAGE_USAGE_STORAGE_BIT;
		}

		VkImage image;
		VkDeviceMemory memory;

		CR_RETURN_HR( CreateImage( image, memory, desc, msaa, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, renderContext ) );

		VkImageViewCreateInfo image_view_create_info = {
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			nullptr,
			0,
			image,
			desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_3D ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D,
			GetVulkanFormat( desc.GetFormat() ),
			{
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY
			},
			{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0,
				desc.GetTrueMipCount(),
				0,
				desc.GetArraySize()
			}
		};

		VkImageView imageView;
		CR_RETURN_HR( Vk2Al( vkCreateImageView( renderContext.m_device, &image_view_create_info, nullptr, &imageView ) ) );

		if( initialData )
		{
			std::vector<VkBufferImageCopy> copyInfo;
			size_t index = 0;
			size_t size = 0;
			for( uint32_t i = 0; i < desc.GetArraySize(); ++i )
			{
				for( uint32_t j = 0; j < desc.GetMipCount(); ++j )
				{
					VkBufferImageCopy buffer_image_copy_info = {
						size,                                  // VkDeviceSize               bufferOffset
						initialData[index].m_sysMemPitch / Tr2RenderContextEnum::GetBytesPerPixel( desc.GetFormat() ),   // uint32_t                   bufferRowLength
						initialData[index].m_sysMemSlicePitch / Tr2RenderContextEnum::GetBytesPerPixel( desc.GetFormat() ),                                  // uint32_t                   bufferImageHeight
						{ VK_IMAGE_ASPECT_COLOR_BIT, j, i, 1 },
						{ 0, 0, 0 },
						{ desc.GetMipWidth( j ), desc.GetMipHeight( j ), desc.GetMipDepth( j ) }
					};
					copyInfo.push_back( buffer_image_copy_info );
					size += initialData[index].m_sysMemSlicePitch * desc.GetMipDepth( j );
					++index;
				}
			}

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingMemory;
			CreateBuffer( stagingBuffer, stagingMemory, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, renderContext );

			void *data;
			Vk2Al( vkMapMemory( renderContext.m_device, stagingMemory, 0, size, 0, &data ) );

			for( size_t i = 0; i < copyInfo.size(); ++i )
			{
				memcpy( static_cast<uint8_t*>( data ) + copyInfo[i].bufferOffset, initialData[i].m_sysMem, initialData[i].m_sysMemSlicePitch * copyInfo[i].imageExtent.depth );
			}

			VkMappedMemoryRange flush_range = {
				VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
				nullptr,
				stagingMemory,
				0,
				VK_WHOLE_SIZE
			};
			vkFlushMappedMemoryRanges( renderContext.m_device, 1, &flush_range );

			vkUnmapMemory( renderContext.m_device, stagingMemory );


			VkImageMemoryBarrier barrier = {
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				nullptr,
				0,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				image,
				{
					VK_IMAGE_ASPECT_COLOR_BIT,
					0,
					desc.GetTrueMipCount(),
					0,
					desc.GetArraySize()
				}
			};
			vkCmdPipelineBarrier( renderContext.m_commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier );

			vkCmdCopyBufferToImage( renderContext.m_commandBuffer, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, uint32_t( copyInfo.size() ), copyInfo.data() );

			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			vkCmdPipelineBarrier( renderContext.m_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier );

			renderContext.DestroyLaterVulkan( stagingBuffer, vkDestroyBuffer );
			renderContext.DestroyLaterVulkan( stagingMemory, vkFreeMemory );
		}

		m_images.push_back( image );
		m_imageViews.push_back( imageView );
		m_memory = memory;
		m_cpuUsage = cpuUsage;
		m_gpuUsage = gpuUsage;
		m_desc = desc;
		m_msaa = msaa;
		m_format = GetVulkanFormat( desc.GetFormat() );
		m_owner = &renderContext;

		return S_OK;
	}

	void Tr2TextureAL::Destroy()
	{
		if( m_owner )
		{
			if( m_memory )
			{
				m_owner->DestroyLaterVulkan( m_memory, vkFreeMemory );
			}
			for( auto it = begin( m_imageViews ); it != end( m_imageViews ); ++it )
			{
				m_owner->DestroyLaterVulkan( *it, vkDestroyImageView );
			}
			if( m_images.size() == 1 )
			{
				// don't destroy swap chain images?
				for( auto it = begin( m_images ); it != end( m_images ); ++it )
				{
					m_owner->DestroyLaterVulkan( *it, vkDestroyImage );
				}
			}
			m_images.clear();
			m_imageViews.clear();
		}
		m_currentIndex = 0;
		m_cpuUsage = Tr2CpuUsage::NONE;
		m_gpuUsage = Tr2GpuUsage::NONE;
		m_format = VK_FORMAT_UNDEFINED;
	}

	bool Tr2TextureAL::IsValid() const
	{
		return !m_images.empty();
	}

	Tr2ALMemoryType Tr2TextureAL::GetMemoryClass() const
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


	ALResult Tr2TextureAL::AssignFromSwapChainVulkan( const std::vector<VkImage>& backBuffers, const Tr2DisplayModeInfo& mode, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		std::vector<VkImageView> views;
		for( auto it = begin( backBuffers ); it != end( backBuffers ); ++it )
		{
			VkImageViewCreateInfo imageViewInfo = {
				VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				nullptr,
				0,
				*it,
				VK_IMAGE_VIEW_TYPE_2D,
				GetVulkanFormat( mode.format ),
				{
					VK_COMPONENT_SWIZZLE_IDENTITY,
					VK_COMPONENT_SWIZZLE_IDENTITY,
					VK_COMPONENT_SWIZZLE_IDENTITY,
					VK_COMPONENT_SWIZZLE_IDENTITY
				},
				{
					VK_IMAGE_ASPECT_COLOR_BIT,
					0,
					1,
					0,
					1
				}
			};

			VkImageView view;

			CR_RETURN_HR( Vk2Al( vkCreateImageView( renderContext.m_device, &imageViewInfo, nullptr, &view ) ) );
			views.push_back( view );
		}

		m_images = backBuffers;
		m_imageViews = views;
		m_desc = Tr2BitmapDimensions( mode.width, mode.height, 1, mode.format );
		m_msaa = Tr2MsaaDesc();
		m_gpuUsage = Tr2GpuUsage::RENDER_TARGET;
		m_format = GetVulkanFormat( mode.format );
		m_owner = &renderContext;
		return S_OK;
	}

	void Tr2TextureAL::SetCurrentImageVulkan( uint32_t index )
	{
		m_currentIndex = index;
	}

	VkImage Tr2TextureAL::GetImageVulkan() const
	{
		return m_images[m_currentIndex];
	}

	VkImageView Tr2TextureAL::GetImageView() const
	{
		return m_imageViews[m_currentIndex];
	}

	void Tr2TextureAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2TextureAL";
	}

	ALResult Tr2TextureAL::SetName( const char* )
	{
		return E_NOTIMPL;
	}
}

#endif