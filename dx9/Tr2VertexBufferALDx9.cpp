#include "StdAfx.h"
#include "Tr2VertexBufferALDx9.h"


#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "ALLog.h"

using namespace Tr2RenderContextEnum;

extern bool g_useManagedDX9Buffers;

Tr2VertexBufferAL::Tr2VertexBufferAL()
: m_lengthInBytes( 0 ),
  m_usage( 0 ),
  m_pool( D3DPOOL_DEFAULT )
{
}

Tr2VertexBufferAL::~Tr2VertexBufferAL()
{
}

Tr2VertexBufferAL& Tr2VertexBufferAL::operator=( Tr2VertexBufferAL&& other )
{
	if ( this != &other )
	{
		m_lengthInBytes = other.m_lengthInBytes;
		m_usage = other.m_usage;
		m_pool = other.m_pool;
		m_buffer.Attach( other.m_buffer.Detach() );
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
	m_lengthInBytes = lengthInBytes;
	m_usage = usage;

	m_buffer = nullptr;

	if ( !ValidateUsage( usage ) )
	{
		CCP_AL_LOGERR( "Invalid combination of USAGE flags passed to Tr2VertexBufferAL Create function" );
		return E_INVALIDARG;
	}

	if ( !renderContext.m_d3dDevice9 )
	{
		return E_FAIL;
	}

	if( ( usage & USAGE_IMMUTABLE ) && !initialData )
	{
		CCP_AL_LOGERR( "Create: Trying to create an immutable buffer without providing data" );
		return E_INVALIDARG;
	}

	unsigned dxUsage = 0;
	m_pool = D3DPOOL_DEFAULT;
	if ( usage & USAGE_LOCK_FREQUENTLY )
	{
		dxUsage = D3DUSAGE_DYNAMIC;
	}
	else if ( g_useManagedDX9Buffers && !renderContext.m_usingEXDevice &&
			  ( ( usage & USAGE_CPU_READ ) || ( usage & USAGE_HINT_MANAGED ) ) )
	{
		m_pool = D3DPOOL_MANAGED;
	}
	if ( ( usage & USAGE_CPU_READ ) == 0 )
	{
		dxUsage |= D3DUSAGE_WRITEONLY;
	}

	CComPtr<IDirect3DVertexBuffer9> buffer;
	CR_RETURN_HR( renderContext.m_d3dDevice9->CreateVertexBuffer( lengthInBytes, dxUsage, 0, m_pool, &buffer, 0 ) );

	if ( initialData )
	{
		void* data;
		CR_RETURN_HR( buffer->Lock( 0, 0, &data, 0 ) );
		memcpy( data, initialData, lengthInBytes );
		CR_RETURN_HR( buffer->Unlock() );
	}

	m_buffer = buffer;
	ChangeObjectId();
	return S_OK;
}

ALResult Tr2VertexBufferAL::Create( uint32_t lengthInBytes,
									Tr2RenderContextAL& renderContext )
{
	return Create( lengthInBytes, USAGE_CPU_READ | USAGE_CPU_WRITE, nullptr, renderContext );
}

ALResult Tr2VertexBufferAL::Lock( uint32_t offset,
								  uint32_t size,
								  void** data,
								  Tr2RenderContextEnum::LockType lockType,
								  Tr2RenderContextAL& /*renderContext*/ )
{
	if ( !m_buffer )
	{
		*data = 0;
		return E_FAIL;
	}

	unsigned dxFlags = 0;
	if ( lockType == LOCK_READONLY )
	{
		if ( ( m_usage & USAGE_CPU_READ ) == 0 && ( m_usage & USAGE_IMMUTABLE ) == 0 )
		{
			return E_FAIL;
		}
		dxFlags = D3DLOCK_READONLY;
	}
	else
	{
		if ( ( m_usage & USAGE_CPU_WRITE ) == 0 )
		{
			return E_FAIL;
		}
		if( lockType == LOCK_NO_OVERWRITE )
		{
			if( m_usage & USAGE_LOCK_FREQUENTLY )
			{
				dxFlags = D3DLOCK_NOOVERWRITE;
			}
			else
			{
				return E_INVALIDARG;
			}
		}
		else
		{
			if ( m_usage & USAGE_LOCK_FREQUENTLY )
			{
				dxFlags = D3DLOCK_DISCARD;
			}
		}
	}

	*data = nullptr;
	CR_RETURN_HR( m_buffer->Lock( offset, size, data, dxFlags ) );
	if( *data == nullptr )
	{
		return E_FAIL;
	}
	return S_OK;
}

ALResult Tr2VertexBufferAL::Unlock( Tr2RenderContextAL& /*renderContext*/ )
{
	return m_buffer ? m_buffer->Unlock() : E_FAIL;
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

	void* lockedData = nullptr;
	CR_RETURN_HR( m_buffer->Lock( offset, size, &lockedData, 0 ) );
	if( !lockedData )
	{
		m_buffer->Unlock();
		return E_FAIL;
	}
	memcpy( lockedData, data, size );
	return m_buffer->Unlock();
}

bool Tr2VertexBufferAL::IsValid() const
{
	return m_buffer != nullptr;
}

void Tr2VertexBufferAL::Destroy()
{
	m_buffer = nullptr;
}

#endif
