#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_OPENGLES2

#include "Tr2BufferALGLES2.h"
#include "Tr2RenderContextGLES2.h"

namespace
{
	template <typename T>
	bool HasFlag( T value, T flag )
	{
		return ( value & flag ) == flag;
	}

}

namespace TrinityALImpl
{
	Tr2BufferAL::Tr2BufferAL()
		:m_buffer( 0 )
	{
	}

	Tr2BufferAL::~Tr2BufferAL()
	{
		Destroy();
	}

	ALResult Tr2BufferAL::Create(
		const Tr2BufferDescriptionAL& desc,
		const void* initialData,
		Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( ( desc.gpuUsage & ( ~Tr2GpuUsage::VERTEX_BUFFER & ~Tr2GpuUsage::INDEX_BUFFER ) ) != 0 )
		{
			return E_INVALIDARG;
		}

		if( HasFlag( desc.gpuUsage, Tr2GpuUsage::VERTEX_BUFFER ) && HasFlag( desc.gpuUsage, Tr2GpuUsage::INDEX_BUFFER ) )
		{
			return E_INVALIDARG;
		}

#ifdef TRINITY_AL_MOBILE
		if( HasFlag( desc.cpuUsage, Tr2CpuUsage::READ ) )
		{
			return E_INVALIDARG;
		}
#endif

		if( desc.count == 0 )
		{
			return E_INVALIDARG;
		}

		if( !HasFlag( desc.cpuUsage, Tr2CpuUsage::WRITE ) && !initialData )
		{
			return E_INVALIDARG;
		}

		if( !renderContext.IsValid() )
		{
			return E_INVALIDCALL;
		}

		auto target = HasFlag( desc.gpuUsage, Tr2GpuUsage::VERTEX_BUFFER ) ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER;

		GL_FAIL( glGenBuffers( 1, &m_buffer ) );
		GL_FAIL( glBindBuffer( target, m_buffer ) );

		auto glUsage = HasFlag( desc.cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;

		GL_FAIL( glBufferData( target, desc.stride * desc.count, initialData, glUsage ) );

		m_memory.Set( Tr2MemoryCounterAL::BUFFER, desc.stride * desc.count );
		m_desc = desc;
		return S_OK;
	}

	void Tr2BufferAL::Destroy()
	{
		m_memory.Reset();
		if( m_buffer )
		{
			glDeleteBuffers( 1, &m_buffer );
			m_buffer = 0;

			if( HasFlag( m_desc.gpuUsage, Tr2GpuUsage::VERTEX_BUFFER ) )
			{
				// We need to disable vertex attrib arrays bound to this VB, otherwise
				// GL will treat previous glVertexAttribPointer calls as specifying raw
				// pointers instead of VB offsets and will crash. We don't know what 
				// attributes are bound to this VB, so we disable everything. This is wrong
				// and needs to be fixed.
				int count;
				CR_GL_RETURN( glGetIntegerv( GL_MAX_VERTEX_ATTRIBS, &count ) );
				for( int i = 0; i < count; ++i )
				{
					glDisableVertexAttribArray( i );
			}
		}
		}
#ifdef TRINITY_AL_MOBILE
		m_lockedData.clear();
#endif
	}

	bool Tr2BufferAL::IsValid() const
	{
		return m_buffer != 0;
	}

	Tr2ALMemoryType Tr2BufferAL::GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	const Tr2BufferDescriptionAL& Tr2BufferAL::GetDesc() const
	{
		return m_desc;
	}

	ALResult Tr2BufferAL::MapForReading( const void*& data, Tr2RenderContextAL& renderContext )
	{
		if( !renderContext.IsValid() || !IsValid() )
		{
			data = nullptr;
			return E_INVALIDCALL;
		}
		if( !HasFlag( m_desc.cpuUsage, Tr2CpuUsage::READ ) )
		{
			return E_INVALIDCALL;
		}

		auto target = HasFlag( m_desc.gpuUsage, Tr2GpuUsage::VERTEX_BUFFER ) ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER;

		GL_FAIL( glBindBuffer( target, m_buffer ) );
		GL_FAIL( data = glMapBuffer( target, GL_READ_ONLY ) );
		if( !data )
		{
			return E_FAIL;
		}
		return S_OK;
	}

	void Tr2BufferAL::UnmapForReading( Tr2RenderContextAL& renderContext )
	{
		if( !renderContext.IsValid() || !IsValid() )
		{
			return;
		}
		if( !HasFlag( m_desc.cpuUsage, Tr2CpuUsage::READ ) )
		{
			return;
		}

		auto target = HasFlag( m_desc.gpuUsage, Tr2GpuUsage::VERTEX_BUFFER ) ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER;

		CR_GL_RETURN( glBindBuffer( target, m_buffer ) );
		CR_GL_RETURN( glUnmapBuffer( target ) );
	}

	ALResult Tr2BufferAL::MapForWriting( void*& data, Tr2LockType::Type, Tr2RenderContextAL& renderContext )
	{
		if( !renderContext.IsValid() || !IsValid() )
		{
			data = nullptr;
			return E_INVALIDCALL;
		}
		if( !HasFlag( m_desc.cpuUsage, Tr2CpuUsage::WRITE ) )
		{
			return E_INVALIDCALL;
		}

#ifdef TRINITY_AL_MOBILE
		m_lockedData.resize( "Tr2BufferAL::MapForWriting", m_desc.count * m_desc.stride );
		data = m_lockedData.get();
#else
		auto target = HasFlag( m_desc.gpuUsage, Tr2GpuUsage::VERTEX_BUFFER ) ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER;

		GL_FAIL( glBindBuffer( target, m_buffer ) );
		GL_FAIL( data = glMapBuffer( target, GL_WRITE_ONLY ) );
#endif
		if( !data )
		{
			return E_FAIL;
		}
		return S_OK;
	}

	void Tr2BufferAL::UnmapForWriting( Tr2RenderContextAL& renderContext )
	{
		if( !renderContext.IsValid() || !IsValid() )
		{
			return;
		}
		if( !HasFlag( m_desc.cpuUsage, Tr2CpuUsage::WRITE ) )
		{
			return;
		}

		auto target = HasFlag( m_desc.gpuUsage, Tr2GpuUsage::VERTEX_BUFFER ) ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER;

		CR_GL_RETURN( glBindBuffer( target, m_buffer ) );
#ifdef TRINITY_AL_MOBILE
		CR_GL_RETURN( glBufferSubData( target, 0, m_desc.count * m_desc.stride, m_lockedData.get() ) );
#else
		CR_GL_RETURN( glUnmapBuffer( target ) );
#endif
	}

	ALResult Tr2BufferAL::UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext )
	{
		if( !renderContext.IsValid() || !IsValid() )
		{
			return E_INVALIDCALL;
		}
		if( offset + size > m_desc.count * m_desc.stride )
		{
			return E_INVALIDARG;
		}
		if( !HasFlag( m_desc.cpuUsage, Tr2CpuUsage::WRITE ) || HasFlag( m_desc.cpuUsage, Tr2CpuUsage::WRITE_OFTEN ) )
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

	void Tr2BufferAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2BufferAL";
		description["size"] = std::to_string( (long long)( GetDesc().count * GetDesc().stride ) );
	}
}

#endif