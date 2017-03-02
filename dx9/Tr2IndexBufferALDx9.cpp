#include "StdAfx.h"
#include "Tr2IndexBufferALDx9.h"


#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )

#include "ALLog.h"

using namespace Tr2RenderContextEnum;

extern bool g_useManagedDX9Buffers;

Tr2IndexBufferAL::Tr2IndexBufferAL()
	: m_numIndices( 0 )
	, m_is16Bit( false )
	, m_usage( 0 )
	, m_pool( D3DPOOL_DEFAULT )
{
}

Tr2IndexBufferAL::~Tr2IndexBufferAL()
{
}

Tr2IndexBufferAL& Tr2IndexBufferAL::operator=( Tr2IndexBufferAL&& other )
{
	if( this != &other )
	{
		m_buffer.Attach( other.m_buffer.Detach() );

		m_usage			= other.m_usage;
		m_pool			= other.m_pool;

		m_numIndices	= other.m_numIndices;
		m_is16Bit		= other.m_is16Bit;		
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
	m_is16Bit = bitCount == IB_16BIT;
	m_numIndices = numberOfIndices;
	m_usage = usage;

	m_buffer = nullptr;

	if( !ValidateUsage( usage ) )
	{
		CCP_AL_LOGERR( "Invalid combination of USAGE flags passed to Tr2IndexBufferAL Create function" );
		return E_INVALIDARG;
	}

	if( !renderContext.m_d3dDevice9 )
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
	if( usage & USAGE_LOCK_FREQUENTLY )
	{
		dxUsage = D3DUSAGE_DYNAMIC;
	}
	else if( g_useManagedDX9Buffers && !renderContext.m_usingEXDevice && 
		( ( usage & USAGE_CPU_READ ) || ( usage & USAGE_HINT_MANAGED ) ) )
	{
		m_pool = D3DPOOL_MANAGED;
	}
	if( ( usage & USAGE_CPU_READ ) == 0 )
	{
		dxUsage |= D3DUSAGE_WRITEONLY;
	}

	CComPtr<IDirect3DIndexBuffer9> buffer;
	CR_RETURN_HR( renderContext.m_d3dDevice9->CreateIndexBuffer( GetTotalSizeInBytes(), dxUsage, Is16Bit() ? D3DFMT_INDEX16 : D3DFMT_INDEX32, m_pool, &buffer, 0 ) );
	
	if( initialData )
	{
		void* data;
		CR_RETURN_HR( buffer->Lock( 0, 0, &data, 0 ) );
		memcpy( data, initialData, GetTotalSizeInBytes() );
		CR_RETURN_HR( buffer->Unlock() );
	}

	m_buffer = buffer;
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
	uint32_t *&data, 
	LockType lockType, 
	Tr2RenderContextAL & renderContext )
{
	if( m_is16Bit )
	{
		data = nullptr;
		return E_INVALIDARG;
	}
	return Lock( offset, sizeInBytes, (void**)&data, lockType, renderContext );
}

ALResult Tr2IndexBufferAL::Lock(	
	uint32_t offset, 
	uint32_t size, 
	void** data, 
	LockType lockType, 
	Tr2RenderContextAL & /*renderContext*/ )
{
	if( !m_buffer )
	{
		*data = 0;
		return E_FAIL;
	}

	unsigned dxFlags = 0;
	if( lockType == LOCK_READONLY )
	{
		if( ( m_usage & USAGE_CPU_READ ) == 0 && ( m_usage & USAGE_IMMUTABLE ) == 0 )
		{
			return E_FAIL;
		}
		dxFlags = D3DLOCK_READONLY;
	}
	else
	{
		if( ( m_usage & USAGE_CPU_WRITE ) == 0 )
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
			if( m_usage & USAGE_LOCK_FREQUENTLY )
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

ALResult Tr2IndexBufferAL::Unlock( Tr2RenderContextAL & /*renderContext*/ )
{
	return m_buffer ? m_buffer->Unlock() : E_FAIL;
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

bool Tr2IndexBufferAL::IsValid() const
{
	return m_buffer != nullptr;
}

void Tr2IndexBufferAL::Destroy()
{
	m_buffer = nullptr;
}

#endif
