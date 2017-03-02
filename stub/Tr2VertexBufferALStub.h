#pragma once
#ifndef Tr2VertexBufferALStub_h_
#define Tr2VertexBufferALStub_h_

#if( TRINITY_PLATFORM==TRINITY_STUB )

class Tr2RenderContextAL;

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

	ALResult Create( uint32_t lengthInBytes,
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
	long Lock( T*& data,
			   Tr2RenderContextEnum::LockType lockType,
			   Tr2RenderContextAL& renderContext )
	{
		return Lock( 0, 0, reinterpret_cast<void**>( &data ), lockType, renderContext ).GetResult();
	}

	uint32_t GetTotalSizeInBytes() const
	{
		return uint32_t( m_buffer.size() );
	}

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}
private:
	CcpMallocBuffer m_buffer;
	Tr2RenderContextEnum::BufferUsage m_usage;


};

#endif // TRINITY_PLATFORM==TRINITY_STUB

#endif //Tr2VertexBufferALStub_h_
