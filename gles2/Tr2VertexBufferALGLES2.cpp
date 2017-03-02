#include "StdAfx.h"
#include "Tr2VertexBufferALGLES2.h"


#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2VertexBufferAL::Tr2VertexBufferAL()
	:m_lengthInBytes( 0 ),
	m_usage( 0 ),
	m_buffer( 0 ),
	m_lockedOffset( 0 ),
	m_lockedSize( 0 )
{
}

Tr2VertexBufferAL::~Tr2VertexBufferAL()
{
	Destroy();
}

Tr2VertexBufferAL& Tr2VertexBufferAL::operator=( Tr2VertexBufferAL&& other )
{
	if( this != &other )
	{
		m_lengthInBytes = other.m_lengthInBytes;
		m_usage = other.m_usage;
		m_buffer = other.m_buffer;
		other.m_buffer = 0;
		ChangeObjectId();
	}

	return *this;
}

ALResult Tr2VertexBufferAL::CreateEx( uint32_t lengthInBytes,
									  Tr2RenderContextEnum::BufferUsage usage,
									  const void* initialData,
									  uint32_t/*exFlags*/,
									  Tr2PrimaryRenderContextAL& renderContext )
{
	return Create( lengthInBytes, usage, initialData, renderContext );
}

ALResult Tr2VertexBufferAL::Create( uint32_t lengthInBytes,
									Tr2RenderContextEnum::BufferUsage usage,
									const void* initialData,
									Tr2RenderContextAL& renderContext )
{
	if ( !ValidateUsage( usage ) )
	{
		CCP_AL_LOGERR( "Invalid combination of USAGE flags passed to Tr2VertexBufferAL Create function" );
		return E_INVALIDARG;
	}

	if ( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	if( ( usage & USAGE_IMMUTABLE ) && !initialData )
	{
		CCP_AL_LOGERR( "Create: Trying to create an immutable buffer without providing data" );
		return E_INVALIDARG;
	}

	m_lengthInBytes = lengthInBytes;
	m_usage = usage;

	if ( m_buffer )
	{
		Destroy();
	}

	GL_FAIL( glGenBuffers( 1, &m_buffer ) );
	GL_FAIL( glBindBuffer( GL_ARRAY_BUFFER, m_buffer ) );

	GLenum glUsage = GL_STATIC_DRAW;
	if ( m_usage & USAGE_LOCK_FREQUENTLY )
	{
		glUsage = GL_DYNAMIC_DRAW;
	}

	GL_FAIL( glBufferData( GL_ARRAY_BUFFER, lengthInBytes, initialData, glUsage ) );

	ChangeObjectId();
	return S_OK;
}

ALResult Tr2VertexBufferAL::Create( uint32_t lengthInBytes,
									Tr2RenderContextAL& renderContext )
{
	return Create( lengthInBytes, Tr2RenderContextEnum::USAGE_CPU_READ | Tr2RenderContextEnum::USAGE_CPU_WRITE,
		nullptr, renderContext );
}

ALResult Tr2VertexBufferAL::Lock( uint32_t offset,
								  uint32_t size,
								  void** data,
								  Tr2RenderContextEnum::LockType lockType,
								  Tr2RenderContextAL& renderContext )
{
	if ( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	if ( !m_buffer || offset != 0 )
	{
		*data = 0;
		return E_FAIL;
	}

	if ( lockType == LOCK_READONLY )
	{
		if ( ( m_usage & USAGE_CPU_READ ) == 0 )
		{
			return E_FAIL;
		}
	}
	else
	{
		if ( ( m_usage & USAGE_CPU_WRITE ) == 0 )
		{
			return E_FAIL;
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
	GL_FAIL( glBindBuffer( GL_ARRAY_BUFFER, m_buffer ) );
	if( lockType == LOCK_NO_OVERWRITE )
	{
		GL_FAIL( *data = glMapBufferRange( GL_ARRAY_BUFFER, offset, size, GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_WRITE_BIT ) );
	}
	else if( lockType == LOCK_WRITEONLY && ( m_usage & USAGE_LOCK_FREQUENTLY ) )
	{
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
	}
	else
	{
		m_lockedSize = 0;
		GL_FAIL( *data = glMapBuffer( GL_ARRAY_BUFFER, lockType == LOCK_READONLY ? GL_READ_ONLY : GL_WRITE_ONLY ) );
	}
#endif
	return S_OK;
}

ALResult Tr2VertexBufferAL::Unlock( Tr2RenderContextAL& renderContext )
{
	if ( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	if ( !m_buffer )
	{
		return E_FAIL;
	}
	GL_FAIL( glBindBuffer( GL_ARRAY_BUFFER, m_buffer ) );
    
#ifdef TRINITY_AL_MOBILE
    if( m_lockedSize == 0 )
    {
        return E_FAIL;
    }
    GL_FAIL( glBufferSubData( GL_ARRAY_BUFFER, m_lockedOffset, m_lockedSize, m_lockedData.get() ) );
#else
	if( m_lockedSize )
	{
		if( m_lockedSize == GetTotalSizeInBytes() )
		{
			GL_FAIL( glBufferData( GL_ARRAY_BUFFER, m_lockedSize, m_lockedData.get(), GL_DYNAMIC_DRAW ) );
		}
		else
		{
			GL_FAIL( glBufferSubData( GL_ARRAY_BUFFER, m_lockedOffset, m_lockedSize, m_lockedData.get() ) );
		}
		m_lockedSize = 0;
	}
	else
	{
		GL_FAIL( glUnmapBuffer( GL_ARRAY_BUFFER ) );
	}
#endif
	return S_OK;
}

ALResult Tr2VertexBufferAL::UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext )
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
    GL_FAIL( glBufferSubData( GL_ARRAY_BUFFER, offset, size, data ) );
	return S_OK;
}

bool Tr2VertexBufferAL::IsValid() const
{
	return m_buffer != 0;
}

void Tr2VertexBufferAL::Destroy()
{
	if ( m_buffer )
	{
		glDeleteBuffers( 1, &m_buffer );
		m_buffer = 0;

		// We need to disable vertex attrib arrays bound to this VB, otherwise
		// GL will treat previous glVertexAttribPointer calls as specifying raw
		// pointers instead of VB offsets and will crash. We don't know what 
		// attributes are bound to this VB, so we disable everything. This is wrong
		// and needs to be fixed.
		int count;
		CR_GL_RETURN( glGetIntegerv( GL_MAX_VERTEX_ATTRIBS, &count ) );
		for ( int i = 0; i < count; ++i )
		{
			glDisableVertexAttribArray( i );
		}
	}
    m_lockedOffset = 0;
    m_lockedSize = 0;
}

#endif
