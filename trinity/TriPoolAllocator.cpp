#include "StdAfx.h"
#include "TriPoolAllocator.h"

TriPoolAllocator::TriPoolAllocator() :
	m_totalBytesAllocated( 0 ),
	m_totalSystemMemoryAllocated( 0 ),
	m_pool( NULL ),
	m_poolEnd( NULL ),
	m_poolCurrent( NULL ),
	m_chunkSize( 256*1024 )
{
}

TriPoolAllocator::~TriPoolAllocator()
{
	for( ChunkList_t::const_iterator it = m_previousPools.begin(); it != m_previousPools.end(); ++it )
	{
		CCP_FREE( *it );
	}
	CCP_FREE( m_pool );
	m_pool = m_poolCurrent = m_poolEnd = NULL;
}

void* TriPoolAllocator::Allocate( size_t size )
{
	// Align size to 16 bytes - pool starts out 16byte aligned - this ensures
	// that all allocations are always 16 byte aligned.
	size = CCP_ALIGN( size, 16 );

	if( m_poolCurrent + size > m_poolEnd )
	{
		GetMoreSystemMemory( size );
		if( m_poolCurrent + size > m_poolEnd )
		{
			return NULL;
		}
	}

	void* ret = m_poolCurrent;

	m_poolCurrent += size;
	m_totalBytesAllocated += size;

	return ret;
}

void TriPoolAllocator::Clear()
{
	if( !m_previousPools.empty() )
	{
		for( ChunkList_t::const_iterator it = m_previousPools.begin(); it != m_previousPools.end(); ++it )
		{
			// doing the explicit iterator cast allows you to debug this more easily
			uint8_t* previousPool = *it;
			CCP_FREE( previousPool );
		}
		m_previousPools.clear();
	}

	size_t curChunkSize = m_poolEnd - m_pool;
	if( m_totalBytesAllocated < curChunkSize / 2 )
	{
		// Pool is too large - free it and shrink the chunk size
		CCP_FREE( m_pool );
		m_pool = m_poolCurrent = m_poolEnd = NULL;
		m_chunkSize = curChunkSize / 2;
		m_chunkSize >>= 8;
		m_chunkSize += 1;
		m_chunkSize <<= 8;
		m_totalSystemMemoryAllocated = 0;
	}
	else if( m_totalBytesAllocated > m_chunkSize )
	{
		// Pool is too small - free it and grow the chunk size
		CCP_FREE( m_pool );
		m_pool = m_poolCurrent = m_poolEnd = NULL;
		m_chunkSize = m_totalBytesAllocated;
		m_chunkSize >>= 8;
		m_chunkSize += 1;
		m_chunkSize <<= 8;
		m_totalSystemMemoryAllocated = 0;
	}
	else
	{
		// Pool seems to be of the right size.
		// Align pool to 16 bytes - size of allocations are also aligned to
		// 16 bytes - this ensures that all allocations are always 16 byte aligned.
		m_poolCurrent = (uint8_t*)( CCP_ALIGN( (uintptr_t)m_pool, 16 ) );
		m_totalSystemMemoryAllocated = curChunkSize;
	}

	m_totalBytesAllocated = 0;
}

void TriPoolAllocator::GetMoreSystemMemory( size_t size )
{
	if( m_pool )
	{
		m_previousPools.push_back( m_pool );
	}

	size_t sizeToRequest = m_chunkSize;
	while( sizeToRequest < size )
	{
		sizeToRequest += m_chunkSize;
	}

	// Align pool to 16 bytes - size of allocations are also aligned to
	// 16 bytes - this ensures that all allocations are always 16 byte aligned.
	m_pool = (uint8_t*)CCP_MALLOC( "TriPoolAllocator/chunk", sizeToRequest );

	if( m_pool )
	{
		m_poolEnd = m_pool + sizeToRequest;
		m_poolCurrent = (uint8_t*)( CCP_ALIGN( (uintptr_t)m_pool, 16 ) );

		m_totalSystemMemoryAllocated += sizeToRequest;
	}
	else
	{
		m_poolCurrent = m_poolEnd = NULL;
	}
}
