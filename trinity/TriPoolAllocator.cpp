// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriPoolAllocator.h"

namespace
{
size_t RoundChunkSize( size_t size )
{
	size >>= 8;
	size += 1;
	size <<= 8;
	return size;
}
}

TriPoolAllocator::TriPoolAllocator() :
	m_totalSystemMemoryAllocated( 0 ),
	m_chunkSize( 256 * 1024 ),
	m_chunk( nullptr )
{
}

TriPoolAllocator::TriPoolAllocator( TriPoolAllocator&& other ) noexcept :
	m_totalSystemMemoryAllocated( other.m_totalSystemMemoryAllocated ),
	m_chunkSize( other.m_chunkSize ),
	m_chunk( other.m_chunk.load() ),
	m_chunks( std::move( other.m_chunks ) )
{
	other.m_chunk = nullptr;
	other.m_chunks.clear();
	other.m_totalSystemMemoryAllocated = 0;
}

TriPoolAllocator& TriPoolAllocator::operator=( TriPoolAllocator&& other ) noexcept
{
	if( this != &other )
	{
		for( Chunk* chunk : m_chunks )
		{
			FreeChunk( chunk );
		}
		m_totalSystemMemoryAllocated = other.m_totalSystemMemoryAllocated;
		m_chunkSize = other.m_chunkSize;
		m_chunk = other.m_chunk.load();
		m_chunks = std::move( other.m_chunks );
		other.m_chunk = nullptr;
		other.m_chunks.clear();
		other.m_totalSystemMemoryAllocated = 0;
	}
	return *this;
}

TriPoolAllocator::~TriPoolAllocator()
{
	for( Chunk* chunk : m_chunks )
	{
		FreeChunk( chunk );
	}
	m_chunks.clear();
	m_chunk = nullptr;
}

void* TriPoolAllocator::Allocate( size_t size )
{
	// Align size to 16 bytes - chunks start out 16byte aligned - this ensures
	// that all allocations are always 16 byte aligned.
	size = CCP_ALIGN( size, 16 );

	for( ;; )
	{
		Chunk* chunk = m_chunk.load( std::memory_order_acquire );
		if( chunk )
		{
			uint8_t* p = chunk->current.fetch_add( size, std::memory_order_relaxed );
			if( p + size <= chunk->end )
			{
				return p;
			}
		}

		std::lock_guard lock( m_mutex );
		if( m_chunk.load( std::memory_order_relaxed ) == chunk && !AddChunk( size ) )
		{
			return NULL;
		}
	}
}

void TriPoolAllocator::Clear()
{
	size_t totalBytesAllocated = 0;
	for( Chunk* chunk : m_chunks )
	{
		totalBytesAllocated += UsedBytes( *chunk );
	}

	Chunk* last = m_chunks.empty() ? nullptr : m_chunks.back();
	for( Chunk* chunk : m_chunks )
	{
		if( chunk != last )
		{
			FreeChunk( chunk );
		}
	}
	m_chunks.clear();
	m_chunk = nullptr;
	m_totalSystemMemoryAllocated = 0;

	if( !last )
	{
		return;
	}
	if( totalBytesAllocated < last->size / 2 )
	{
		// Pool is too large - free it and shrink the chunk size
		m_chunkSize = RoundChunkSize( last->size / 2 );
		FreeChunk( last );
	}
	else if( totalBytesAllocated > m_chunkSize )
	{
		// Pool is too small - free it and grow the chunk size
		m_chunkSize = RoundChunkSize( totalBytesAllocated );
		FreeChunk( last );
	}
	else
	{
		// Pool seems to be of the right size.
		last->current = (uint8_t*)( CCP_ALIGN( (uintptr_t)last->memory, 16 ) );
		m_chunks.push_back( last );
		m_chunk = last;
		m_totalSystemMemoryAllocated = last->size;
	}
}

TriPoolAllocator::Chunk* TriPoolAllocator::AddChunk( size_t size )
{
	size_t sizeToRequest = m_chunkSize;
	while( sizeToRequest < size + 16 )
	{
		sizeToRequest += m_chunkSize;
	}

	uint8_t* memory = (uint8_t*)CCP_MALLOC( "TriPoolAllocator/chunk", sizeToRequest );
	if( !memory )
	{
		return nullptr;
	}
	Chunk* chunk = new Chunk;
	chunk->memory = memory;
	chunk->end = memory + sizeToRequest;
	chunk->size = sizeToRequest;
	chunk->current = (uint8_t*)( CCP_ALIGN( (uintptr_t)memory, 16 ) );
	m_chunks.push_back( chunk );
	m_totalSystemMemoryAllocated += sizeToRequest;
	m_chunk.store( chunk, std::memory_order_release );
	return chunk;
}

size_t TriPoolAllocator::UsedBytes( const Chunk& chunk )
{
	uint8_t* current = chunk.current.load( std::memory_order_relaxed );
	return size_t( std::min( current, chunk.end ) - chunk.memory );
}

void TriPoolAllocator::FreeChunk( Chunk* chunk )
{
	CCP_FREE( chunk->memory );
	delete chunk;
}
