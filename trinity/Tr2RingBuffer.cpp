// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2RingBufferImpl.h"


Tr2MorphTargetAnimationData::Tr2MorphTargetAnimationData( uint32_t index, float weight )
{
	m_index = index;
	m_weight = weight;
}

void Tr2RingBuffer::PrepareBuffer( Tr2RenderContext& renderContext )
{
	if( m_gpuBufferStale )
	{
		CreateGpuBuffer( renderContext.GetPrimaryRenderContext() );
	}
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
	m_gpuBufferStale = true;
}

void Tr2RingBuffer::CreateGpuBuffer( Tr2PrimaryRenderContextAL& renderContext )
{
	m_buffer.Create( m_stride, m_size, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE_OFTEN | Tr2CpuUsage::NON_SYNCRONIZED_WRITE, nullptr, renderContext );
	m_buffer.SetName( m_name.c_str() );
	m_gpuBufferStale = false;
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
		m_gpuBufferStale = false;
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

template uint32_t Tr2RingBuffer::UploadTransforms<Tr2ChildBoosterInstanceData>( const Tr2ChildBoosterInstanceData* data, uint32_t dataCount );
template Tr2RingBuffer& Tr2RingBuffer::GetInstance<Tr2ChildBoosterInstanceData>();
template void Tr2RingBufferOffsets::UploadTransforms<Tr2ChildBoosterInstanceData>( Tr2RingBuffer& buffer, const Tr2ChildBoosterInstanceData* instances, uint32_t count );
