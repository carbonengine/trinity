
#pragma once
#ifndef Tr2GpuBufferALDx9_h_
#define Tr2GpuBufferALDx9_h_

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

class Tr2RenderContextAL;

class Tr2GpuBufferAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_GPU_BUFFER>
{
public:
	Tr2GpuBufferAL()
		: m_numElements(0)
		, m_format(Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN)
	{
	}
	
	ALResult Create(			
		uint32_t numberOfElements, 
		Tr2RenderContextEnum::PixelFormat format, 
		Tr2RenderContextEnum::BufferUsage usage,
		const void* initialData, 
		Tr2RenderContextAL & renderContext ) 
	{ 
		return E_FAIL; 
	}	

	ALResult CreateEx(			
		uint32_t numberOfElements, 
		Tr2RenderContextEnum::PixelFormat format, 
		Tr2RenderContextEnum::BufferUsage usage,
		const void* initialData, 
		uint32_t exFlags,
		Tr2RenderContextAL & renderContext ) 
	{ 
		return E_FAIL; 
	}	

	ALResult CreateStructured(	
		uint32_t numberOfElements, 
		uint32_t elementSize, 
		Tr2RenderContextEnum::BufferUsage usage,
		Tr2RenderContextEnum::GpuBufferUsage gpuBufferUsage,
		const void* initialData, 
		Tr2RenderContextAL & renderContext )
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

	ALResult CreateAlias(		
		Tr2GpuBufferAL& other, 
		Tr2RenderContextEnum::PixelFormat format, 
		Tr2RenderContextAL & renderContext )
	{ 
		return E_FAIL; 
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
	bool IsValid() const 
	{ 
		return false; 
	}
	void Destroy() 
	{
	}

	unsigned BytesPerElement() const		{ return 0; }
	unsigned GetNumElements() const			{ return 0; }
	unsigned GetTotalSizeInBytes() const	{ return 0; }
	Tr2RenderContextEnum::PixelFormat GetFormat() const 
	{ return Tr2RenderContextEnum::PIXEL_FORMAT_UNKNOWN; }
	Tr2RenderContextEnum::GpuBufferUsage GetGpuBufferUsage() const
	{
		return 0;
	}

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }

	ALResult CopySubBuffer( 
		uint32_t offset, 
		uint32_t length, 
		Tr2GpuBufferAL& dest, 
		uint32_t destOffset, 
		Tr2RenderContextAL& renderContext )
	{
		return E_FAIL;
	}
	
private:
	friend class Tr2RenderContextAL;

	uint32_t							m_numElements;
	Tr2RenderContextEnum::PixelFormat	m_format;

	Tr2GpuBufferAL( const Tr2GpuBufferAL& ) /* = delete */;
	Tr2GpuBufferAL& operator=( const Tr2GpuBufferAL& ) /* = delete */;
};

#endif // #if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#endif //Tr2GpuBufferALDx9Dx11_h_
