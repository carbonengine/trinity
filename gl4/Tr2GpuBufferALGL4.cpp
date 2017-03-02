#include "StdAfx.h"
#include "Tr2GpuBufferALGL4.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGL4 )

using namespace Tr2RenderContextEnum;

namespace
{

struct OnDeleteBuffer
{
	void operator()( GLuint* object )
	{
		if ( object && *object )
		{
			glDeleteBuffers( 1, object );
		}
	}
};

struct OnDeleteTexture
{
	void operator()( GLuint* object )
	{
		if ( object && *object )
		{
			glDeleteTextures( 1, object );
		}
	}
};

}

Tr2GpuBufferAL::Tr2GpuBufferAL()
	:m_numElements( 0 ),
	m_elementSize( 0 ),
	m_format( PIXEL_FORMAT_UNKNOWN ),
	m_usage( 0 ),
	m_clObject( nullptr )
{
}
	
ALResult Tr2GpuBufferAL::Create(			
	uint32_t numberOfElements, 
	Tr2RenderContextEnum::PixelFormat format, 
	Tr2RenderContextEnum::BufferUsage usage,
	const void* initialData, 
	Tr2RenderContextAL & renderContext ) 
{ 
	return CreateEx( numberOfElements, format, usage, initialData, 0, renderContext );
}	

ALResult Tr2GpuBufferAL::CreateEx(			
	uint32_t numberOfElements, 
	Tr2RenderContextEnum::PixelFormat format, 
	Tr2RenderContextEnum::BufferUsage usage,
	const void* initialData, 
	uint32_t exFlags,
	Tr2RenderContextAL & renderContext ) 
{ 
	Destroy();
	GLenum internalFormat;
	GLenum targetFormat;
	GLenum targetType;
	if ( !Tr2RenderContextAL::ConvertToGLPixelFormat( format, internalFormat, targetFormat, targetType ) )
	{
		return E_INVALIDARG;
	}

	CR_RETURN_HR( Create( numberOfElements, GetBytesPerPixel( format ), internalFormat, format, usage, initialData, exFlags, renderContext ) );
	m_elementSize = 0;
	return S_OK;
}	

ALResult Tr2GpuBufferAL::CreateStructured(	
	uint32_t numberOfElements, 
	uint32_t elementSize, 
	Tr2RenderContextEnum::BufferUsage usage,
	Tr2RenderContextEnum::GpuBufferUsage gpuBufferUsage,
	const void* initialData, 
	Tr2RenderContextAL & renderContext )
{ 
	Destroy();

	if( elementSize % 4 )
	{
		return E_INVALIDARG;
	}

	GLenum internalFormat;
	GLenum targetFormat;
	GLenum targetType;
	if ( !Tr2RenderContextAL::ConvertToGLPixelFormat( PIXEL_FORMAT_R32_SINT, internalFormat, targetFormat, targetType ) )
	{
		return E_INVALIDARG;
	}

	return Create( numberOfElements, elementSize, internalFormat, PIXEL_FORMAT_UNKNOWN, usage, initialData, 0, renderContext );
}

ALResult Tr2GpuBufferAL::Create(			
	uint32_t numberOfElements, 
	uint32_t elementSize, 
	GLenum internalFormat,
	Tr2RenderContextEnum::PixelFormat format, 
	Tr2RenderContextEnum::BufferUsage usage,
	const void* initialData, 
	uint32_t exFlags,
	Tr2RenderContextAL & renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_INVALIDCALL;
	}

	if( ( usage & USAGE_IMMUTABLE ) && !initialData )
	{
		return E_INVALIDARG;
	}

	GLuint buffer = 0;
	GL_FAIL( glGenBuffers( 1, &buffer ) );
	if( !buffer )
	{
		return E_FAIL;
	}

	GL_FAIL( glBindBuffer( GL_TEXTURE_BUFFER, buffer ) );
	GL_VALIDATE( glBufferData( GL_TEXTURE_BUFFER, elementSize * numberOfElements, initialData, GL_STATIC_DRAW ) );
	GL_FAIL( glBindBuffer( GL_TEXTURE_BUFFER, 0 ) );

	GLuint texture = 0;
	GL_FAIL( glGenTextures( 1, &texture ) );
	if( !texture )
	{
		return E_FAIL;
	}

	GL_FAIL( glBindTexture( GL_TEXTURE_BUFFER, texture ) );
	GL_VALIDATE( glTexBuffer( GL_TEXTURE_BUFFER, internalFormat, buffer ) );
	GL_FAIL( glBindTexture( GL_TEXTURE_BUFFER, 0 ) );

	m_buffer = std::shared_ptr<GLuint>( new GLuint( buffer ), OnDeleteBuffer() );
	m_texture = std::shared_ptr<GLuint>( new GLuint( texture ), OnDeleteTexture() );
	m_format = format;
	m_numElements = numberOfElements;
	m_elementSize = elementSize;
	m_usage = usage;

	return S_OK; 
}


ALResult Tr2GpuBufferAL::CreateVbView(		
	Tr2VertexBufferAL& vb,
	bool gpuWritable,
	Tr2PrimaryRenderContextAL & renderContext )
{ 
	return E_FAIL; 
}

