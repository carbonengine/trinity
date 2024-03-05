////////////////////////////////////////////////////////////
//
//    Created:   May 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "UtilitiesVulkan.h"
#include "ALResult.h"
#include "Tr2PrimaryRenderContextVulkan.h"

namespace TrinityALImpl
{
	VkFormat GetVulkanFormat( Tr2RenderContextEnum::PixelFormat format )
	{
		switch( format )
		{
		case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_TYPELESS: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_UINT: return VK_FORMAT_R32G32B32A32_UINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_SINT: return VK_FORMAT_R32G32B32A32_SINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32_TYPELESS: return VK_FORMAT_R32G32B32_SFLOAT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32_FLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32_UINT: return VK_FORMAT_R32G32B32_UINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32_SINT: return VK_FORMAT_R32G32B32_SINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_TYPELESS: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_UNORM: return VK_FORMAT_R16G16B16A16_UNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_UINT: return VK_FORMAT_R16G16B16A16_UINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_SNORM: return VK_FORMAT_R16G16B16A16_SNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16B16A16_SINT: return VK_FORMAT_R16G16B16A16_SINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32_TYPELESS: return VK_FORMAT_R32G32_SFLOAT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32_FLOAT: return VK_FORMAT_R32G32_SFLOAT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32_UINT: return VK_FORMAT_R32G32_UINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R32G32_SINT: return VK_FORMAT_R32G32_SINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R32G8X24_TYPELESS: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_D32_FLOAT_S8X24_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT_X8X24_TYPELESS: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_X32_TYPELESS_G8X24_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R10G10B10A2_TYPELESS: return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R10G10B10A2_UNORM: return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R10G10B10A2_UINT: return VK_FORMAT_A2B10G10R10_UINT_PACK32;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R11G11B10_FLOAT: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_TYPELESS: return VK_FORMAT_R8G8B8A8_UNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_UINT: return VK_FORMAT_R8G8B8A8_UINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_SNORM: return VK_FORMAT_R8G8B8A8_SNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8B8A8_SINT: return VK_FORMAT_R8G8B8A8_SINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_TYPELESS: return VK_FORMAT_R16G16_SFLOAT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_FLOAT: return VK_FORMAT_R16G16_SFLOAT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_UNORM: return VK_FORMAT_R16G16_UNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_UINT: return VK_FORMAT_R16G16_UINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_SNORM: return VK_FORMAT_R16G16_SNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R16G16_SINT: return VK_FORMAT_R16G16_SINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R32_TYPELESS: return VK_FORMAT_R32_SFLOAT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_D32_FLOAT: return VK_FORMAT_D32_SFLOAT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R32_FLOAT: return VK_FORMAT_R32_SFLOAT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R32_UINT: return VK_FORMAT_R32_UINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R32_SINT: return VK_FORMAT_R32_SINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R24G8_TYPELESS: return VK_FORMAT_D24_UNORM_S8_UINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R24_UNORM_X8_TYPELESS: return VK_FORMAT_D24_UNORM_S8_UINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_X24_TYPELESS_G8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_TYPELESS: return VK_FORMAT_R8G8_UNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_UNORM: return VK_FORMAT_R8G8_UNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_UINT: return VK_FORMAT_R8G8_UINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_SNORM: return VK_FORMAT_R8G8_SNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_SINT: return VK_FORMAT_R8G8_SINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R16_TYPELESS: return VK_FORMAT_R16_SFLOAT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R16_FLOAT: return VK_FORMAT_R16_SFLOAT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_D16_UNORM: return VK_FORMAT_D16_UNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R16_UNORM: return VK_FORMAT_R16_UNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R16_UINT: return VK_FORMAT_R16_UINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R16_SNORM: return VK_FORMAT_R16_SNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R16_SINT: return VK_FORMAT_R16_SINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R8_TYPELESS: return VK_FORMAT_R8_UNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R8_UNORM: return VK_FORMAT_R8_UNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R8_UINT: return VK_FORMAT_R8_UINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R8_SNORM: return VK_FORMAT_R8_SNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R8_SINT: return VK_FORMAT_R8_SINT;
		case Tr2RenderContextEnum::PIXEL_FORMAT_A8_UNORM: return VK_FORMAT_UNDEFINED;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R1_UNORM: return VK_FORMAT_UNDEFINED;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R9G9B9E5_SHAREDEXP: return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_B8G8_UNORM: return VK_FORMAT_UNDEFINED;
		case Tr2RenderContextEnum::PIXEL_FORMAT_G8R8_G8B8_UNORM: return VK_FORMAT_UNDEFINED;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC1_TYPELESS: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC1_UNORM: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC1_UNORM_SRGB: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC2_TYPELESS: return VK_FORMAT_BC2_UNORM_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC2_UNORM: return VK_FORMAT_BC2_UNORM_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC2_UNORM_SRGB: return VK_FORMAT_BC2_SRGB_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC3_TYPELESS: return VK_FORMAT_BC3_UNORM_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC3_UNORM: return VK_FORMAT_BC3_UNORM_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC3_UNORM_SRGB: return VK_FORMAT_BC3_SRGB_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC4_TYPELESS: return VK_FORMAT_BC4_UNORM_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC4_UNORM: return VK_FORMAT_BC4_UNORM_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC4_SNORM: return VK_FORMAT_BC4_SNORM_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC5_TYPELESS: return VK_FORMAT_BC5_UNORM_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC5_UNORM: return VK_FORMAT_BC5_UNORM_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC5_SNORM: return VK_FORMAT_BC5_SNORM_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_B5G6R5_UNORM: return VK_FORMAT_B5G6R5_UNORM_PACK16;
		case Tr2RenderContextEnum::PIXEL_FORMAT_B5G5R5A1_UNORM: return VK_FORMAT_B5G5R5A1_UNORM_PACK16;
		case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_R10G10B10_XR_BIAS_A2_UNORM: return VK_FORMAT_UNDEFINED;
		case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_TYPELESS: return VK_FORMAT_B8G8R8A8_UNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
		case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_TYPELESS: return VK_FORMAT_B8G8R8_UNORM;
		case Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB: return VK_FORMAT_B8G8R8A8_SRGB;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC6H_TYPELESS: return VK_FORMAT_BC6H_SFLOAT_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC6H_UF16: return VK_FORMAT_BC6H_UFLOAT_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC6H_SF16: return VK_FORMAT_BC6H_SFLOAT_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC7_TYPELESS: return VK_FORMAT_BC7_UNORM_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC7_UNORM: return VK_FORMAT_BC7_UNORM_BLOCK;
		case Tr2RenderContextEnum::PIXEL_FORMAT_BC7_UNORM_SRGB: return VK_FORMAT_BC7_SRGB_BLOCK;
		default:
			return VK_FORMAT_UNDEFINED;
		}
	}

