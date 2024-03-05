////////////////////////////////////////////////////////////
//
//    Created:   April 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "Tr2BufferALVulkan.h"
#include "Tr2PrimaryRenderContextVulkan.h"
#include "UtilitiesVulkan.h"


namespace TrinityALImpl
{
	Tr2BufferAL::Tr2BufferAL()
		:m_buffer( VK_NULL_HANDLE ),
		m_memory( VK_NULL_HANDLE ),
		m_owner( nullptr )
	{

	}

	Tr2BufferAL::~Tr2BufferAL()
	{
		Destroy();
	}

	ALResult Tr2BufferAL::Create(
		const Tr2BufferDescriptionAL& desc,
		const void* initialData,
		Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( desc.count == 0 )
		{
			return E_INVALIDARG;
		}

		if( !renderContext.IsValid() )
		{
			return E_INVALIDCALL;
		}

		bool isImmutable = !HasFlag( desc.cpuUsage, Tr2CpuUsage::WRITE ) && !HasFlag( desc.gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS );
		if( isImmutable && !initialData )
		{
			return E_INVALIDARG;
		}

		if( HasFlag( desc.cpuUsage, Tr2CpuUsage::READ ) && HasFlag( desc.cpuUsage, Tr2CpuUsage::WRITE ) )
		{
			return E_INVALIDARG;
		}

		auto stride = desc.stride;
		if( desc.format != Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN )
		{
			stride = GetBytesPerPixel( desc.format );
		}

		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::INDEX_BUFFER ) && stride != 2 && stride != 4 )
		{
			return E_INVALIDARG;
		}

		auto size = desc.count * stride;

		VkBufferUsageFlags usage = 0;
		if( initialData || HasFlag( desc.cpuUsage, Tr2CpuUsage::WRITE ) )
		{
			usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}
		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::INDEX_BUFFER ) )
		{
			usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		}
		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::VERTEX_BUFFER ) )
		{
			usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		}
		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::DRAW_INDIRECT_ARGS ) )
		{
			usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		}
		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::SHADER_RESOURCE ) )
		{
			return E_NOTIMPL;
		}
		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::UNORDERED_ACCESS ) )
		{
			return E_NOTIMPL;
		}

		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		FORWARD_HR( CreateBuffer( buffer, memory, size, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, renderContext ) );
		ON_BLOCK_EXIT( [&] {
			
			if( buffer ) renderContext.DestroyLaterVulkan( buffer, &vkDestroyBuffer );
			if( memory ) renderContext.DestroyLaterVulkan( memory, &vkFreeMemory );
		} );

		if( initialData )
		{
			VkBuffer stagingBuffer = VK_NULL_HANDLE;
			VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
			FORWARD_HR( CreateBuffer( stagingBuffer, stagingMemory, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, renderContext ) );

			ON_BLOCK_EXIT( [&] {
				if( stagingBuffer ) renderContext.DestroyLaterVulkan( stagingBuffer, &vkDestroyBuffer );
				if( stagingMemory ) renderContext.DestroyLaterVulkan( stagingMemory, &vkFreeMemory );
			} );


			void *mapped;
			CR_RETURN_HR( Vk2Al( vkMapMemory( renderContext.m_device, stagingMemory, 0, size, 0, &mapped ) ) );
			memcpy( mapped, initialData, size );

			VkMappedMemoryRange flushRange = {
				VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
				nullptr,
				stagingMemory,
				0,
				VK_WHOLE_SIZE
			};
			vkFlushMappedMemoryRanges( renderContext.m_device, 1, &flushRange );
			vkUnmapMemory( renderContext.m_device, stagingMemory );

			VkBufferCopy copyInfo = { 0, 0, size };
			vkCmdCopyBuffer( renderContext.m_commandBuffer, stagingBuffer, buffer, 1, &copyInfo );

			VkBufferMemoryBarrier barrier = {
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				nullptr,
				VK_ACCESS_MEMORY_WRITE_BIT,
				VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				buffer,
				0,
				VK_WHOLE_SIZE
			};
			vkCmdPipelineBarrier( renderContext.m_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr );
		}

		m_buffer = buffer;
		buffer = VK_NULL_HANDLE;
		m_memory = memory;
		memory = VK_NULL_HANDLE;

		m_owner = &renderContext;
		m_desc = desc;

		return S_OK;
	}

	void Tr2BufferAL::Destroy()
	{
		if( m_buffer )
		{
			m_owner->DestroyLaterVulkan( m_buffer, &vkDestroyBuffer );
			m_buffer = VK_NULL_HANDLE;

			m_owner->DestroyLaterVulkan( m_memory, &vkFreeMemory );
			m_memory = VK_NULL_HANDLE;

			m_owner = nullptr;
			m_desc = Tr2BufferDescriptionAL();
		}
	}

	bool Tr2BufferAL::IsValid() const
	{
		return m_buffer != VK_NULL_HANDLE;
	}

	Tr2ALMemoryType Tr2BufferAL::GetMemoryClass() const
	{
		return AL_MEMORY_VIDEO;
	}

	const Tr2BufferDescriptionAL& Tr2BufferAL::GetDesc() const
	{
		return m_desc;
	}

	ALResult Tr2BufferAL::MapForReading( const void*& data, Tr2RenderContextAL& renderContext )
	{
		return E_NOTIMPL;
	}
	void Tr2BufferAL::UnmapForReading( Tr2RenderContextAL& renderContext )
	{
	}
	ALResult Tr2BufferAL::MapForWriting( void*& data, Tr2LockType::Type lockType, Tr2RenderContextAL& renderContext )
	{
		return E_NOTIMPL;
	}
	void Tr2BufferAL::UnmapForWriting( Tr2RenderContextAL& renderContext )
	{
	}

	ALResult Tr2BufferAL::UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext )
	{
		return E_NOTIMPL;;
	}

	void Tr2BufferAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2BufferAL";
		description["size"] = std::to_string( long long( GetDesc().count * GetDesc().stride ) );
	}

	ALResult Tr2BufferAL::SetName( const char* )
	{
		return E_NOTIMPL;
	}
}


#endif