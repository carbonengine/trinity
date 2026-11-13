// Copyright © 2026 CCP ehf.

#pragma once

// Definitions of the Tr2RingBuffer member templates. Include this in the .cpp file that uses a ring buffer for its own
// data type, so that the type can live with the code that uses it rather than in Tr2RingBuffer.h. Tr2RingBuffer.cpp
// explicitly instantiates them for the types declared in Tr2RingBuffer.h.

#include "Tr2RingBuffer.h"

template <typename T>
uint32_t Tr2RingBuffer::UploadTransforms( const T* data, uint32_t dataCount )
{
	CCP_ASSERT_M( sizeof( T ) == m_stride, "Stride has to match size of datatype!" );

	std::unique_lock lock( m_mutex );
	if( m_head >= m_tail )
	{
		if( m_head + dataCount > m_size )
		{
			m_head = 0;
		}
	}
	if( m_head < m_tail && m_head + dataCount >= m_tail )
	{
		// Resize moves the head to the old end of the buffer, so grow by at least dataCount
		auto newSize = std::max( m_size * 2, m_size + dataCount );
		CCP_LOG( "Resizing Tr2RingBuffer %s to %uKB", m_name.c_str(), uint32_t( newSize * m_stride ) / 1024 );
		Resize( newSize );
	}

	memcpy( m_mirror.data() + m_head * m_stride, data, dataCount * m_stride );

	if( m_dirtyRegions[0].offset + m_dirtyRegions[0].size == m_head )
	{
		m_dirtyRegions[0].size += dataCount;
	}
	else if( m_dirtyRegions[1].offset + m_dirtyRegions[1].size == m_head )
	{
		m_dirtyRegions[1].size += dataCount;
	}
	else
	{
		CCP_ASSERT( false );
	}

	auto result = m_head;
	m_head += dataCount;
	return result;
}

template <typename T>
Tr2RingBuffer& Tr2RingBuffer::GetInstance()
{
	static Tr2RingBuffer* self = nullptr;
	if( !self )
	{
		self = new CTr2RingBuffer();

		USE_MAIN_THREAD_RENDER_CONTEXT();
		self->m_stride = sizeof( T );
		self->SetFrameNumbers( renderContext.GetRecordingFrameNumber(), renderContext.GetRenderedFrameNumber() );
		self->Resize( INITIAL_SIZE );
		self->CreateGpuBuffer( renderContext );
	}

	CCP_ASSERT_M( sizeof( T ) == self->m_stride, "Stride has to match size of datatype!" );

	return *self;
}
