// Copyright © 2014 CCP ehf.

#include "StdAfx.h"
#include "Tr2DynamicRingBuffer.h"
#include "TriSettingsRegistrar.h"

using namespace Tr2RenderContextEnum;

namespace
{
uint32_t Align( uint32_t offset, uint32_t alignment )
{
	return ( offset + alignment - 1 ) / alignment * alignment;
}
}

Tr2DynamicRingBuffer::Tr2DynamicRingBuffer() :
	m_regions( "Tr2DynamicRingBuffer::m_regions" ),
	m_sizeIncrement( 0 ),
	m_lastPutSucceeded( false ),
	m_bufferSize( 0 ),
	m_availableFences( "Tr2DynamicRingBuffer::m_availableFences" )
{
}

Tr2DynamicRingBuffer::~Tr2DynamicRingBuffer()
{
	ReleaseResources( TRISTORAGE_ALL );
}

// --------------------------------------------------------------------------------------
// Description:
//   Puts new data into the buffer. If the data size is larget than the buffer size, the
//   buffer is resized.
// Arguments:
//   data - Pointer to data
//   size - Size of data in bytes
//   bufferOffset - (out) offset into the buffer in bytes where the data will reside
//   renderContext - current render context
// Return Value:
//   success code
// --------------------------------------------------------------------------------------
ALResult Tr2DynamicRingBuffer::PutData(
	const void* data,
	uint32_t size,
	uint32_t& offset,
	Tr2RenderContext& renderContext )
{
	return PutData( data, size, 4, offset, renderContext );
}

ALResult Tr2DynamicRingBuffer::PutData(
	const void* data,
	uint32_t size,
	uint32_t alignment,
	uint32_t& bufferOffset,
	Tr2RenderContext& renderContext )
{
	m_lastPutSucceeded = false;
	TrimUnusedRegions( renderContext );

	if( !size )
	{
		return S_OK;
	}
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}

	auto allocationSize = size + alignment;
	uint32_t allocationOffset;

	if( !GetUnusedRegion( allocationSize, allocationOffset ) )
	{
		CCP_STATS_ZONE( "Tr2DynamicRingBuffer full buffer lock" );
		allocationOffset = 0;
		RemoveRegions( m_regions.begin(), m_regions.end() );
		if( m_bufferSize < allocationSize + m_sizeIncrement )
		{
			CR_RETURN_HR( CreateBuffer( allocationSize + m_sizeIncrement ) );
			if( !m_name.empty() )
			{
				m_buffer.SetName( m_name.c_str() );
			}
			m_bufferSize = allocationSize + m_sizeIncrement;
		}
		else if( m_sizeIncrement )
		{
			CR_RETURN_HR( CreateBuffer( m_bufferSize + m_sizeIncrement ) );
			if( !m_name.empty() )
			{
				m_buffer.SetName( m_name.c_str() );
			}
			m_bufferSize += m_sizeIncrement;
		}
		else
		{
			CR_RETURN_HR( CreateBuffer( m_bufferSize * 2 ) );
			if( !m_name.empty() )
			{
				m_buffer.SetName( m_name.c_str() );
			}
			m_bufferSize *= 2;
		}
	}

	bufferOffset = Align( allocationOffset, alignment );

	CR_RETURN_HR( UpdateBuffer( data, bufferOffset, size, renderContext ) );

	BufferRegion region;
	region.offset = allocationOffset;
	region.length = allocationSize;
	region.fence = AllocateFence();
	m_regions.push_back( region );

	m_lastPutSucceeded = true;

	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Notifies the object that the data updated with the previous call to PutData is no
