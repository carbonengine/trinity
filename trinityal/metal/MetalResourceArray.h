// Copyright © 2023 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_METAL

#include <Metal/Metal.h>

namespace TrinityALImpl
{

class ResourceArrayArgumentBuffer
{
public:
	using GpuResourceID = uint64_t;

	void Initialize( uint32_t initialSize, id<MTLDevice> device );
	operator bool() const;

	uint32_t Allocate( GpuResourceID gpuResourceID );
	void Deallocate( uint32_t );

	id<MTLBuffer> GetBuffer() const;

	void SetFrameIndices( uint64_t recordingFrame, uint64_t renderedFrame );

private:
	void Grow( uint32_t newSize );

	id<MTLBuffer> m_buffer;
	std::vector<GpuResourceID> m_data;
	std::vector<uint32_t> m_free;
	std::vector<std::pair<uint32_t, uint64_t>> m_pendingFree;
	uint64_t m_recordingFrame = 0;
	uint32_t m_size = 0;
	id<MTLDevice> m_device;
};

}

#endif
