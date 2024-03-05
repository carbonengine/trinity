#pragma once
#ifndef TRIPOOLALLOCATOR_H
#define TRIPOOLALLOCATOR_H

// See http://core/wiki/TriPoolAllocator

class TriPoolAllocator
{
public:
	TriPoolAllocator();
	~TriPoolAllocator();

	// Allocates 'size' bytes, aligned to 16 bytes
	void* Allocate( size_t size );

	// Allocates an object of type T, aligned to 16 bytes
	template < class T >
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
	void GetMoreSystemMemory( size_t size );

private:
	size_t m_totalBytesAllocated;
	size_t m_totalSystemMemoryAllocated;

	size_t m_chunkSize;

	uint8_t* m_pool;
	uint8_t* m_poolEnd;
	uint8_t* m_poolCurrent;

	typedef std::list<uint8_t*> ChunkList_t;
	ChunkList_t m_previousPools;

};


#endif