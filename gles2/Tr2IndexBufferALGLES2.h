#pragma once
#ifndef Tr2IndexBufferALGLES2_h_
#define Tr2IndexBufferALGLES2_h_

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

class Tr2RenderContextAL;

// -------------------------------------------------------------
// Description:
//   A low level wrapper around the calls needed to set up an index
//   buffer for a given platform. Any higher level logic should be
//   handled one level up, this is only to avoid ifdef soup when
//   creating and locking buffers.
//	 32bit: no support for 64bit-sized buffers.
// SeeAlso:
//   Tr2VertexBufferAL
// -------------------------------------------------------------
class Tr2IndexBufferAL : 
	public Tr2TrackedALObject<Tr2RenderContextEnum::OT_INDEX_BUFFER>
{
public:
    Tr2IndexBufferAL();
	~Tr2IndexBufferAL();

	Tr2IndexBufferAL& operator=( Tr2IndexBufferAL&& );

	// Create an indexbuffer. NOTE that you pass the number of indices that you want, NOT the size in bytes
	// of the buffer.  That can be calculated from bitCount (which is an enum), so it would be redundant and error-prone
	// to ask a bytecount.
	ALResult Create(	
		uint32_t numberOfIndices, 
		Tr2RenderContextEnum::BufferUsage usage, 
		Tr2RenderContextEnum::IndexBufferBitcount bitCount, 
		const void* initialData, 
		Tr2RenderContextAL & renderContext );

	// Lock the index buffer. sizeInBytes is in bytes, not indices.
	ALResult Lock(		
		uint32_t offset, 
		uint32_t sizeInBytes, 
		void** data, 
		Tr2RenderContextEnum::LockType lockType, 
		Tr2RenderContextAL & renderContext );
	// typesafe, asserting versions
	ALResult Lock(		
		uint32_t offset, 
		uint32_t sizeInBytes, 
		uint16_t *&data, 
		Tr2RenderContextEnum::LockType, 
		Tr2RenderContextAL & renderContext );
	ALResult Lock(		
		uint32_t offset, 
		uint32_t sizeInBytes, 
		uint32_t *&data, 
		Tr2RenderContextEnum::LockType, 
		Tr2RenderContextAL & renderContext );
	ALResult Lock( uint16_t *&data, Tr2RenderContextEnum::LockType lockType, Tr2RenderContextAL & renderContext )
	{ 
		return Lock( 0, 0, data, lockType, renderContext ); 
	}
	ALResult Lock( uint32_t *&data, Tr2RenderContextEnum::LockType lockType, Tr2RenderContextAL & renderContext )
	{ 
		return Lock( 0, 0, data, lockType, renderContext ); 
	}
	ALResult Unlock( Tr2RenderContextAL & renderContext );

	ALResult UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext );

	bool IsValid() const;
	void Destroy();

	bool Is16Bit() const			{ return m_is16Bit; }
	uint32_t BytesPerIndex() const	{ return m_is16Bit ? 2 : 4; }
	uint32_t GetNumIndices() const	{ return m_numIndices; }
	uint32_t GetTotalSizeInBytes() const { return GetNumIndices() * BytesPerIndex(); }

	Tr2RenderContextEnum::IndexBufferBitcount	GetIBBitcount() const { return Is16Bit() ? Tr2RenderContextEnum::IB_16BIT : Tr2RenderContextEnum::IB_32BIT; }

	bool operator==( const Tr2IndexBufferAL& other ) const { return m_buffer == other.m_buffer; }

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }

	GLuint	m_buffer;
	
	Tr2RenderContextEnum::BufferUsage	m_usage;

private:
	uint32_t m_numIndices;	
	bool	 m_is16Bit;
#ifdef TRINITY_AL_MOBILE
    CcpMallocBuffer m_lockedData;
    uint32_t m_lockedOffset;
    uint32_t m_lockedSize;
#endif

	Tr2IndexBufferAL( const Tr2IndexBufferAL& ) /* = delete */;
	Tr2IndexBufferAL& operator=( const Tr2IndexBufferAL& ) /* = delete */;
};

#endif // #if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#endif //Tr2IndexBufferALDx9GLES2_h_
