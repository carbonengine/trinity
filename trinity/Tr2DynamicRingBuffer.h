// Copyright © 2014 CCP ehf.

#pragma once
#ifndef Tr2DynamicRingBuffer_H
#define Tr2DynamicRingBuffer_H

// --------------------------------------------------------------------------------------
// Description:
//   A base class for index/vertex ring buffers. Ring buffers are used in case when
//   dynamic data needs to be passed multiple times per frame for rendering and is
//   discared right after rendering. With a ring buffer we can avoid excessive discard
//   locks one would need for use. The basic patter of using this class is:
//   1. collect data for the buffer>\
//   2. call ringBuffer.PutData to put data into the buffer
//   3. draw primitives using the ring buffer
//   4. call ringBuffer.DoneUsingData once the data is no longer needed
//   5. repeat
//  For an example of use see UI.
// See Also:
//   Tr2Sprite2dScene, Tr2StreamVertexBuffer, Tr2StreamIndexBuffer
// --------------------------------------------------------------------------------------
class Tr2DynamicRingBuffer : public Tr2DeviceResource
{
public:
	Tr2DynamicRingBuffer();
	~Tr2DynamicRingBuffer();

	ALResult PutData(
		const void* data,
		uint32_t size,
		uint32_t& offset,
		Tr2RenderContext& renderContext );
	ALResult PutData(
		const void* data,
		uint32_t size,
		uint32_t alignment,
		uint32_t& offset,
		Tr2RenderContext& renderContext );
	void DoneUsingData( Tr2RenderContext& renderContext );

	void SetSizeIncrement( uint32_t sizeIncrement );

	uint32_t GetBufferSize() const;

	bool IsValid() const;
	Tr2BufferAL& GetBuffer();

	void SetName( const char* name );

	//////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );

protected:
	virtual ALResult CreateBuffer( uint32_t size ) = 0;
	ALResult UpdateBuffer( const void* data, uint32_t offset, uint32_t size, Tr2RenderContext& renderContext );
	virtual bool OnPrepareResources();

	// Size of buffer in bytes
	uint32_t m_bufferSize;

private:
	// Represents a continuous region in buffer used by GPU
	struct BufferRegion
	{
		// Offset into the buffer
		uint32_t offset;
		// Length of the region in bytes
		uint32_t length;
		// Fence used to checking if the region is still in use by GPU
		Tr2FenceAL* fence;
	};
	typedef TrackableStdVector<BufferRegion> RegionVector;
	typedef TrackableStdVector<Tr2FenceAL*> FenceVector;

	bool IsRegionUsedByGpu( BufferRegion& region, Tr2RenderContext& renderContext );
	void TrimUnusedRegions( Tr2RenderContext& renderContext );
	bool GetUnusedRegion( uint32_t minSize, uint32_t& offset );
	Tr2FenceAL* AllocateFence();
	void DeallocateFence( Tr2FenceAL* fence );
	void RemoveRegions( RegionVector::iterator begin, RegionVector::iterator end );

	// Buffer regions used by GPU
	RegionVector m_regions;
	// How much to grow the buffer if there run out of space
	uint32_t m_sizeIncrement;
	// Did the last PutData succeeded
	bool m_lastPutSucceeded;

	// Fence objects available for reuse
	FenceVector m_availableFences;

protected:
	Tr2BufferAL m_buffer;
	std::string m_name;
};

// --------------------------------------------------------------------------------------
// Description:
//   A ring vertex buffer. Maintains an vertex buffer for dynamic ring access.
// See Also:
//   Tr2DynamicRingBuffer
// --------------------------------------------------------------------------------------
class Tr2RingVertexBuffer : public Tr2DynamicRingBuffer
{
public:
	bool Create( uint32_t bufferSize );

protected:
	virtual ALResult CreateBuffer( uint32_t size );
};

// --------------------------------------------------------------------------------------
// Description:
//   A ring index buffer. Maintains an index buffer for dynamic ring access.
// See Also:
//   Tr2DynamicRingBuffer
// --------------------------------------------------------------------------------------
class Tr2RingIndexBuffer : public Tr2DynamicRingBuffer
{
public:
	Tr2RingIndexBuffer();

	bool Create( uint32_t numberOfIndices, uint32_t indexSize );

protected:
	virtual ALResult CreateBuffer( uint32_t size );

private:
	uint32_t m_indexSize;
};

#endif