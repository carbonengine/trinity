#include "StdAfx.h"

#include "Tr2VirtualAllocator.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_VULKAN_VERSION 1000000
#define VMA_STATS_STRING_ENABLED 0 
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

Tr2VirtualAllocator::Tr2VirtualAllocator( size_t size ) :
	m_size( size )
{

	VmaVirtualBlockCreateInfo info = {};
	info.size = size;
	info.flags = 0;

	vmaCreateVirtualBlock( &info, (VmaVirtualBlock*)&block );
}

void Tr2VirtualAllocator::Free()
{
	vmaDestroyVirtualBlock( (VmaVirtualBlock) block );
}

bool Tr2VirtualAllocator::Allocate( size_t size, size_t alignment, VirtualAllocation& result )
{
	VmaVirtualAllocationCreateInfo info;
	info.size = size;
	info.alignment = alignment;
	info.flags = VMA_VIRTUAL_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;

	VmaVirtualAllocation allocation;
	VkDeviceSize offset;
	if( vmaVirtualAllocate( (VmaVirtualBlock)block, &info, &allocation, &offset ) != VK_SUCCESS )
	{
		return false;
	}

	result.allocation = allocation;
	result.offset = offset;
	result.size = size;

	return true;
}

void Tr2VirtualAllocator::Free( VirtualAllocation allocation )
{
	vmaVirtualFree( (VmaVirtualBlock)block, (VmaVirtualAllocation)allocation.allocation );
}

size_t Tr2VirtualAllocator::GetSize() const
{
	return m_size;
}