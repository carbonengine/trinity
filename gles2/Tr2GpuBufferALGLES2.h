
#pragma once
#ifndef Tr2GpuBufferALGLES2_h_
#define Tr2GpuBufferALGLES2_h_


#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"


#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

class Tr2RenderContextAL;

class Tr2GpuBufferAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_GPU_BUFFER>
{
public:
	Tr2GpuBufferAL()
	{
	}

	ALResult Create(
		uint32_t numberOfElements,
		Tr2RenderContextEnum::PixelFormat format,
		Tr2RenderContextEnum::BufferUsage usage,
		Tr2RenderContextEnum::ExFlag flags,
		const void* initialData,
		Tr2PrimaryRenderContextAL & renderContext )
	{
		return E_FAIL;
	}

	ALResult Create(
		uint32_t numberOfElements,
		uint32_t elementSize,
		Tr2RenderContextEnum::BufferUsage usage,
		Tr2RenderContextEnum::GpuBufferUsage gpuBufferUsage,
		Tr2RenderContextEnum::ExFlag flags,
		const void* initialData,
		Tr2PrimaryRenderContextAL & renderContext )
	{
		return E_FAIL;
	}

	ALResult CreateVbView(
		Tr2VertexBufferAL& vb,
		bool gpuWritable,
		Tr2PrimaryRenderContextAL & renderContext )
	{
		return E_FAIL;
	}

	void Destroy()
	{
	}
	bool IsValid() const
	{
		return false;
	}

	ALResult Lock(
		uint32_t offset,
		uint32_t sizeInBytes,
		void** data,
		Tr2RenderContextEnum::LockType lockType,
		Tr2RenderContextAL & renderContext )
	{
		return E_FAIL;
	}

	ALResult Unlock( Tr2RenderContextAL & renderContext )
	{
		return E_FAIL;
	}

	unsigned BytesPerElement() const
	{
		return 0;
	}

	unsigned GetNumElements() const
	{
		return 0;
	}

	unsigned GetTotalSizeInBytes() const
	{
		return 0;
	}

	Tr2RenderContextEnum::PixelFormat GetFormat() const
	{
		return Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN;
	}

	Tr2RenderContextEnum::GpuBufferUsage GetGpuBufferUsage() const
	{
		return 0;
	}

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	ALResult CopySubBuffer(
		uint32_t offset,
		uint32_t length,
		Tr2GpuBufferAL& dest,
		uint32_t destOffset,
		Tr2RenderContextAL& renderContext )
	{
		return E_FAIL;
	}
};

#endif // #if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#endif //Tr2GpuBufferALGLES2_h_
