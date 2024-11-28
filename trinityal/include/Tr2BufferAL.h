#pragma once

#include "../Tr2DeviceResourceAL.h"
#include "../ALResult.h"
#include "../Tr2RenderContextEnum.h"

class Tr2PrimaryRenderContextAL;
class Tr2RenderContextAL;

namespace TrinityALImpl
{
	class Tr2BufferAL;
	class Tr2ResourceSetAL;
}


struct Tr2BufferDescriptionAL
{
public:
	Tr2BufferDescriptionAL();
	Tr2BufferDescriptionAL(
		Tr2RenderContextEnum::PixelFormat format,
		uint32_t count,
		Tr2GpuUsage::Type gpuUsage,
		Tr2CpuUsage::Type cpuUsage );
	Tr2BufferDescriptionAL(
		uint32_t stride,
		uint32_t count,
		Tr2GpuUsage::Type gpuUsage,
		Tr2CpuUsage::Type cpuUsage );

	Tr2RenderContextEnum::PixelFormat format;
	uint32_t stride;
	uint32_t count;
	Tr2GpuUsage::Type gpuUsage;
	Tr2CpuUsage::Type cpuUsage;
};


class Tr2BufferAL
{
public:
	Tr2BufferAL();

	ALResult Create(
		const Tr2BufferDescriptionAL& desc,
		const void* initialData,
		Tr2PrimaryRenderContextAL& renderContext );

	ALResult Create(
		Tr2RenderContextEnum::PixelFormat format,
		uint32_t count,
		Tr2GpuUsage::Type gpuUsage,
		Tr2CpuUsage::Type cpuUsage,
		const void* initialData,
		Tr2PrimaryRenderContextAL& renderContext );

	ALResult Create(
		uint32_t stride,
		uint32_t count,
		Tr2GpuUsage::Type gpuUsage,
		Tr2CpuUsage::Type cpuUsage,
		const void* initialData,
		Tr2PrimaryRenderContextAL& renderContext );

	bool IsValid() const;
	Tr2ALMemoryType GetMemoryClass();

	const Tr2BufferDescriptionAL& GetDesc() const;
	uint32_t GetSize() const;

	bool operator==( const Tr2BufferAL& other ) const;

	template <typename T> ALResult MapForReading( const T*& data, Tr2RenderContextAL& renderContext );
	ALResult MapForReading( const void*& data, Tr2RenderContextAL& renderContext );
	void UnmapForReading( Tr2RenderContextAL& renderContext );

	template <typename T> ALResult MapForWriting( T*& data, Tr2RenderContextAL& renderContext );
	ALResult MapForWriting( void*& data, Tr2RenderContextAL& renderContext );
	void UnmapForWriting( Tr2RenderContextAL& renderContext );

	ALResult UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL& renderContext );

	uint32_t GetSrvIndexInHeap() const;
	uint32_t GetUavIndexInHeap() const;

	ALResult SetName( const char* name );

	TrinityALImpl::Tr2BufferAL* TrinityALImpl_GetObject() const;

private:
	std::shared_ptr<TrinityALImpl::Tr2BufferAL> m_buffer;

	friend class Tr2RenderContextAL;
	friend class Tr2PrimaryRenderContextAL;
	friend class TrinityALImpl::Tr2ResourceSetAL;
};


template <typename T>
ALResult Tr2BufferAL::MapForReading( const T*& data, Tr2RenderContextAL& renderContext )
{
	return MapForReading( *reinterpret_cast<const void**>( &data ), renderContext );
}


template <typename T>
ALResult Tr2BufferAL::MapForWriting( T*& data, Tr2RenderContextAL& renderContext )
{
	return MapForWriting( *reinterpret_cast<void**>( &data ), renderContext );
}
