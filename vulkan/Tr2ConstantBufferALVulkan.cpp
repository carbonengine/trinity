////////////////////////////////////////////////////////////
//
//    Created:   August 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "Tr2ConstantBufferALVulkan.h"
#include "Tr2PrimaryRenderContextVulkan.h"
#include "UtilitiesVulkan.h"

namespace TrinityALImpl
{

	Tr2ConstantBufferAL::Tr2ConstantBufferAL()
		:m_buffer( VK_NULL_HANDLE ),
		m_memory( VK_NULL_HANDLE ),
		m_owner( nullptr ),
		m_size( 0 )
	{
	}

	Tr2ConstantBufferAL::~Tr2ConstantBufferAL()
	{
		Destroy();
	}

	ALResult Tr2ConstantBufferAL::Create( uint32_t size, Tr2ConstantUsageAL::Type usage, const void* initialData, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( size == 0 )
		{
			return E_INVALIDARG;
		}

		if( !renderContext.IsValid() )
		{
			return E_INVALIDCALL;
		}

		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		FORWARD_HR( TrinityALImpl::CreateBuffer( buffer, memory, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VkMemoryPropertyFlagBits( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ), renderContext ) );
		ON_BLOCK_EXIT( [&] {
			if( buffer ) renderContext.DestroyLaterVulkan( buffer, &vkDestroyBuffer );
			if( memory ) renderContext.DestroyLaterVulkan( memory, &vkFreeMemory );
			} );

		if( initialData )
		{
			void* mapped;
			CR_RETURN_HR( Vk2Al( vkMapMemory( renderContext.m_device, memory, 0, size, 0, &mapped ) ) );
			memcpy( mapped, initialData, size );

			VkMappedMemoryRange flushRange = {
				VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
				nullptr,
				memory,
				0,
				VK_WHOLE_SIZE
			};
			vkFlushMappedMemoryRanges( renderContext.m_device, 1, &flushRange );
			vkUnmapMemory( renderContext.m_device, memory );
		}

		m_buffer = buffer;
		buffer = VK_NULL_HANDLE;
		m_memory = memory;
		memory = VK_NULL_HANDLE;

		m_owner = &renderContext;
		m_size = size;

		return S_OK;
	}

	ALResult Tr2ConstantBufferAL::Lock( void** data, Tr2RenderContextAL& )
	{
		if( !IsValid() )
		{
			return E_INVALIDCALL;
		}
		return Vk2Al( vkMapMemory( m_owner->m_device, m_memory, 0, VK_WHOLE_SIZE, 0, data ) );
	}

	ALResult Tr2ConstantBufferAL::Unlock( Tr2RenderContextAL& renderContext )
	{
		if( !IsValid() )
		{
			return E_INVALIDCALL;
		}
		VkMappedMemoryRange flushRange = {
			VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
			nullptr,
			m_memory,
			0,
			VK_WHOLE_SIZE
		};
		vkFlushMappedMemoryRanges( m_owner->m_device, 1, &flushRange );
		vkUnmapMemory( m_owner->m_device, m_memory );
		return S_OK;
	}

	bool Tr2ConstantBufferAL::IsValid() const
	{
		return m_owner != nullptr;
	}

	void Tr2ConstantBufferAL::Destroy()
	{
		if( m_owner )
		{
			m_owner->DestroyLaterVulkan( m_buffer, &vkDestroyBuffer );
			m_buffer = VK_NULL_HANDLE;

			m_owner->DestroyLaterVulkan( m_memory, &vkFreeMemory );
			m_memory = VK_NULL_HANDLE;

			m_owner = nullptr;
			m_size = 0;
		}
	}

	Tr2ConstantUsageAL::Type Tr2ConstantBufferAL::GetUsage() const
	{
		return Tr2ConstantUsageAL::REUSABLE;
	}

	uint32_t Tr2ConstantBufferAL::GetSize() const
	{
		return m_size;
	}

	Tr2ALMemoryType Tr2ConstantBufferAL::GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	void Tr2ConstantBufferAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2ConstantBufferAL";
	}

	ALResult Tr2ConstantBufferAL::SetName( const char* )
	{
		return E_NOTIMPL;
	}
}

#endif