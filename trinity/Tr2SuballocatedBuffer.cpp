////////////////////////////////////////////////////////////
//
//    Created:   October 2023
//    Copyright: CCP 2023
//

#include "StdAfx.h" 

#include "Tr2SuballocatedBuffer.h"



CCP_STATS_DECLARE( suballocatedBufferAllocated, "Trinity/SuballocatedBuffer/Allocated", false, CST_COUNTER_HIGH, "The currently number of bytes currently in use in the buffer." );
CCP_STATS_DECLARE( suballocatedBufferCapacity, "Trinity/SuballocatedBuffer/Capacity", false, CST_COUNTER_HIGH, "The buffer's current capacity." );








Tr2SuballocatedBuffer::Tr2SuballocatedBuffer( const char* name, const Tr2GpuUsage::Type gpuUsage, const uint32_t blockSize, const uint32_t maxSize ) :
	m_name( name ),
	m_gpuUsage( gpuUsage ),
	m_allocator( blockSize, maxSize, blockSize )
{
	CCP_STATS_SET( suballocatedBufferAllocated, m_allocator.GetAllocatedMemory() );
	CCP_STATS_SET( suballocatedBufferCapacity, m_allocator.GetMaxSize() );
}

ALResult Tr2SuballocatedBuffer::Allocate( uint32_t stride, uint32_t count, void* data, Tr2RenderContextAL& renderContext, Allocation& result )
{
	if( !m_buffer.IsValid() )
	{
		PrepareResources();
		if( !m_buffer.IsValid() )
		{
			return E_FAIL;
		}
	}
	size_t size = count * stride;

	Tr2VirtualAllocator::VirtualAllocation allocation = {};
	if( m_allocator.Allocate( size, stride, allocation ) )
	{
		result.m_offset = static_cast<uint32_t>(allocation.offset);
		result.m_size = static_cast<uint32_t>(size);

		result.m_stride = static_cast<uint32_t>(stride);

		result.m_allocation = allocation;
		result.m_parent = this;
		m_allocations.push_back( &result );

		CCP_ASSERT( result.m_offset % stride == 0 );

		if( data != nullptr )
		{
			result.Update( data, renderContext );
		}

		CCP_STATS_SET( suballocatedBufferAllocated, m_allocator.GetAllocatedMemory() );

		return S_OK;
	}

	FORWARD_HR( Expand() );

	return Allocate( stride, count, data, renderContext, result );
}

void Tr2SuballocatedBuffer::Free( Allocation& allocation )
{
	if( allocation.m_parent )
	{
		auto found = find( begin( m_allocations ), end( m_allocations ), &allocation );
		if( found != end( m_allocations ) )
		{
			m_allocator.Free( allocation.m_allocation );
			allocation.m_parent = nullptr;

			m_allocations.erase( found );
			CCP_STATS_SET( suballocatedBufferAllocated, m_allocator.GetAllocatedMemory() );
		}
		else
		{
			CCP_LOGERR( "Memory corruption in Tr2SuballocatedBuffer::Free()! Trying to free an allocation that has already been freed!" );
		}
	}
}

void Tr2SuballocatedBuffer::ReleaseResources( TriStorage s )
{
	if( ( s & TRISTORAGE_MANAGEDMEMORY ) != 0 )
	{
		m_buffer = Tr2BufferAL();

		for( auto& allocation : m_allocations )
		{
			m_allocator.Free( allocation->m_allocation );
			allocation->m_parent = nullptr;
		}

		m_allocations.clear();

		CCP_STATS_SET( suballocatedBufferAllocated, m_allocator.GetAllocatedMemory() );
	}
}

bool Tr2SuballocatedBuffer::OnPrepareResources()
{
	if( m_buffer.IsValid() )
	{
		return true;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();

	uint32_t bufferSize = static_cast<uint32_t>(m_allocator.GetCurrentSize());

	Tr2BufferAL buffer;
	Tr2BufferDescriptionAL desc( 1, bufferSize, m_gpuUsage, Tr2CpuUsage::READ | Tr2CpuUsage::WRITE );
	buffer.Create( desc, nullptr, renderContext );

	m_buffer = buffer;

	return true;
}

ALResult Tr2SuballocatedBuffer::Expand()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	//We have reached the allocator's max size. This probably means we're leaking a lot of memory.
	if (m_allocator.GetCurrentSize() == m_allocator.GetMaxSize())
	{
		CCP_LOGERR( "Max size reached for buffer '%s'. This probably means we're leaking a lot of memory.", m_name.c_str() );
		return E_OUTOFMEMORY;
	}

	uint32_t oldBufferSize = static_cast<uint32_t>( m_allocator.GetCurrentSize() );
	uint32_t newBufferSize = oldBufferSize + static_cast<uint32_t>( m_allocator.GetBlockSize() );

	Tr2BufferAL newBuffer;
	Tr2BufferDescriptionAL desc( 1, newBufferSize, m_gpuUsage, Tr2CpuUsage::READ | Tr2CpuUsage::WRITE );
	ALResult result = newBuffer.Create( desc, nullptr, renderContext );
	if( FAILED( result ) )
	{
		CCP_LOGERR( "Failed to allocate %zu MBs for buffer '%s'.", newBufferSize / size_t( 1024 * 1024 ), m_name.c_str() );
		return result;
	}
	newBuffer.SetName( m_name.c_str() );

	//If the previous buffer had valid data, we copy it over to the new buffer.
	if( m_buffer.IsValid() )
	{
		renderContext.CopySubBuffer( newBuffer, 0, m_buffer, 0, oldBufferSize );
	}

	m_buffer = newBuffer;

	m_allocator.Expand(); //Cannot fail, as we checked this first.

	CCP_ASSERT( m_allocator.GetCurrentSize() == newBufferSize );

	CCP_LOG( "Allocating more buffer memory for buffer '%s'. Total memory allocated: %zu MBs", m_name.c_str(), newBufferSize / size_t( 1024 * 1024 ) );
	CCP_STATS_SET( suballocatedBufferCapacity, m_allocator.GetMaxSize() );

	return S_OK;
}

ALResult Tr2SuballocatedBuffer::ReadBuffer( std::unique_ptr<uint8_t[]>& dest, uint32_t offset, uint32_t size, Tr2RenderContextAL& renderContext )
{
	const void* data = nullptr;
	CR_RETURN_HR( m_buffer.MapForReading( data, offset, size, renderContext ) );
	if ( data == nullptr )
	{
		return E_FAIL;
	}
	dest.reset( new uint8_t[size] );
	memcpy( dest.get(), data, size );
	m_buffer.UnmapForReading( renderContext );
	return S_OK;
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

void Tr2SuballocatedBuffer::Allocation::Update( const void* data, uint32_t offset, uint32_t size, Tr2RenderContextAL& renderContext )
{
	CCP_ASSERT_M( GetSize() - offset >= size, "Writeing data outside the allocated bounds of the suballocation buffer" );
	m_parent->m_buffer.UpdateBuffer( GetOffset() + offset, size, data, renderContext );
}

ALResult Tr2SuballocatedBuffer::Allocation::MapForReading( const void*& data, Tr2RenderContextAL& renderContext )
{
	CR_RETURN_HR( m_parent->ReadBuffer( m_mappedCopy, m_offset, m_size, renderContext ) );
	data = m_mappedCopy.get();
	return S_OK;
}


void Tr2SuballocatedBuffer::Allocation::UnmapForReading( Tr2RenderContextAL& renderContext )
{
	m_mappedCopy.reset();
}
