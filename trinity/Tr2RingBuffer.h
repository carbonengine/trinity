// Copyright © 2026 CCP ehf.

#pragma once

#include "include/ITr2GpuBuffer.h"
#include "Tr2DeviceResource.h"
#include "Utilities/MatrixUtils.h"


struct Tr2MorphTargetAnimationData
{
	Tr2MorphTargetAnimationData() = default;
	Tr2MorphTargetAnimationData( uint32_t index, float weight );

	uint32_t m_index;
	float m_weight;
};

BLUE_CLASS( Tr2RingBuffer ) :
	public ITr2GpuBuffer,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	template <typename T>
	uint32_t UploadTransforms( const T* data, uint32_t matrixCount );
	void PrepareBuffer( Tr2RenderContext & renderContext );
	void SetFrameNumbers( uint64_t recordingFrame, uint64_t completedFrame );

	Tr2BufferAL* GetGpuBuffer( unsigned index ) override;

	template <typename T>
	static Tr2RingBuffer& GetInstance();

	void SetName( const std::string& name );

private:
	void ReleaseResources( TriStorage s ) override;
	bool OnPrepareResources() override;

	void Resize( uint32_t size );

	std::string m_name;

	uint32_t m_stride;
	std::mutex m_mutex;
	Tr2BufferAL m_buffer;
	std::vector<uint8_t> m_mirror;

	uint64_t m_frame = 0;
	uint32_t m_head = 0;
	uint32_t m_tail = 0;
	uint32_t m_size = 0;

	struct DirtyRegion
	{
		uint32_t offset = 0;
		uint32_t size = 0;
	};
	DirtyRegion m_dirtyRegions[2];

	struct LockedRegion
	{
		uint64_t frame;
		uint32_t tail;
	};

	std::vector<LockedRegion> m_lockedRegions;
};

TYPEDEF_BLUECLASS( Tr2RingBuffer );


class Tr2RingBufferOffsets
{
public:
	uint32_t GetCurrentFrameOffset() const;
	uint32_t GetPreviousFrameOffset() const;

	template <typename T>
	void UploadTransforms( Tr2RingBuffer& buffer, const T* transforms, uint32_t count );
	void AdvanceFrame();

	static const uint32_t INVALID_OFFSET = 0xffffffff;

private:
	uint32_t m_currentFrameOffset = INVALID_OFFSET;
	uint32_t m_previousFrameOffset = INVALID_OFFSET;
};
