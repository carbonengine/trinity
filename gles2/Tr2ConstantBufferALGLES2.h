#pragma once
#ifndef Tr2ConstantBufferALGLES2_h_
#define Tr2ConstantBufferALGLES2_h_

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

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

	ALResult Create( uint32_t size, Tr2RenderContextAL & renderContext )
	{
		return Create( size, Tr2RenderContextEnum::USAGE_CPU_WRITE, nullptr, renderContext );
	}

	ALResult Create( uint32_t size, Tr2RenderContextEnum::BufferUsage usage, const void* initialData, Tr2RenderContextAL & renderContext );
	ALResult Lock( void** data, Tr2RenderContextAL & renderContext );
	ALResult Unlock( Tr2RenderContextAL & renderContext );
	bool IsValid() const;
	void Destroy();

	Tr2RenderContextEnum::BufferUsage GetUsage() const
	{
		return m_usage;
	}

	uint32_t GetSize() const { return static_cast<uint32_t>( m_shadowCopy.size() ); }

	void* GetBufferMirror( uint32_t minimumSize, Tr2RenderContextAL & renderContext );
	void* GetBufferMirror( Tr2RenderContextAL & renderContext ) { return GetBufferMirror( 0, renderContext ); }
	ALResult UpdateFromMirror( Tr2RenderContextAL & renderContext );

	bool operator==( const Tr2ConstantBufferAL& other ) const { return this == &other; }

	Tr2ALMemoryType GetMemoryClass() const { return AL_MEMORY_MANAGED; }
	
private:
	Tr2ConstantBufferAL( const Tr2ConstantBufferAL& ) /* = delete */;
	Tr2ConstantBufferAL& operator=( const Tr2ConstantBufferAL& ) /* = delete */;

	CcpMallocBuffer	m_shadowCopy;
	bool			m_debugUsingMirror;
	Tr2RenderContextEnum::BufferUsage m_usage;

	friend class Tr2RenderContextAL;
};

#endif // #if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#endif //Tr2ConstantBufferALGLES2_h_
