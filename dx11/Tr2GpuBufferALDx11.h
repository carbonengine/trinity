
#pragma once
#ifndef Tr2GpuBufferALDx11_h_
#define Tr2GpuBufferALDx11_h_

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2BufferImplALDx11.h"

class Tr2RenderContextAL;

// -------------------------------------------------------------
// Description:
//   Unordered Access Buffer
// 32bit - does not support buffers > 4gig
// SeeAlso:
//   Tr2VertexBufferAL
// -------------------------------------------------------------
class Tr2GpuBufferAL : 
	public Tr2BufferImplAL, 
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_GPU_BUFFER>
{
public:
	Tr2GpuBufferAL();
	Tr2GpuBufferAL& operator=( Tr2GpuBufferAL&& );

    ALResult Create(			
		uint32_t numberOfElements, 
		Tr2RenderContextEnum::PixelFormat format, 
		Tr2RenderContextEnum::BufferUsage usage,
		const void* initialData, 
		Tr2PrimaryRenderContextAL & renderContext );

	ALResult CreateEx(			
		uint32_t numberOfElements, 
		Tr2RenderContextEnum::PixelFormat format, 
		Tr2RenderContextEnum::BufferUsage usage,
		const void* initialData, 
		uint32_t flags,
		Tr2PrimaryRenderContextAL & renderContext );

	ALResult CreateStructured(	
		uint32_t numberOfElements, 
		uint32_t elementSize, 
		Tr2RenderContextEnum::BufferUsage usage,
		Tr2RenderContextEnum::GpuBufferUsage gpuBufferUsage,
		const void* initialData, 
		Tr2PrimaryRenderContextAL & renderContext );

	ALResult CreateAlias(		
		Tr2GpuBufferAL& other, 
		Tr2RenderContextEnum::PixelFormat format, 
		Tr2PrimaryRenderContextAL & renderContext );

	ALResult CreateVbView(		
		Tr2VertexBufferAL& vb,
		bool gpuWritable,
		Tr2PrimaryRenderContextAL & renderContext );
	
	ALResult Lock(	
		uint32_t offset, 
		uint32_t sizeInBytes, 
		void** data, 
		Tr2RenderContextEnum::LockType lockType, 
		Tr2RenderContextAL & renderContext )
	{
		return Tr2BufferImplAL::Lock( offset, sizeInBytes, data, lockType, renderContext );
	}

	ALResult Unlock( Tr2RenderContextAL & renderContext )
	{
		return Tr2BufferImplAL::Unlock( renderContext );
	}

	bool IsValid() const;
	void Destroy();

	using Tr2BufferImplAL::IsValid;
	using Tr2BufferImplAL::Destroy;

	CComPtr<ID3D11ShaderResourceView>	m_srv;
	CComPtr<ID3D11UnorderedAccessView>	m_uav;

	unsigned BytesPerElement() const
	{ 
		return m_elementSize ? m_elementSize : GetBytesPerPixel( m_format ); 
	}
	unsigned GetNumElements() const
	{ 
		return m_numElements; 
	}
	unsigned GetTotalSizeInBytes() const
	{ 
		return GetNumElements() * BytesPerElement(); 
	}
	Tr2RenderContextEnum::PixelFormat GetFormat() const 
	{ 
		return m_format; 
	}
	Tr2RenderContextEnum::GpuBufferUsage GetGpuBufferUsage() const
	{
		return m_gpuBufferUsage;
	}

	bool operator==( const Tr2GpuBufferAL& other ) const
	{ 
		return m_buffer == other.m_buffer; 
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
		Tr2RenderContextAL& renderContext );
	
private:
	ALResult CreateImpl( 
		Tr2RenderContextEnum::BufferUsage usage,
		Tr2RenderContextEnum::GpuBufferUsage gpuBufferUsage,
		const void* initialData, 
		uint32_t flags,
		Tr2PrimaryRenderContextAL & renderContext );

	friend class Tr2RenderContextAL;

	uint32_t m_numElements;
	uint32_t m_elementSize;
	Tr2RenderContextEnum::PixelFormat	m_format;
	Tr2RenderContextEnum::GpuBufferUsage m_gpuBufferUsage;
	Tr2GpuBufferAL( const Tr2GpuBufferAL& ) /* = delete */;
	Tr2GpuBufferAL& operator=( const Tr2GpuBufferAL& ) /* = delete */;
};

#endif // #if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#endif //Tr2GpuBufferALDx11Dx11_h_
