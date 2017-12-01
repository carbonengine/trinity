
#pragma once
#ifndef Tr2GpuBufferALGL4_h_
#define Tr2GpuBufferALGL4_h_


#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"


class Tr2RenderContextAL;


#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

class Tr2RenderContextAL;

namespace TrinityALImpl
{
	class Tr2ResourceSetAL;
}

class Tr2GpuBufferAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_GPU_BUFFER>
{
public:
	Tr2GpuBufferAL();
	
	ALResult Create(			
		uint32_t numberOfElements, 
		Tr2RenderContextEnum::PixelFormat format, 
		Tr2RenderContextEnum::BufferUsage usage,
		Tr2RenderContextEnum::ExFlag flags,
		const void* initialData,
		Tr2RenderContextAL & renderContext );

	ALResult Create(	
		uint32_t numberOfElements, 
		uint32_t elementSize, 
		Tr2RenderContextEnum::BufferUsage usage,
		Tr2RenderContextEnum::GpuBufferUsage gpuBufferUsage,
		Tr2RenderContextEnum::ExFlag flags,
		const void* initialData,
		Tr2RenderContextAL & renderContext );

	ALResult CreateVbView(		
		Tr2VertexBufferAL& vb,
		bool gpuWritable,
		Tr2PrimaryRenderContextAL & renderContext );

	void Destroy();
	bool IsValid() const;

	ALResult Lock(				
		uint32_t offset, 
		uint32_t sizeInBytes, 
		void** data, 
		Tr2RenderContextEnum::LockType lockType, 
		Tr2RenderContextAL & renderContext );

	ALResult Unlock( Tr2RenderContextAL & renderContext );


	unsigned BytesPerElement() const;
	unsigned GetNumElements() const;
	unsigned GetTotalSizeInBytes() const;
	Tr2RenderContextEnum::PixelFormat GetFormat() const;
	Tr2RenderContextEnum::GpuBufferUsage GetGpuBufferUsage() const;

	Tr2ALMemoryType GetMemoryClass() const;

	ALResult CopySubBuffer( 
		uint32_t offset, 
		uint32_t length, 
		Tr2GpuBufferAL& dest, 
		uint32_t destOffset, 
		Tr2RenderContextAL& renderContext );
private:
	ALResult Create(			
		uint32_t numberOfElements, 
		uint32_t elementSize, 
		GLenum internalFormat,
		Tr2RenderContextEnum::PixelFormat format, 
		Tr2RenderContextEnum::BufferUsage usage,
		const void* initialData, 
		uint32_t exFlags,
		Tr2RenderContextAL & renderContext );


	friend class Tr2RenderContextAL;
	friend class TrinityALImpl::Tr2ResourceSetAL;

	uint32_t m_numElements;
	uint32_t m_elementSize;
	Tr2RenderContextEnum::PixelFormat m_format;
	Tr2RenderContextEnum::BufferUsage m_usage;
	std::shared_ptr<GLuint> m_buffer;
	std::shared_ptr<GLuint> m_texture;
	mutable cl_mem m_clObject;

	Tr2GpuBufferAL( const Tr2GpuBufferAL& ) /* = delete */;
	Tr2GpuBufferAL& operator=( const Tr2GpuBufferAL& ) /* = delete */;
};

#endif

#endif