	ALResult AllocateMemory( VkDeviceMemory& memory, const VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlagBits memoryProperty, Tr2PrimaryRenderContextAL& renderContext )
	{
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties( renderContext.m_physicalDevice, &memoryProperties );

		for( uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i )
		{
			if( ( memoryRequirements.memoryTypeBits & ( 1 << i ) ) && ( ( memoryProperties.memoryTypes[i].propertyFlags & memoryProperty ) == memoryProperty ) )
			{

				VkMemoryAllocateInfo memory_allocate_info = {
					VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
					nullptr,
					memoryRequirements.size,
					i
				};

				if( vkAllocateMemory( renderContext.m_device, &memory_allocate_info, nullptr, &memory ) == VK_SUCCESS )
				{
					return S_OK;
				}
			}
		}
		return E_FAIL;
	}

	ALResult CreateBuffer( VkBuffer& buffer, VkDeviceMemory& memory, size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlagBits memoryProperty, Tr2PrimaryRenderContextAL& renderContext )
	{
		buffer = VK_NULL_HANDLE;
		memory = VK_NULL_HANDLE;

		VkBufferCreateInfo createInfo = {
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			nullptr,
			0,
			size,
			usage,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr
		};

		CR_RETURN_HR( Vk2Al( vkCreateBuffer( renderContext.m_device, &createInfo, nullptr, &buffer ) ) );

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements( renderContext.m_device, buffer, &memoryRequirements );

		CR_RETURN_HR( AllocateMemory( memory, memoryRequirements, memoryProperty, renderContext ) );
		return Vk2Al( vkBindBufferMemory( renderContext.m_device, buffer, memory, 0 ) );
	}

	ALResult CreateImage( VkImage& image, VkDeviceMemory& memory, const Tr2BitmapDimensions& desc, const Tr2MsaaDesc& msaa, VkImageUsageFlags usage, VkMemoryPropertyFlagBits memoryProperty, Tr2PrimaryRenderContextAL& renderContext )
	{
		image = VK_NULL_HANDLE;
		memory = VK_NULL_HANDLE;

		VkImageType imageType;
		if( desc.GetType() == Tr2RenderContextEnum::TEX_TYPE_3D )
		{
			imageType = VK_IMAGE_TYPE_3D;
		}
		else
		{
			imageType = VK_IMAGE_TYPE_2D;
		}

		VkImageCreateInfo createInfo = {
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			nullptr,
			0,
			imageType,
			GetVulkanFormat( desc.GetFormat() ),
			{ desc.GetWidth(), desc.GetHeight(), desc.GetDepth() },
			desc.GetTrueMipCount(),
			desc.GetArraySize(),
			VkSampleCountFlagBits( msaa.samples ),
			VK_IMAGE_TILING_OPTIMAL,
			usage,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr,
			VK_IMAGE_LAYOUT_UNDEFINED
		};

		CR_RETURN_HR( Vk2Al( vkCreateImage( renderContext.m_device, &createInfo, nullptr, &image ) ) );

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements( renderContext.m_device, image, &memoryRequirements );

		CR_RETURN_HR( AllocateMemory( memory, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, renderContext ) );
		CR_RETURN_HR( vkBindImageMemory( renderContext.m_device, image, memory, 0 ) );
		return S_OK;
	}

}
#endif