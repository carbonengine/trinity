// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TRIPOOLALLOCATOR_H
#define TRIPOOLALLOCATOR_H

#include <atomic>
#include <mutex>
#include <vector>

// See http://core/wiki/TriPoolAllocator
// Allocate is safe to call from several threads at once; Clear is not.

class TriPoolAllocator
{
public:
	TriPoolAllocator();
	TriPoolAllocator( TriPoolAllocator&& other ) noexcept;
	TriPoolAllocator& operator=( TriPoolAllocator&& other ) noexcept;
	~TriPoolAllocator();

	// Allocates 'size' bytes, aligned to 16 bytes
	void* Allocate( size_t size );

	// Allocates an object of type T, aligned to 16 bytes
	template <class T>
	T* Allocate()
	{
		void* p = Allocate( sizeof( T ) );
		if( !p )
		{
			return NULL;
		}
		T* ret = new( p ) T;

		return ret;
	}

	// Clears the allocator, resetting to initial state
	void Clear();

private:
	struct Chunk
	{
		uint8_t* memory;
		uint8_t* end;
		size_t size;
		std::atomic<uint8_t*> current;
	};

	Chunk* AddChunk( size_t size );
	static size_t UsedBytes( const Chunk& chunk );
	static void FreeChunk( Chunk* chunk );

private:
	size_t m_totalSystemMemoryAllocated;

	size_t m_chunkSize;

	std::atomic<Chunk*> m_chunk;
	std::vector<Chunk*> m_chunks;
	std::mutex m_mutex;
};


#endif
