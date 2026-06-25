// Copyright © 2023 CCP ehf.

#pragma once

#ifndef Tr2VirtualBlock_H
#define Tr2VirtualBlock_H

#include <vector>

// --------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------
class Tr2VirtualAllocator
{

public:
	struct VirtualAllocation
	{
		void* allocation;
		size_t offset;
		size_t size;
	};

	Tr2VirtualAllocator( size_t blockSize, size_t maxSize, size_t initialSize );
	~Tr2VirtualAllocator();

	Tr2VirtualAllocator( const Tr2VirtualAllocator& obj ) = delete;
	void operator=( const Tr2VirtualAllocator& ) = delete;

	bool Expand();

	bool Allocate( size_t size, size_t alignment, VirtualAllocation& result );
	void Free( VirtualAllocation allocation );


	size_t GetBlockSize() const;
	size_t GetMaxSize() const;
	size_t GetCurrentSize() const;
	size_t GetAllocatedMemory() const;

private:
	const size_t m_blockSize;
	const size_t m_maxSize;
	size_t m_currentSize;

	size_t m_allocatedMemory;

	std::vector<VirtualAllocation> m_reservedBlocks;

	void* block;
};

#endif