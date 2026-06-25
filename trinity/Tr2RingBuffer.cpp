// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2RingBuffer.h"

namespace
{
constexpr uint32_t INITIAL_SIZE = 16 * 1024;
}


Tr2MorphTargetAnimationData::Tr2MorphTargetAnimationData( uint32_t index, float weight )
{
	m_index = index;
	m_weight = weight;
}

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
		CCP_LOG( "Resizing Tr2RingBuffer %s to %uKB", m_name.c_str(), uint32_t( m_size * 2 * m_stride ) / 1024 );
		Resize( m_size * 2 );
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

void Tr2RingBuffer::PrepareBuffer( Tr2RenderContext& renderContext )
{
	for( auto& region : m_dirtyRegions )
	{
		if( region.size )
		{
			m_buffer.UpdateBuffer( region.offset * m_stride, region.size * m_stride, m_mirror.data() + region.offset * m_stride, renderContext );
			m_lockedRegions.push_back( { m_frame, region.offset + region.size } );
		}
	}
	m_dirtyRegions[0] = { m_head, 0 };
	m_dirtyRegions[1] = {};
}

Tr2BufferAL* Tr2RingBuffer::GetGpuBuffer( unsigned )
{
	return &m_buffer;
}

void Tr2RingBuffer::SetFrameNumbers( uint64_t recordingFrame, uint64_t completedFrame )
{
	m_frame = recordingFrame;
	completedFrame = std::min( completedFrame, recordingFrame - 2 );

	for( auto it = begin( m_lockedRegions ); it != end( m_lockedRegions ); ++it )
	{
		if( it->frame <= completedFrame )
		{
			m_tail = it->tail;
		}
		else
		{
			m_lockedRegions.erase( begin( m_lockedRegions ), it );
			break;
		}
	}
}

void Tr2RingBuffer::Resize( uint32_t size )
{
	m_dirtyRegions[0] = { 0, m_size };
	m_dirtyRegions[1] = {};
	m_lockedRegions.clear();

	m_mirror.resize( size * m_stride );
	m_head = m_size;
	m_size = size;
	m_tail = m_size;

	USE_MAIN_THREAD_RENDER_CONTEXT();
	m_buffer.Create( m_stride, size, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE_OFTEN | Tr2CpuUsage::NON_SYNCRONIZED_WRITE, nullptr, renderContext );
	m_buffer.SetName( m_name.c_str() );
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
	}

	CCP_ASSERT_M( sizeof( T ) == self->m_stride, "Stride has to match size of datatype!" );

	return *self;
}

void Tr2RingBuffer::SetName( const std::string& name )
{
	this->m_name = name;
	if( m_buffer.IsValid() )
	{
		m_buffer.SetName( m_name.c_str() );
	}
}

void Tr2RingBuffer::ReleaseResources( TriStorage )
{
}

bool Tr2RingBuffer::OnPrepareResources()
{
	if( !m_mirror.empty() && !m_buffer.IsValid() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		m_buffer.Create( m_stride, m_size, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE_OFTEN | Tr2CpuUsage::NON_SYNCRONIZED_WRITE, m_mirror.data(), renderContext );
		m_buffer.SetName( m_name.c_str() );
	}
	return true;
}



uint32_t Tr2RingBufferOffsets::GetCurrentFrameOffset() const
{
	return m_currentFrameOffset;
}

uint32_t Tr2RingBufferOffsets::GetPreviousFrameOffset() const
{
	return m_previousFrameOffset;
}

template <typename T>
void Tr2RingBufferOffsets::UploadTransforms( Tr2RingBuffer& buffer, const T* transforms, uint32_t count )
{
	if( m_currentFrameOffset != INVALID_OFFSET )
	{
		return;
	}
	m_currentFrameOffset = buffer.UploadTransforms<T>( transforms, count );
	if( m_previousFrameOffset == INVALID_OFFSET )
	{
		m_previousFrameOffset = m_currentFrameOffset;
	}
}

void Tr2RingBufferOffsets::AdvanceFrame()
{
	m_previousFrameOffset = m_currentFrameOffset;
	m_currentFrameOffset = 0xffffffff;
}

template uint32_t Tr2RingBuffer::UploadTransforms<Tr2MorphTargetAnimationData>( const Tr2MorphTargetAnimationData* data, uint32_t dataCount );
template Tr2RingBuffer& Tr2RingBuffer::GetInstance<Tr2MorphTargetAnimationData>();
template void Tr2RingBufferOffsets::UploadTransforms<Tr2MorphTargetAnimationData>( Tr2RingBuffer& buffer, const Tr2MorphTargetAnimationData* transforms, uint32_t count );

template uint32_t Tr2RingBuffer::UploadTransforms<Float4x3>( const Float4x3* data, uint32_t dataCount );
template Tr2RingBuffer& Tr2RingBuffer::GetInstance<Float4x3>();
template void Tr2RingBufferOffsets::UploadTransforms<Float4x3>( Tr2RingBuffer& buffer, const Float4x3* transforms, uint32_t count );