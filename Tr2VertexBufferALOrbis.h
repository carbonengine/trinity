#pragma once
#ifndef Tr2VertexBufferALOrbis_h_
#define Tr2VertexBufferALOrbis_h_

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

class Tr2RenderContextAL;

#include "Tr2BufferImplALOrbis.h"

// -------------------------------------------------------------
// Description:
//   A low level wrapper around the calls needed to set up a vertex
//   buffer for a given platform. Any higher level logic should be
//   handled one level up, this is only to avoid ifdef soup when
//   creating and locking buffers.
//	 32bit: no support for 64bit-sized buffers.
// SeeAlso:
//   Tr2IndexBufferAL
// -------------------------------------------------------------
class Tr2VertexBufferAL :
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_VERTEX_BUFFER>
{
public:
	Tr2VertexBufferAL();
	~Tr2VertexBufferAL();

	Tr2VertexBufferAL& operator=( Tr2VertexBufferAL&& );

	ALResult Create( uint32_t lengthInBytes,
					 Tr2RenderContextEnum::BufferUsage usage,
					 const void* initialData,
					 Tr2RenderContextAL& renderContext );

	ALResult Create( uint32_t length,
					 Tr2RenderContextAL& renderContext );

	ALResult CreateEx( uint32_t lengthInBytes,
					   Tr2RenderContextEnum::BufferUsage usage,
					   const void* initialData,
					   uint32_t exFlags,
					   Tr2PrimaryRenderContextAL& renderContext );

	ALResult Lock( uint32_t offset,
				   uint32_t size,
				   void** data,
				   Tr2RenderContextEnum::LockType lockType,
				   Tr2RenderContextAL& renderContext );
	ALResult Unlock( Tr2RenderContextAL& renderContext );

	ALResult UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext );

	bool IsValid() const;
	void Destroy();

	template<typename T>
	ALResult Lock( T*& data,
				   Tr2RenderContextEnum::LockType lockType,
				   Tr2RenderContextAL& renderContext )
	{
		// does the VB contain an integer multiple of T's ? If not, and you know what you're doing, use Lock(0,0,(void**)data...) instead
		CCP_ASSERT( ( m_lengthInBytes % sizeof( T ) ) == 0 );
		return Lock( 0, 0, reinterpret_cast<void**>( &data ), lockType, renderContext );
	}

	uint32_t GetTotalSizeInBytes() const
	{
		return m_lengthInBytes;
	}

	bool operator==( const Tr2VertexBufferAL& other ) const;

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	//GLuint m_buffer;

private:
	uint32_t m_lengthInBytes;
	Tr2RenderContextEnum::BufferUsage m_usage;
	Tr2BufferImplAL m_buffer;

	Tr2VertexBufferAL( const Tr2VertexBufferAL& )/* = delete */;
	Tr2VertexBufferAL& operator=( const Tr2VertexBufferAL& )/* = delete */;

	friend class Tr2RenderContextAL;
};

#endif

#endif