//   longer needed.
// Arguments:
//   renderContext - current render context
// --------------------------------------------------------------------------------------
void Tr2DynamicRingBuffer::DoneUsingData( Tr2RenderContext& renderContext )
{
	if( m_lastPutSucceeded )
	{
		if( !m_regions.empty() && m_regions.back().fence && FAILED( m_regions.back().fence->PutFence( renderContext ) ) )
		{
			DeallocateFence( m_regions.back().fence );
			m_regions.back().fence = nullptr;
		}
		m_lastPutSucceeded = false;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if the given region is still used by GPU.
// Arguments:
//   region - Buffer region
//   renderContext - current render context
// Return value:
//   true If the region is still in use
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2DynamicRingBuffer::IsRegionUsedByGpu( BufferRegion& region, Tr2RenderContext& renderContext )
{
	if( !region.fence )
	{
		return false;
	}

	bool isReached = false;
	return FAILED( region.fence->IsReached( isReached, renderContext ) ) || !isReached;
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes regions no longer used by GPU from m_regions vector.
// Arguments:
//   renderContext - current render context
// --------------------------------------------------------------------------------------
void Tr2DynamicRingBuffer::TrimUnusedRegions( Tr2RenderContext& renderContext )
{
	auto used = m_regions.begin();
	while( used != m_regions.end() )
	{
		if( IsRegionUsedByGpu( *used, renderContext ) )
		{
			break;
		}
		++used;
	}
	RemoveRegions( m_regions.begin(), used );
}

// --------------------------------------------------------------------------------------
// Description:
//   Finds the next unused region in the buffer.
// Arguments:
//   minSize - Minimum size of the region
//   offset - (out) Offset into the buffer of the region start
// Return value:
//   true If the buffer contains an unused region of at least minSize bytes
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2DynamicRingBuffer::GetUnusedRegion( uint32_t minSize, uint32_t& offset )
{
	uint32_t totalSize = m_bufferSize;

	if( m_regions.empty() )
	{
		offset = 0;
		return minSize <= totalSize;
	}

	offset = m_regions.back().offset + m_regions.back().length;
	uint32_t endOffset = m_regions.front().offset;
	if( endOffset < offset )
	{
		if( minSize < totalSize - offset )
		{
			return true;
		}
		else if( minSize < endOffset )
		{
			offset = 0;
			return true;
		}
		return false;
	}
	else
	{
		return endOffset >= offset + minSize;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource interface. Releases GPU resources.
// --------------------------------------------------------------------------------------
void Tr2DynamicRingBuffer::ReleaseResources( TriStorage )
{
	m_buffer = Tr2BufferAL();
	RemoveRegions( m_regions.begin(), m_regions.end() );
	for( auto it = m_availableFences.begin(); it != m_availableFences.end(); ++it )
	{
		delete *it;
	}
	m_availableFences.clear();
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements Tr2DeviceResource interface.
// Return value:
//   true Always
// --------------------------------------------------------------------------------------
bool Tr2DynamicRingBuffer::OnPrepareResources()
{
	if( !m_bufferSize )
	{
		return true;
	}
	return SUCCEEDED( CreateBuffer( m_bufferSize ) );
}

// --------------------------------------------------------------------------------------
// Description:
//   Either returns a cached unused fence or creates a new one.
// Return value:
//   A fence that can be used
// --------------------------------------------------------------------------------------
Tr2FenceAL* Tr2DynamicRingBuffer::AllocateFence()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( m_availableFences.empty() )
	{
		Tr2FenceAL* fence = CCP_NEW( "Tr2DynamicRingBuffer::fence" ) Tr2FenceAL;
		fence->Create( renderContext );
		return fence;
	}
	else
	{
		Tr2FenceAL* fence = m_availableFences.back();
		m_availableFences.pop_back();
		CCP_ASSERT( fence );
		return fence;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Recycles a fence.
// Arguments:
//   fence - A fence to be recycled
// --------------------------------------------------------------------------------------
void Tr2DynamicRingBuffer::DeallocateFence( Tr2FenceAL* fence )
{
	if( fence )
	{
		m_availableFences.push_back( fence );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes region records from m_region vector and recycles its fences.
// Arguments:
//   begin - Iterator to the beginning of sequence to be removed
//   end - Iterator to the end of sequence to be removed
// --------------------------------------------------------------------------------------
void Tr2DynamicRingBuffer::RemoveRegions( RegionVector::iterator begin, RegionVector::iterator end )
{
	for( auto it = begin; it != end; ++it )
	{
		DeallocateFence( it->fence );
	}
	m_regions.erase( begin, end );
}

// --------------------------------------------------------------------------------------
// Description:
//   Changes the size of the buffer increments when rewriging the buffer.
// Arguments:
//   sizeIncrement - Buffer size increment in bytes
// --------------------------------------------------------------------------------------
void Tr2DynamicRingBuffer::SetSizeIncrement( uint32_t sizeIncrement )
{
	m_sizeIncrement = sizeIncrement;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the current size of the buffer in bytes.
// --------------------------------------------------------------------------------------
uint32_t Tr2DynamicRingBuffer::GetBufferSize() const
{
	return m_bufferSize;
}

void Tr2DynamicRingBuffer::SetName( const char* name )
{
	m_name = name;
	if( m_buffer.IsValid() )
	{
		m_buffer.SetName( name );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Check if the buffer is valid.
// Return Value:
//   true If the buffer is valid
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2DynamicRingBuffer::IsValid() const
{
	return m_buffer.IsValid();
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the buffer.
// Return Value:
//   Vertex buffer
// --------------------------------------------------------------------------------------
Tr2BufferAL& Tr2DynamicRingBuffer::GetBuffer()
{
	return m_buffer;
}

// --------------------------------------------------------------------------------------
// Description:
//   Updates portion of the buffer with new data.
// Arguments:
//   data - Pointer to data
//   offset - Offset into the buffer in bytes where the data needs to be stored
//   size - Size of the data in bytes
//   renderContext - Current render context
// Return Value:
//   AL result code
// --------------------------------------------------------------------------------------
ALResult Tr2DynamicRingBuffer::UpdateBuffer(
	const void* data,
	uint32_t offset,
	uint32_t size,
	Tr2RenderContext& renderContext )
{
	uint8_t* bufferData = nullptr;
	CR_RETURN_HR( m_buffer.MapForWriting( bufferData, renderContext ) );
	if( !bufferData )
	{
		return E_FAIL;
	}
	memcpy( bufferData + offset, data, size );
	m_buffer.UnmapForWriting( renderContext );
	return S_OK;
}


// --------------------------------------------------------------------------------------
// Description:
//   Creates a new ring vertex buffer.
// Arguments:
//   bufferSize - Vertex buffer size in bytes
// Return Value:
//   true If the buffer was successfully created
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2RingVertexBuffer::Create( uint32_t bufferSize )
{
	ReleaseResources( TRISTORAGE_ALL );
	m_bufferSize = bufferSize;
	return PrepareResources();
}

// --------------------------------------------------------------------------------------
// Description:
//   Creates a vertex buffer.
// Arguments:
//   size - Size of the buffer in bytes
// Return Value:
//   AL result code
// --------------------------------------------------------------------------------------
ALResult Tr2RingVertexBuffer::CreateBuffer( uint32_t size )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return m_buffer.Create(
		1,
		size,
		Tr2GpuUsage::VERTEX_BUFFER,
		Tr2CpuUsage::WRITE_OFTEN | Tr2CpuUsage::NON_SYNCRONIZED_WRITE,
		nullptr,
		renderContext );
}


Tr2RingIndexBuffer::Tr2RingIndexBuffer() :
	m_indexSize( 4 )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Creates a new ring index buffer.
// Arguments:
//   numberOfIndices - Number of indices in the buffer
//   indexSize - Index size in bytes (2 or 4)
// Return Value:
//   true If the buffer was successfully created
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2RingIndexBuffer::Create( uint32_t numberOfIndices, uint32_t indexSize )
{
	CCP_ASSERT( indexSize == 2 || indexSize == 4 );
	ReleaseResources( TRISTORAGE_ALL );
	m_indexSize = indexSize;
	m_bufferSize = numberOfIndices * m_indexSize;
	return PrepareResources();
}

// --------------------------------------------------------------------------------------
// Description:
//   Creates an index buffer.
// Arguments:
//   size - Size of the buffer in bytes
// Return Value:
//   AL result code
// --------------------------------------------------------------------------------------
ALResult Tr2RingIndexBuffer::CreateBuffer( uint32_t size )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return m_buffer.Create(
		m_indexSize,
		size / m_indexSize,
		Tr2GpuUsage::INDEX_BUFFER,
		Tr2CpuUsage::WRITE_OFTEN | Tr2CpuUsage::NON_SYNCRONIZED_WRITE,
		nullptr,
		renderContext );
}
