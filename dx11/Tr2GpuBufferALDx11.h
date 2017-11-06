
#pragma once
#ifndef Tr2GpuBufferALDx11_h_
#define Tr2GpuBufferALDx11_h_


#include "../Tr2TrackedALObject.h"
#include "Tr2BufferImplALDx11.h"


class Tr2RenderContextAL;
class Tr2VertexBufferAL;


#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

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
		Tr2RenderContextEnum::ExFlag flags,
		const void* initialData, 
		Tr2PrimaryRenderContextAL & renderContext );

	ALResult Create(	
		uint32_t numberOfElements, 
		uint32_t elementSize, 
		Tr2RenderContextEnum::BufferUsage usage,
		Tr2RenderContextEnum::GpuBufferUsage gpuBufferUsage,
		Tr2RenderContextEnum::ExFlag flags,
		const void* initialData,
		Tr2PrimaryRenderContextAL & renderContext );

	ALResult CreateVbView(		
		Tr2VertexBufferAL& vb,
		bool gpuWritable,
		Tr2PrimaryRenderContextAL & renderContext );

	void Destroy();
	bool IsValid() const;

	bool operator==( const Tr2GpuBufferAL& other ) const;

	uint32_t BytesPerElement() const;
	uint32_t GetNumElements() const;
	uint32_t GetTotalSizeInBytes() const;
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
	ALResult CreateImpl( 
		Tr2RenderContextEnum::BufferUsage usage,
		Tr2RenderContextEnum::GpuBufferUsage gpuBufferUsage,
		const void* initialData, 
		uint32_t flags,
		Tr2PrimaryRenderContextAL & renderContext );

	friend class Tr2RenderContextAL;

	CComPtr<ID3D11ShaderResourceView>	m_srv;
	CComPtr<ID3D11UnorderedAccessView>	m_uav;

	uint32_t m_numElements;
	uint32_t m_elementSize;
	Tr2RenderContextEnum::PixelFormat	m_format;
	Tr2RenderContextEnum::GpuBufferUsage m_gpuBufferUsage;
};

#endif // #if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#endif //Tr2GpuBufferALDx11Dx11_h_
