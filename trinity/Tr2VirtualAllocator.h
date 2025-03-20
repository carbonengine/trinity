#pragma once
#ifndef Tr2VirtualBlock_H
#define Tr2VirtualBlock_H

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

	Tr2VirtualAllocator( size_t size );
	void Free();

	bool Allocate( size_t size, size_t alignment, VirtualAllocation& result );
	void Free( VirtualAllocation allocation );

	size_t GetSize() const;

private:
	void* block;
	const size_t m_size;
};

#endif