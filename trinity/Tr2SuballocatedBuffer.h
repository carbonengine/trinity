// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2SuballocatedBuffer_H
#define Tr2SuballocatedBuffer_H

#include "include/ITr2GpuBuffer.h"

#include "Tr2DeviceResource.h"
#include "Tr2VirtualAllocator.h"

// --------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------
class Tr2SuballocatedBuffer : public Tr2DeviceResource
{
public:
	Tr2SuballocatedBuffer( const char* name, const Tr2GpuUsage::Type gpuUsage, const uint32_t blockSize, const uint32_t maxSize );

	Tr2SuballocatedBuffer( const Tr2SuballocatedBuffer& obj ) = delete;
	void operator=( const Tr2SuballocatedBuffer& ) = delete;


	struct Allocation
	{
		friend class Tr2SuballocatedBuffer;

	public:
		Allocation() = default;

		~Allocation();

		Allocation( const Allocation& ) = delete;
		Allocation& operator=( Allocation& ) = delete;

		Tr2BufferAL& GetBuffer() const;
		uint32_t GetOffset() const;
		uint32_t GetSize() const;
		uint32_t GetStride() const;
		uint32_t GetStartIndex() const;
		bool IsValid() const;

		void Update( const void* data, Tr2RenderContextAL& renderContext );

		void Update( const void* data, uint32_t offset, uint32_t size, Tr2RenderContextAL& renderContext );



		template <typename T>
		ALResult MapForReading( const T*& data, Tr2RenderContextAL& renderContext )
		{
			return MapForReading( *reinterpret_cast<const void**>( &data ), renderContext );
		}
		ALResult MapForReading( const void*& data, Tr2RenderContextAL& renderContext );

		void UnmapForReading( Tr2RenderContextAL& renderContext );

	private:
		Tr2VirtualAllocator::VirtualAllocation m_allocation = {};
		std::unique_ptr<uint8_t[]> m_mappedCopy;

		uint32_t m_offset = 0;
		uint32_t m_size = 0;
		uint32_t m_stride = 0;

		Tr2SuballocatedBuffer* m_parent = nullptr;
	};

	ALResult Allocate( uint32_t stride, uint32_t count, void* data, Tr2RenderContextAL& renderContext, Allocation& result );

	void Free( Allocation& allocation );

	void ReleaseResources( TriStorage ) override;

	const Tr2BufferAL& GetBuffer() const;

protected:
	bool OnPrepareResources();

private:
	const std::string m_name;
	const Tr2GpuUsage::Type m_gpuUsage;


	Tr2BufferAL m_buffer;

	Tr2VirtualAllocator m_allocator;
	std::vector<Allocation*> m_allocations;


	ALResult Expand();

	ALResult ReadBuffer( std::unique_ptr<uint8_t[]>& dest, uint32_t offset, uint32_t size, Tr2RenderContextAL& renderContext );
};

#endif
