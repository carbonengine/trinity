#pragma once
#ifndef Tr2ConstantBufferALDx11_h_
#define Tr2ConstantBufferALDx11_h_

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#ifdef TRINITY_AL_GUARD_LOCKS
#include "Tr2LockGuard.h"
#endif

// -------------------------------------------------------------
// Description:
//   A low level wrapper around the calls needed to set up a constant
//   buffer for a given platform. Any higher level logic should be
//   handled one level up, this is only to avoid ifdef soup when
//   creating and locking buffers.
//	 32bit: no support for 64bit-sized buffers.
// SeeAlso:
//   Tr2VertexBufferAL
// -------------------------------------------------------------
class Tr2ConstantBufferAL: public Tr2TrackedALObject<Tr2RenderContextEnum::OT_CONSTANT_BUFFER>
{
public:
    Tr2ConstantBufferAL();
	~Tr2ConstantBufferAL();

	Tr2ConstantBufferAL( Tr2ConstantBufferAL&& );
	Tr2ConstantBufferAL& operator=( Tr2ConstantBufferAL&& );

	ALResult Create( uint32_t size, Tr2PrimaryRenderContextAL & renderContext )
	{
		return Create( size, Tr2RenderContextEnum::USAGE_CPU_WRITE, nullptr, renderContext );
	}

	ALResult Create( uint32_t size, Tr2RenderContextEnum::BufferUsage usage, const void* initialData, Tr2PrimaryRenderContextAL & renderContext );
	ALResult Lock( void** data, Tr2RenderContextAL & renderContext );
	ALResult Unlock( Tr2RenderContextAL & renderContext );
	bool IsValid() const 
	{ 
		return m_size > 0 && ( ( m_usage & Tr2RenderContextEnum::USAGE_LOCK_FREQUENTLY ) != 0 || m_buffer != nullptr ); 
	}
	void Destroy();

	Tr2RenderContextEnum::BufferUsage GetUsage() const
	{
		return m_usage;
	}

	uint32_t GetSize() const { return m_size; }

	void* GetBufferMirror( uint32_t minimumSize, Tr2RenderContextAL & renderContext );
	void* GetBufferMirror( Tr2RenderContextAL & renderContext ) 
	{ 
		return GetBufferMirror( 0, renderContext ); 
	}
	ALResult UpdateFromMirror( Tr2RenderContextAL & renderContext );

	bool operator==( const Tr2ConstantBufferAL& other ) const 
	{ 
		return this == &other; 
	}

	CComPtr<ID3D11Buffer>	m_buffer;

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }

private:
	Tr2ConstantBufferAL( const Tr2ConstantBufferAL& ) /* = delete */;
	Tr2ConstantBufferAL& operator=( const Tr2ConstantBufferAL& ) /* = delete */;

	CcpMallocBuffer	m_bufferMirror;
	Tr2RenderContextEnum::BufferUsage m_usage;
	uint32_t	m_size;

	// Helper for asserting correct use: if in the same frame we lock/unlock, and use get/updateMirror, that's
	// a bug.
	enum FrameUse
	{
		FRAME_USE_NOT_USED_YET,
		FRAME_USE_LOCKING,
		FRAME_USE_MIRRORED
	};
	mutable FrameUse	m_frameUse;
	friend class Tr2RenderContextAL;

#ifdef TRINITY_AL_GUARD_LOCKS
	Tr2LockGuard m_lockGuard;
#endif
};

#endif // #if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#endif //Tr2ConstantBufferALDx11_h_
