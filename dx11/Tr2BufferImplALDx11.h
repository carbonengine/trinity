#pragma once
#ifndef Tr2BufferImplALDx11_h_
#define Tr2BufferImplALDx11_h_


#include "../ALResult.h"
#include "../Tr2RenderContextEnum.h"
#include "../Tr2MemoryCounterAL.h"

#ifdef TRINITY_AL_GUARD_LOCKS
#include "../Tr2LockGuard.h"
#endif


class Tr2PrimaryRenderContextAL;
class Tr2RenderContextAL;


#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

// -------------------------------------------------------------
// Description:
//   Helper class that manages an ID3D11Buffer plus a staging buffer
//   for CPU readback.  Vertex and Index buffers are almost identical
//   in DX11 so we can use this code.
//	 32bit: no support for 64bit-sized buffers.
// SeeAlso:
//   Tr2VertexBuffer, Tr2IndexBufferAL
// -------------------------------------------------------------
class Tr2BufferImplAL
{
public:
    Tr2BufferImplAL();
	~Tr2BufferImplAL();

	Tr2BufferImplAL& operator=( Tr2BufferImplAL&& );

	// Lock the index buffer. sizeInBytes is in bytes, not indices.
	ALResult Lock(
		uint32_t offset, 
		uint32_t sizeInBytes, 
		void** data, 
		Tr2RenderContextEnum::LockType, 
		Tr2RenderContextAL & renderContext );

	ALResult Unlock( Tr2RenderContextAL & renderContext );

	bool IsValid() const { return m_buffer != nullptr; }
	void Destroy();

	ALResult UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext );

	Tr2RenderContextEnum::BufferUsage m_usage;
protected:
	ALResult Create(
		uint32_t lengthInBytes,
		Tr2RenderContextEnum::BufferUsage usage,
		/*D3D11_BIND_FLAG*/				uint32_t bindFlags,
		/*D3D11_RESOURCE_MISC_FLAG*/	uint32_t miscFlags,
		const void* initialData,
		Tr2PrimaryRenderContextAL &renderContext );

	ALResult LockReading( uint32_t offset, uint32_t size, void** data, Tr2RenderContextAL & renderContext );
	ALResult UnlockReading( Tr2RenderContextAL & renderContext );
	ALResult LockWriting( uint32_t offset, uint32_t size, void** data, Tr2RenderContextEnum::LockType lockType, Tr2RenderContextAL & renderContext );
	ALResult UnlockWriting( Tr2RenderContextAL & renderContext );

	Tr2RenderContextEnum::LockType	m_currentLock;
	CComPtr<ID3D11Buffer>			m_staging;
	CcpMallocBuffer m_writeLockMemory;
	Tr2MemoryCounterAL m_memory;

	CComPtr<ID3D11Buffer>	m_buffer;

	uint32_t m_lengthInBytes;

#if TRINITY_AL_GUARD_LOCKS
	Tr2LockGuard m_lockGuard;
#endif

	friend class Tr2RenderContextAL;
	friend class Tr2PrimaryRenderContextAL;
	friend class Tr2GpuBufferAL;

	Tr2BufferImplAL( const Tr2BufferImplAL& ) /* = delete */;
};

#endif	// #if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#endif //Tr2BufferImplALDx11_h_
