// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "Tr2VirtualAllocator.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_VULKAN_VERSION 1000000
#define VMA_STATS_STRING_ENABLED 0
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

Tr2VirtualAllocator::Tr2VirtualAllocator( size_t blockSize, size_t maxSize, size_t initialSize ) :
	m_blockSize( blockSize ),
	m_maxSize( maxSize ),
	m_currentSize( 0 ),
	m_allocatedMemory( 0 )
{
	size_t blockSizeBits = blockSize - 1;
	CCP_ASSERT_M( ( blockSize & blockSizeBits ) == 0, "blockSize must be a power of two!" );
	CCP_ASSERT_M( ( maxSize & blockSizeBits ) == 0, "maxSize must a multiple of blockSize" );
	CCP_ASSERT_M( ( initialSize & blockSizeBits ) == 0, "initialSize must a multiple of blockSize" );

	CCP_ASSERT_M( initialSize <= maxSize, "initialSize must be less than or equal to blockSize" );

	/*
	Virtual blocks are not resizable. To facilitate resizing, create a virtual block of the
	maximum possible size and fill it with dummy memory allocations (called reservations),
	which we can then free to "resize" the allocator to a larger size in increments of the block size.
	*/

	VmaVirtualBlockCreateInfo info = {};
	info.size = maxSize;
	info.flags = 0;

	vmaCreateVirtualBlock( &info, (VmaVirtualBlock*)&block );

	//Fill the entire buffer with reservations.
	size_t numReservedBlocks = maxSize / blockSize;
	for( size_t i = 0; i < numReservedBlocks; i++ )
	{
		VirtualAllocation reservedBlock;
		Allocate( blockSize, 1, reservedBlock );
		m_reservedBlocks.push_back( reservedBlock );
	}

	//Reverse the order of them, so that the one at offset=0 is at the end of the vector.
	//This lets us pop a reserved block from the end of the list, which is faster than removing the head.
	std::reverse( m_reservedBlocks.begin(), m_reservedBlocks.end() );

	//Free reserved blocks until we reach the required initial size.
	while( m_currentSize < initialSize )
	{
		Expand();
	}
}

Tr2VirtualAllocator::~Tr2VirtualAllocator()
{
	vmaClearVirtualBlock( (VmaVirtualBlock)block );
	vmaDestroyVirtualBlock( (VmaVirtualBlock)block );
}

bool Tr2VirtualAllocator::Expand()
{
	if( m_reservedBlocks.empty() )
	{
		//We have reached the maximum size.
		return false;
	}

	//Free the lowest offset reserved block to grow the allocatable range
	m_currentSize += m_blockSize;
	Free( m_reservedBlocks.back() );
	m_reservedBlocks.pop_back();

	return true;
}

bool Tr2VirtualAllocator::Allocate( size_t size, size_t alignment, VirtualAllocation& result )
{
	VmaVirtualAllocationCreateInfo info = {};
	info.size = size;
	info.alignment = alignment;
	info.flags = VMA_VIRTUAL_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;

	if( ( alignment & ( alignment - 1 ) ) != 0 )
	{
		//The alignment is not a power of two. VMA does not like non-power-of-two alignments, so pad it and align it manually later.
		info.size += alignment - 1;
		info.alignment = 1;
	}

	VmaVirtualAllocation allocation;
	VkDeviceSize offset;
	if( vmaVirtualAllocate( (VmaVirtualBlock)block, &info, &allocation, &offset ) != VK_SUCCESS )
	{
		return false;
	}

	offset = ( offset + alignment - 1 ) / alignment * alignment;

	result.allocation = allocation;
	result.offset = offset;
	result.size = size;

	m_allocatedMemory += size;
	CCP_LOG( "Allocated %zu bytes at offset %zu with alignment %zu from %p. Memory usage: %f / %zu MBs ", size, result.offset, alignment, block, (float)GetAllocatedMemory() / float( 1024 * 1024 ), m_currentSize / size_t( 1024 * 1024 ) );

	return true;
}

void Tr2VirtualAllocator::Free( VirtualAllocation allocation )
{
	vmaVirtualFree( (VmaVirtualBlock)block, (VmaVirtualAllocation)allocation.allocation );

	m_allocatedMemory -= allocation.size;
	CCP_LOG( "Freed %zu bytes at offset %zu from %p. Memory usage: %f / %zu MBs", allocation.size, allocation.offset, block, (float)GetAllocatedMemory() / float( 1024 * 1024 ), m_currentSize / size_t( 1024 * 1024 ) );
}

size_t Tr2VirtualAllocator::GetBlockSize() const
{
	return m_blockSize;
}

size_t Tr2VirtualAllocator::GetMaxSize() const
{
	return m_maxSize;
}

size_t Tr2VirtualAllocator::GetCurrentSize() const
{
	return m_currentSize;
}

size_t Tr2VirtualAllocator::GetAllocatedMemory() const
{
	//m_allocatedMemory includes the reserved blocks, so subtract the size of the reservations.
	return m_allocatedMemory - m_reservedBlocks.size() * m_blockSize;
}