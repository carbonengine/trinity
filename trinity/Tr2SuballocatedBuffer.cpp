////////////////////////////////////////////////////////////
//
//    Created:   October 2023
//    Copyright: CCP 2023
//

#include "StdAfx.h" 

#include "Tr2SuballocatedBuffer.h"

#include "Tr2SyncToGpu.h"

Tr2SuballocatedBuffer::Tr2SuballocatedBuffer( const char* name, const Tr2GpuUsage::Type gpuUsage, const uint32_t blockSize ) :
	m_name( name ),
	m_gpuUsage( gpuUsage ),
	m_blockSize( blockSize )
{
}

Tr2SuballocatedBuffer::~Tr2SuballocatedBuffer()
{
	for( uint32_t i = 0; i < m_allocators.size(); i++ )
	{
		m_allocators[i].Free();
	}
}

ALResult Tr2SuballocatedBuffer::Allocate( size_t stride, size_t count, void* data, Tr2RenderContextAL& renderContext, Allocation& result )
{
	if( !m_buffer.IsValid() && !m_allocators.empty() )
	{
		PrepareResources();
		if( !m_buffer.IsValid() )
		{
			return E_FAIL;
		}
	}
	size_t size = count * stride;
	size_t paddedSize = size;
	size_t alignment = stride;
	if( m_blockSize % stride != 0 )
	{
		// Each suballocator has their own local 0 offset.
		// This makes alignment management quite complicated if the block size is not divisible by the stride.
		// We need to consveratively allocate and manually align the allocation in this case.
		paddedSize += stride;
		alignment = 1;
	}


	if( paddedSize > m_blockSize )
	{
		//We cannot allocate a single allocation that is larger than a single block.
		//If this is required, we'll need to increase the block size, which wastes more memory.
		return E_FAIL;
	}

	for( uint32_t i = 0; i < m_allocators.size(); i++ )
	{
		auto allocator = m_allocators[i];

		Tr2VirtualAllocator::VirtualAllocation allocation = {};
		if( !allocator.Allocate( paddedSize, alignment, allocation ) )
		{
			continue; //This allocators is full, try the next one!
		}

		auto Align = []( size_t offset, size_t alignment ) {
			return ( offset + ( alignment - 1 ) ) / alignment * alignment;
		};

		result.m_allocatorIndex = i;
		result.m_offset = static_cast<uint32_t>( Align( static_cast<size_t>( i ) * m_blockSize + allocation.offset, stride ) );
		result.m_size = static_cast<uint32_t>( size ); //Make sure to save the unpadded size.

		result.m_stride = static_cast<uint32_t>( stride );

		result.m_allocation = allocation;
		result.m_parent = this;
		m_allocations.push_back( &result );

		CCP_ASSERT( result.m_offset % stride == 0 );

		if( data != nullptr )
		{
			result.Update( data, renderContext );
		}

		return S_OK;
	}
	//All allocators are full! Expand the buffer and try again!
	Expand();
	return Allocate( stride, count, data, renderContext, result );
}

void Tr2SuballocatedBuffer::Free( Allocation& allocation )
{
	if( allocation.m_parent )
	{
		SyncToGpu( [allocator = m_allocators[allocation.m_allocatorIndex], alloc = allocation.m_allocation]() {
			Tr2VirtualAllocator( allocator ).Free( alloc );
		} );
		allocation.m_parent = nullptr;

		auto found = find( begin( m_allocations ), end( m_allocations ), &allocation );
		if( found != end( m_allocations ) )
		{
			m_allocations.erase( found );
		}
	}
}

void Tr2SuballocatedBuffer::ReleaseResources( TriStorage s )
{
	if( ( s & TRISTORAGE_MANAGEDMEMORY ) != 0 )
	{
		m_buffer = Tr2BufferAL();

		auto allocations = m_allocations;
		for( auto& allocation : allocations )
		{
			Free( *allocation );
		}
	}
}

