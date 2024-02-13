#include "StdAfx.h"
#include "Tr2BoneTransformBuffer.h"

namespace
{
constexpr uint32_t INITIAL_SIZE = 16 * 1024;
}

uint32_t Tr2BoneTransformBuffer::UploadTransforms( const Float4x3* data, uint32_t matrixCount )
{
	if( m_head >= m_tail )
	{
		if( m_head + matrixCount > m_size )
		{
			m_head = 0;
		}
	}
	if( m_head <= m_tail && m_head + matrixCount >= m_tail )
	{
		CCP_LOG( "Resizing bone transform buffer to %uKB", ( m_size * 2 * sizeof( Float4x3 ) ) / 1024 );
		Resize( m_size * 2 );
	}

	std::copy( data, data + matrixCount, m_mirror.begin() + m_head );

	if( m_dirtyRegions[0].offset + m_dirtyRegions[0].size == m_head )
	{
		m_dirtyRegions[0].size += matrixCount;
	}
	else if( m_dirtyRegions[1].offset + m_dirtyRegions[1].size == m_head )
	{
		m_dirtyRegions[1].size += matrixCount;
	}
	else
	{
		CCP_ASSERT( false );
	}

	auto result = m_head;
	m_head += matrixCount;
	return result;
}

void Tr2BoneTransformBuffer::PrepareBuffer( Tr2RenderContext& renderContext )
{
	for( auto& region : m_dirtyRegions )
	{
		if( region.size )
		{
			m_buffer.UpdateBuffer( region.offset * sizeof( Float4x3 ), region.size * sizeof( Float4x3 ), m_mirror.data() + region.offset, renderContext );
			m_lockedRegions.push_back( { m_frame, region.offset + region.size } );
		}
	}
	m_dirtyRegions[0] = { m_head, 0 };
	m_dirtyRegions[1] = {};
}

Tr2BufferAL* Tr2BoneTransformBuffer::GetGpuBuffer( unsigned )
{
	return &m_buffer;
}

void Tr2BoneTransformBuffer::SetFrameNumbers( uint64_t recordingFrame, uint64_t completedFrame )
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

void Tr2BoneTransformBuffer::Resize( uint32_t size )
{
	m_dirtyRegions[0] = { 0, m_size };
	m_dirtyRegions[1] = {};
	m_lockedRegions.clear();

	m_mirror.resize( size );
	m_head = m_size;
	m_size = size;
	m_tail = m_size;

	USE_MAIN_THREAD_RENDER_CONTEXT();
	m_buffer.Create( sizeof( Float4x3 ), size, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE_OFTEN | Tr2CpuUsage::NON_SYNCRONIZED_WRITE, nullptr, renderContext );
	m_buffer.SetName( "Bone transforms" );
}

Tr2BoneTransformBuffer& Tr2BoneTransformBuffer::GetInstance()
{
	static Tr2BoneTransformBuffer* self = nullptr;
	if( !self )
	{
		self = new CTr2BoneTransformBuffer();

		USE_MAIN_THREAD_RENDER_CONTEXT();
		self->SetFrameNumbers( renderContext.GetRecordingFrameNumber(), renderContext.GetRenderedFrameNumber() );
		self->Resize( INITIAL_SIZE );
	}

	return *self;
}

void Tr2BoneTransformBuffer::ReleaseResources( TriStorage )
{
}

bool Tr2BoneTransformBuffer::OnPrepareResources()
{
	if( !m_mirror.empty() && !m_buffer.IsValid() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		m_buffer.Create( sizeof( Float4x3 ), m_size, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE_OFTEN | Tr2CpuUsage::NON_SYNCRONIZED_WRITE, m_mirror.data(), renderContext );
		m_buffer.SetName( "Bone transforms" );
	}
	return true;
}



uint32_t Tr2BoneTransformOffsets::GetCurrentFrameOffset() const
{
	return m_currentFrameOffset;
}

uint32_t Tr2BoneTransformOffsets::GetPreviousFrameOffset() const
{
	return m_previousFrameOffset;
}

void Tr2BoneTransformOffsets::UploadTransforms( Tr2BoneTransformBuffer& buffer, const Tr2BoneTransformBuffer::Float4x3* transforms, uint32_t count )
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

void Tr2BoneTransformOffsets::AdvanceFrame()
{
	m_previousFrameOffset = m_currentFrameOffset;
	m_currentFrameOffset = 0xffffffff;
}
