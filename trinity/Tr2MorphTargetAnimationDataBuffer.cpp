#include "StdAfx.h"
#include "Tr2MorphTargetAnimationDataBuffer.h"

namespace
{
constexpr uint32_t INITIAL_SIZE = 16 * 1024;
}

Tr2MorphTargetAnimationDataBuffer::AnimationData::AnimationData( uint32_t index, float weight )
{
	m_index = index;
	m_weight = weight;
}


uint32_t Tr2MorphTargetAnimationDataBuffer::UploadTransforms( const AnimationData* data, uint32_t dataCount )
{
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
		CCP_LOG( "Resizing morph target animation data buffer to %uKB", uint32_t( m_size * 2 * sizeof( AnimationData ) ) / 1024 );
		Resize( m_size * 2 );
	}

	std::copy( data, data + dataCount, m_mirror.begin() + m_head );

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

void Tr2MorphTargetAnimationDataBuffer::PrepareBuffer( Tr2RenderContext& renderContext )
{
	for( auto& region : m_dirtyRegions )
	{
		if( region.size )
		{
			m_buffer.UpdateBuffer( region.offset * sizeof( AnimationData ), region.size * sizeof( AnimationData ), m_mirror.data() + region.offset, renderContext );
			m_lockedRegions.push_back( { m_frame, region.offset + region.size } );
		}
	}
	m_dirtyRegions[0] = { m_head, 0 };
	m_dirtyRegions[1] = {};
}

Tr2BufferAL* Tr2MorphTargetAnimationDataBuffer::GetGpuBuffer( unsigned )
{
	return &m_buffer;
}

void Tr2MorphTargetAnimationDataBuffer::SetFrameNumbers( uint64_t recordingFrame, uint64_t completedFrame )
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

void Tr2MorphTargetAnimationDataBuffer::Resize( uint32_t size )
{
	m_dirtyRegions[0] = { 0, m_size };
	m_dirtyRegions[1] = {};
	m_lockedRegions.clear();

	m_mirror.resize( size );
	m_head = m_size;
	m_size = size;
	m_tail = m_size;

	USE_MAIN_THREAD_RENDER_CONTEXT();
	m_buffer.Create( sizeof( AnimationData ), size, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE_OFTEN | Tr2CpuUsage::NON_SYNCRONIZED_WRITE, nullptr, renderContext );
	m_buffer.SetName( "Bone transforms" );
}

Tr2MorphTargetAnimationDataBuffer& Tr2MorphTargetAnimationDataBuffer::GetInstance()
{
	static Tr2MorphTargetAnimationDataBuffer* self = nullptr;
	if( !self )
	{
		self = new CTr2MorphTargetAnimationDataBuffer();

		USE_MAIN_THREAD_RENDER_CONTEXT();
		self->SetFrameNumbers( renderContext.GetRecordingFrameNumber(), renderContext.GetRenderedFrameNumber() );
		self->Resize( INITIAL_SIZE );
	}

	return *self;
}

void Tr2MorphTargetAnimationDataBuffer::ReleaseResources( TriStorage )
{
}

bool Tr2MorphTargetAnimationDataBuffer::OnPrepareResources()
{
	if( !m_mirror.empty() && !m_buffer.IsValid() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		m_buffer.Create( sizeof( AnimationData ), m_size, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE_OFTEN | Tr2CpuUsage::NON_SYNCRONIZED_WRITE, m_mirror.data(), renderContext );
		m_buffer.SetName( "Bone transforms" );
	}
	return true;
}



uint32_t Tr2MorphTargetAnimationDataOffsets::GetCurrentFrameOffset() const
{
	return m_currentFrameOffset;
}

uint32_t Tr2MorphTargetAnimationDataOffsets::GetPreviousFrameOffset() const
{
	return m_previousFrameOffset;
}

void Tr2MorphTargetAnimationDataOffsets::UploadTransforms( Tr2MorphTargetAnimationDataBuffer& buffer, const Tr2MorphTargetAnimationDataBuffer::AnimationData* transforms, uint32_t count )
{
	if( m_currentFrameOffset != INVALID_OFFSET )
	{
		return;
	}
	m_currentFrameOffset = buffer.UploadTransforms( transforms, count );
	if( m_previousFrameOffset == INVALID_OFFSET )
	{
		m_previousFrameOffset = m_currentFrameOffset;
	}
}

void Tr2MorphTargetAnimationDataOffsets::AdvanceFrame()
{
	m_previousFrameOffset = m_currentFrameOffset;
	m_currentFrameOffset = 0xffffffff;
}
