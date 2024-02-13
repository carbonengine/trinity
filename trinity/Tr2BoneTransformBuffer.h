#pragma once

#include "include/ITr2GpuBuffer.h"
#include "Tr2DeviceResource.h"


BLUE_CLASS( Tr2BoneTransformBuffer ) :
	public ITr2GpuBuffer,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	struct Float4x3
	{
		float elements[12];
	};

	uint32_t UploadTransforms( const Float4x3* data, uint32_t matrixCount );
	void PrepareBuffer( Tr2RenderContext& renderContext );
	void SetFrameNumbers( uint64_t recordingFrame, uint64_t completedFrame );

	Tr2BufferAL* GetGpuBuffer( unsigned index ) override;

	static Tr2BoneTransformBuffer& GetInstance();
private:
	void ReleaseResources( TriStorage s ) override;
	bool OnPrepareResources() override;

	void Resize( uint32_t size );

	Tr2BufferAL m_buffer;
	std::vector<Float4x3> m_mirror;

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

TYPEDEF_BLUECLASS( Tr2BoneTransformBuffer );


class Tr2BoneTransformOffsets
{
public:
	uint32_t GetCurrentFrameOffset() const;
	uint32_t GetPreviousFrameOffset() const;

	void UploadTransforms( Tr2BoneTransformBuffer& buffer, const Tr2BoneTransformBuffer::Float4x3* transforms, uint32_t count );
	void AdvanceFrame();

private:
	static const uint32_t INVALID_OFFSET = 0xffffffff;

	uint32_t m_currentFrameOffset = INVALID_OFFSET;
	uint32_t m_previousFrameOffset = INVALID_OFFSET;
};
