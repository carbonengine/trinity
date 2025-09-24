#pragma once

#include "include/ITr2GpuBuffer.h"
#include "Tr2DeviceResource.h"


BLUE_CLASS( Tr2MorphTargetAnimationDataBuffer ) :
	public ITr2GpuBuffer,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	struct AnimationData
	{
		AnimationData() = default;
		explicit AnimationData( uint32_t index, float weight );

		uint32_t m_index;
		float m_weight;
	};

	uint32_t UploadTransforms( const AnimationData* data, uint32_t matrixCount );
	void PrepareBuffer( Tr2RenderContext& renderContext );
	void SetFrameNumbers( uint64_t recordingFrame, uint64_t completedFrame );

	Tr2BufferAL* GetGpuBuffer( unsigned index ) override;

	static Tr2MorphTargetAnimationDataBuffer& GetInstance();

private:
	void ReleaseResources( TriStorage s ) override;
	bool OnPrepareResources() override;

	void Resize( uint32_t size );

	std::mutex m_mutex;
	Tr2BufferAL m_buffer;
	std::vector<AnimationData> m_mirror;

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

TYPEDEF_BLUECLASS( Tr2MorphTargetAnimationDataBuffer );


class Tr2MorphTargetAnimationDataOffsets
{
public:
	uint32_t GetCurrentFrameOffset() const;
	uint32_t GetPreviousFrameOffset() const;

	void UploadTransforms( Tr2MorphTargetAnimationDataBuffer& buffer, const Tr2MorphTargetAnimationDataBuffer::AnimationData* transforms, uint32_t count );
	void AdvanceFrame();

	static const uint32_t INVALID_OFFSET = 0xffffffff;
private:

	uint32_t m_currentFrameOffset = INVALID_OFFSET;
	uint32_t m_previousFrameOffset = INVALID_OFFSET;
};
