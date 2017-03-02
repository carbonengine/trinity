#include "StdAfx.h"
#include "Tr2IndexBufferALGLES2.h"


#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2IndexBufferAL::Tr2IndexBufferAL()
	: m_numIndices( 0 )
	, m_is16Bit( false )
	, m_usage( 0 )
	, m_buffer( 0 )
{
#ifdef TRINITY_AL_MOBILE
    m_lockedSize = 0;
#endif
}

Tr2IndexBufferAL::~Tr2IndexBufferAL()
{
	Destroy();
}

Tr2IndexBufferAL& Tr2IndexBufferAL::operator=( Tr2IndexBufferAL&& other )
{
	if( this != &other )
	{
		m_usage			= other.m_usage;

		m_numIndices	= other.m_numIndices;
		m_is16Bit		= other.m_is16Bit;		

		m_buffer		= other.m_buffer;
		other.m_buffer = 0;
		ChangeObjectId();
	}

	return *this;
}

ALResult Tr2IndexBufferAL::Create(	
	uint32_t numberOfIndices, 
	BufferUsage usage, 
	IndexBufferBitcount bitCount, 
	const void* initialData, 
	Tr2RenderContextAL & renderContext )
{
	if( !ValidateUsage( usage ) )
	{
		CCP_AL_LOGERR( "Invalid combination of USAGE flags passed to Tr2IndexBufferAL Create function" );
		return E_INVALIDARG;
	}

	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	if( ( usage & USAGE_IMMUTABLE ) && !initialData )
	{
		CCP_AL_LOGERR( "Create: Trying to create an immutable buffer without providing data" );
		return E_INVALIDARG;
	}

	m_is16Bit = bitCount == IB_16BIT;
	m_numIndices = numberOfIndices;
	m_usage = usage;

	if( m_buffer )
	{
		glDeleteBuffers( 1, &m_buffer );
		m_buffer = 0;
	}

	GL_FAIL(	glGenBuffers( 1, &m_buffer ) );
	GL_FAIL(	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_buffer ) );

	GLenum glUsage = GL_STATIC_DRAW;
	if( m_usage & USAGE_LOCK_FREQUENTLY )
	{
		glUsage = GL_DYNAMIC_DRAW;
	}

	GL_FAIL( glBufferData( GL_ELEMENT_ARRAY_BUFFER, GetTotalSizeInBytes(), initialData, glUsage ) );
	ChangeObjectId();
	return S_OK;
}

ALResult Tr2IndexBufferAL::Lock(	
	uint32_t offset, 
	uint32_t sizeInBytes, 
	uint16_t *&data, 
	LockType lockType, 
	Tr2RenderContextAL & renderContext )
{
	if( !m_is16Bit )
	{
		data = nullptr;
		return E_INVALIDARG;
	}
	return Lock( offset, sizeInBytes, (void**)&data, lockType, renderContext );
}

ALResult Tr2IndexBufferAL::Lock(	
	uint32_t offset, 
	uint32_t sizeInBytes, 
	uint32_t *&  data, 
	LockType lockType, 
	Tr2RenderContextAL & renderContext )
{
	if( m_is16Bit )
	{
		data = nullptr;
		return E_FAIL;
	}
	return Lock( offset, sizeInBytes, (void**)&data, lockType, renderContext );
}

ALResult Tr2IndexBufferAL::Lock(	
	uint32_t offset, 
	uint32_t size, 
	void** data, 
	LockType lockType, 
	Tr2RenderContextAL & renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	if( !m_buffer || offset != 0 )
	{
		*data = 0;
		return E_INVALIDARG;
	}

	if( lockType == LOCK_READONLY )
	{
#ifdef TRINITY_AL_MOBILE
        return E_FAIL;
#endif
        
		if( ( m_usage & USAGE_CPU_READ ) == 0 )
		{
			return E_INVALIDARG;
		}
	}
	else
	{
		if( ( m_usage & USAGE_CPU_WRITE ) == 0 )
		{
			return E_INVALIDARG;
		}
	}

#ifdef TRINITY_AL_MOBILE
	if( lockType == LOCK_READONLY || lockType == LOCK_NO_OVERWRITE )
	{
		return E_FAIL;
	}
    if( size == 0 )
    {
        size = GetTotalSizeInBytes();
    }
    if( offset + size > GetTotalSizeInBytes() )
    {
        return E_INVALIDARG;
    }
    m_lockedData.resize( "Tr2VertexBufferAL::Lock", size );
    m_lockedOffset = offset;
    m_lockedSize = size;
    *data = m_lockedData.get();
#else
	if( lockType == LOCK_NO_OVERWRITE && ( m_usage & USAGE_LOCK_FREQUENTLY ) == 0 )
	{
		return E_INVALIDARG;
	}
    if( lockType == LOCK_NO_OVERWRITE && !glMapBufferRange )
    {
        return E_FAIL;
    }
	GL_FAIL( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_buffer ) );
	if( lockType == LOCK_NO_OVERWRITE )
	{
		GL_FAIL( *data = glMapBufferRange( GL_ELEMENT_ARRAY_BUFFER, offset, size, GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_WRITE_BIT ) );
	}
	else
	{
		GL_FAIL( *data = glMapBuffer(	
							GL_ELEMENT_ARRAY_BUFFER, 
							lockType == LOCK_READONLY ? GL_READ_ONLY : GL_WRITE_ONLY ) 
			   );
	}
#endif
	return S_OK;
}

ALResult Tr2IndexBufferAL::Unlock( Tr2RenderContextAL & renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	if( !m_buffer )
	{
		return E_FAIL;
	}
	GL_FAIL( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_buffer ) );
#ifdef TRINITY_AL_MOBILE
    if( m_lockedSize == 0 )
    {
        return E_FAIL;
    }
    GL_FAIL( glBufferSubData( GL_ARRAY_BUFFER, m_lockedOffset, m_lockedSize, m_lockedData.get() ) );
#else
	GL_FAIL( glUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER ) );
#endif
	return S_OK;
}

ALResult Tr2IndexBufferAL::UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext )
{
	if( !renderContext.IsValid() || !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( offset + size > GetTotalSizeInBytes() )
	{
		return E_INVALIDARG;
	}
	if( ( m_usage & USAGE_CPU_WRITE ) == 0 || ( m_usage & USAGE_LOCK_FREQUENTLY ) != 0 )
	{
		return E_INVALIDCALL;
	}
	if( size == 0 )
	{
		return S_OK;
	}

	GL_FAIL( glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_buffer ) );
    GL_FAIL( glBufferSubData( GL_ELEMENT_ARRAY_BUFFER, offset, size, data ) );
	return S_OK;
}

bool Tr2IndexBufferAL::IsValid() const
{
	return m_buffer != 0;
}

void Tr2IndexBufferAL::Destroy()
{
	if( m_buffer )
	{
		glDeleteBuffers( 1, &m_buffer );
		m_buffer = 0;
	}
#ifdef TRINITY_AL_MOBILE
    m_lockedSize = 0;
#endif
}

#endif