bool Tr2SuballocatedBuffer::OnPrepareResources()
{
	if( m_buffer.IsValid() )
	{
		return true;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();

	uint32_t bufferSize = m_blockSize * static_cast<uint32_t>( m_allocators.size() );

	Tr2BufferAL buffer;
	Tr2BufferDescriptionAL desc( 1, bufferSize, m_gpuUsage, Tr2CpuUsage::READ | Tr2CpuUsage::WRITE );
	buffer.Create( desc, nullptr, renderContext );

	m_buffer = buffer;

	return true;
}

void Tr2SuballocatedBuffer::Expand()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	uint32_t oldBufferSize = m_blockSize * static_cast<uint32_t>( m_allocators.size() );

	Tr2VirtualAllocator allocator( m_blockSize );
	m_allocators.push_back( allocator );

	uint32_t newBufferSize = m_blockSize * static_cast<uint32_t>( m_allocators.size() );

	Tr2BufferAL newBuffer;
	Tr2BufferDescriptionAL desc( 1, newBufferSize, m_gpuUsage, Tr2CpuUsage::READ | Tr2CpuUsage::WRITE );
	newBuffer.Create( desc, nullptr, renderContext );
	newBuffer.SetName( m_name.c_str() );

	if( m_buffer.IsValid() )
	{
		renderContext.CopySubBuffer( newBuffer, 0, m_buffer, 0, oldBufferSize );
	}

	m_buffer = newBuffer;

	size_t BYTES_TO_MEGABYTES = size_t( 1024 ) * 1024;
	CCP_LOGNOTICE( "Allocating more buffer memory for buffer '%s'. Total memory allocated: %zu MBs", m_name.c_str(), m_blockSize * m_allocators.size() / BYTES_TO_MEGABYTES );
}

ALResult Tr2SuballocatedBuffer::MapForReading( Tr2RenderContextAL& renderContext )
{
	if( m_mapCounter == 0 )
	{
		CR_RETURN_HR( m_buffer.MapForReading( m_mappedPointer, renderContext ) );
	}
	m_mapCounter++;

	CCP_ASSERT( m_mapCounter < 10 ); //Sanity check to make sure we're not leaking mappings. We should never need more than 2 realistically.

	return S_OK;
}

void Tr2SuballocatedBuffer::UnmapForReading( Tr2RenderContextAL& renderContext )
{
	CCP_ASSERT( m_mapCounter > 0 );

	if( --m_mapCounter == 0 )
	{
		m_buffer.UnmapForReading( renderContext );
		m_mappedPointer = nullptr;
	}
}



Tr2SuballocatedBuffer::Allocation::~Allocation()
{
	if( m_parent )
	{
		m_parent->Free( *this );
	}
}

Tr2BufferAL& Tr2SuballocatedBuffer::Allocation::GetBuffer() const
{
	return m_parent->m_buffer;
}

uint32_t Tr2SuballocatedBuffer::Allocation::GetOffset() const
{
	return m_offset;
}

uint32_t Tr2SuballocatedBuffer::Allocation::GetSize() const
{
	return m_size;
}

uint32_t Tr2SuballocatedBuffer::Allocation::GetStride() const
{
	return m_stride;
}

uint32_t Tr2SuballocatedBuffer::Allocation::GetStartIndex() const
{
	return m_offset / m_stride;
}

bool Tr2SuballocatedBuffer::Allocation::IsValid() const
{
	return m_parent != nullptr;
}

void Tr2SuballocatedBuffer::Allocation::Update( const void* data, Tr2RenderContextAL& renderContext )
{
	m_parent->m_buffer.UpdateBuffer( GetOffset(), GetSize(), data, renderContext );
}

ALResult Tr2SuballocatedBuffer::Allocation::MapForReading( const void*& data, Tr2RenderContextAL& renderContext )
{
	ALResult result = m_parent->MapForReading( renderContext );
	if( !FAILED( result ) )
	{
		data = (void*)( m_parent->m_mappedPointer + m_offset );
	}
	return result;
}


void Tr2SuballocatedBuffer::Allocation::UnmapForReading( Tr2RenderContextAL& renderContext )
{
	m_parent->UnmapForReading( renderContext );
}