ALResult Tr2GpuBufferAL::CreateAlias(		
	Tr2GpuBufferAL& other, 
	Tr2RenderContextEnum::PixelFormat format, 
	Tr2RenderContextAL & renderContext )
{ 
	Destroy();
	if( !other.IsValid() || other.m_elementSize != 0 )
	{
		return E_INVALIDARG;
	}

	GLenum internalFormat;
	GLenum targetFormat;
	GLenum targetType;
	if ( !Tr2RenderContextAL::ConvertToGLPixelFormat( format, internalFormat, targetFormat, targetType ) )
	{
		return E_INVALIDARG;
	}

	if( GetBytesPerPixel( format ) != GetBytesPerPixel( other.GetFormat() ) )
	{
		return E_INVALIDARG;
	}

	GLuint texture = 0;
	GL_FAIL( glGenTextures( 1, &texture ) );
	if( !texture )
	{
		return E_FAIL;
	}

	GL_FAIL( glBindTexture( GL_TEXTURE_BUFFER, texture ) );
	GL_VALIDATE( glTexBuffer( GL_TEXTURE_BUFFER, internalFormat, *other.m_buffer ) );
	GL_FAIL( glBindTexture( GL_TEXTURE_BUFFER, 0 ) );

	m_buffer = other.m_buffer;
	m_texture = std::shared_ptr<GLuint>( new GLuint( texture ), OnDeleteTexture() );
	m_format = format;
	m_numElements = other.m_numElements;
	m_elementSize = other.m_elementSize;
	m_usage = other.m_usage;
	
	return S_OK; 
}

ALResult Tr2GpuBufferAL::Lock(				
	uint32_t offset, 
	uint32_t size, 
	void** data, 
	Tr2RenderContextEnum::LockType lockType, 
	Tr2RenderContextAL & renderContext )
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

	if( lockType == LOCK_NO_OVERWRITE && ( m_usage & USAGE_LOCK_FREQUENTLY ) == 0 )
	{
		return E_INVALIDARG;
	}
	GL_FAIL( glBindBuffer( GL_TEXTURE_BUFFER, *m_buffer ) );
	if( lockType == LOCK_NO_OVERWRITE )
	{
		*data = nullptr;
		GL_FAIL( *data = glMapBufferRange( GL_TEXTURE_BUFFER, offset, size, GL_MAP_UNSYNCHRONIZED_BIT | GL_MAP_WRITE_BIT ) );
		if( !*data )
		{
			return E_FAIL;
		}
	}
	else
	{
		*data = nullptr;
		GL_FAIL( *data = glMapBufferRange( GL_TEXTURE_BUFFER, offset, size ? size : ( GetTotalSizeInBytes() - offset ), lockType == LOCK_READONLY ? GL_MAP_READ_BIT : GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_WRITE_BIT ) );
		if( !*data )
		{
			return E_FAIL;
		}
	}

	return S_OK;
}

ALResult Tr2GpuBufferAL::Unlock( Tr2RenderContextAL & renderContext ) 
{ 
	if ( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	if ( !m_buffer )
	{
		return E_FAIL;
	}
	GL_FAIL( glBindBuffer( GL_ARRAY_BUFFER, *m_buffer ) );
	GL_FAIL( glUnmapBuffer( GL_ARRAY_BUFFER ) );

	return S_OK;
}

bool Tr2GpuBufferAL::IsValid() const 
{ 
	return m_texture != nullptr; 
}

void Tr2GpuBufferAL::Destroy() 
{
	m_buffer = nullptr;
	m_texture = nullptr;
	m_numElements = 0;
	m_elementSize = 0;
	m_format = PIXEL_FORMAT_UNKNOWN;
	if( m_clObject )
	{
		clReleaseMemObject( m_clObject );
		m_clObject = nullptr;
	}
}

unsigned Tr2GpuBufferAL::BytesPerElement() const		
{ 
	return m_elementSize ? m_elementSize : GetBytesPerPixel( m_format ); 
}

unsigned Tr2GpuBufferAL::GetNumElements() const			
{ 
	return m_numElements; 
}

unsigned Tr2GpuBufferAL::GetTotalSizeInBytes() const	
{ 
	return GetNumElements() * BytesPerElement(); 
}

Tr2RenderContextEnum::PixelFormat Tr2GpuBufferAL::GetFormat() const 
{ 
	return m_format; 
}

Tr2RenderContextEnum::GpuBufferUsage Tr2GpuBufferAL::GetGpuBufferUsage() const
{
	return 0;
}

Tr2ALMemoryType Tr2GpuBufferAL::GetMemoryClass() const 
{ 
	return AL_MEMORY_MANAGED; 
}

ALResult Tr2GpuBufferAL::CopySubBuffer( 
	uint32_t offset, 
	uint32_t length, 
	Tr2GpuBufferAL& dest, 
	uint32_t destOffset, 
	Tr2RenderContextAL& renderContext )
{
	if ( !renderContext.IsValid() )
	{
		return E_INVALIDCALL;
	}

	if ( !IsValid() || !dest.IsValid() )
	{
		return E_INVALIDARG;
	}

	GL_FAIL( glBindBuffer( GL_COPY_READ_BUFFER, *m_buffer ) );
	GL_FAIL( glBindBuffer( GL_COPY_WRITE_BUFFER, *dest.m_buffer ) );
	GL_FAIL( glCopyBufferSubData( GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, offset, destOffset, length ) );
	GL_FAIL( glBindBuffer( GL_COPY_READ_BUFFER, 0 ) );
	GL_FAIL( glBindBuffer( GL_COPY_WRITE_BUFFER, 0 ) );

	return S_OK;
}

#endif