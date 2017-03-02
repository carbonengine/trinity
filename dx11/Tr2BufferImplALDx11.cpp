#include "StdAfx.h"
#include "Tr2BufferImplALDx11.h"


#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2BufferImplAL::Tr2BufferImplAL()	
	: m_currentLock( LOCK_INVALID )
	, m_lengthInBytes( 0 )
	, m_usage( 0 )
{
}

Tr2BufferImplAL::~Tr2BufferImplAL()
{
}

Tr2BufferImplAL& Tr2BufferImplAL::operator=( Tr2BufferImplAL&& other )
{
	if( this != &other )
	{
		m_buffer.Attach( other.m_buffer.Detach() );
		m_lengthInBytes = other.m_lengthInBytes;
		m_usage = other.m_usage;
	}

	return *this;
}

ALResult Tr2BufferImplAL::Create(
	uint32_t lengthInBytes, 
	BufferUsage usage, 
	/*D3D11_BIND_FLAG*/				uint32_t bindFlags,
	/*D3D11_RESOURCE_MISC_FLAG*/	uint32_t miscFlags,
	const void* initialData, 
	Tr2PrimaryRenderContextAL &renderContext )
{
	m_buffer = nullptr;
	m_staging = nullptr;
	m_lengthInBytes = lengthInBytes;
	m_usage = usage;

	if( ( usage & USAGE_IMMUTABLE ) && !initialData )
	{
		CCP_AL_LOGERR( "Create: Trying to create an immutable buffer without providing data" );
		return E_INVALIDARG;
	}

	if( !renderContext.m_d3dDevice11 )
	{
		return E_FAIL;
	}

	if( !ValidateUsage( usage ) )
	{
		CCP_AL_LOGERR( "Invalid combination of USAGE flags passed to Tr2BufferAL Create function" );
		return E_INVALIDARG;
	}

	D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));

	if( usage & USAGE_IMMUTABLE )
	{
		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.CPUAccessFlags = 0;
	}
	else if( usage & USAGE_LOCK_FREQUENTLY )
	{
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else
	{
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.CPUAccessFlags = 0;
	}

    bd.ByteWidth		= lengthInBytes;
    bd.BindFlags		= bindFlags;
	bd.MiscFlags		= miscFlags;

	if( usage & USAGE_SHADER_RESOURCE )
	{
		bd.BindFlags |= D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_SHADER_RESOURCE;
		bd.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	}

	if( usage & USAGE_UNORDERED_ACCESS )
	{
		bd.BindFlags |= D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		bd.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
	}

	D3D11_SUBRESOURCE_DATA initialData11;
	if( initialData )
	{
		initialData11.pSysMem = initialData;
		initialData11.SysMemPitch = 0;
		initialData11.SysMemSlicePitch = 0;
	}
	HRESULT hr = renderContext.m_d3dDevice11->CreateBuffer( &bd, initialData ? &initialData11 : nullptr, &m_buffer );
	if( hr == E_OUTOFMEMORY )
	{
		int retries = 10;
		while( hr == E_OUTOFMEMORY && retries )
		{
			CCP_AL_LOGWARN( "CreateBuffer failed with E_OUTOFMEMORY - retrying after Flush" );
			renderContext.m_context->Flush();
			hr = renderContext.m_d3dDevice11->CreateBuffer( &bd, initialData ? &initialData11 : nullptr, &m_buffer );
			--retries;
		}
	}

	return hr;
}

ALResult Tr2BufferImplAL::Lock( 
	uint32_t offset, 
	uint32_t size, 
	void** data, 
	LockType lockType, 
	Tr2RenderContextAL & renderContext )
{
	if( m_currentLock != LOCK_INVALID )
	{
		CCP_AL_LOGERR( "Tr2BufferImplAL - Attempting to lock already locked buffer" );
		return E_FAIL;
	}

	if( lockType == LOCK_READONLY )
	{
		return LockReading( offset, size, data, renderContext );
	}

	return LockWriting( offset, size, data, lockType, renderContext );
}

ALResult Tr2BufferImplAL::Unlock( Tr2RenderContextAL & renderContext )
{
	switch( m_currentLock )
	{
	case LOCK_READONLY:
		return UnlockReading( renderContext );
	case LOCK_WRITEONLY:
		return UnlockWriting( renderContext );
	case LOCK_NO_OVERWRITE:
		if( renderContext.m_context )
		{
			renderContext.m_context->Unmap( m_buffer, 0 );
		}
		m_currentLock = LOCK_INVALID;
		return S_OK;
	}

	CCP_AL_LOGERR( "Trying to Unlock buffer that's not locked" );
	return E_FAIL;
}

ALResult Tr2BufferImplAL::LockReading(	
	uint32_t /*offset*/, 
	uint32_t /*size*/, 
	void** data, 
	Tr2RenderContextAL & renderContext )
{
	if( !m_buffer || !renderContext.m_context )
	{
		*data = 0;
		return E_FAIL;
	}

	if( !m_staging )
	{		
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));

		bd.Usage			= D3D11_USAGE_STAGING;
		bd.ByteWidth		= m_lengthInBytes;
		bd.BindFlags		= 0;
		bd.CPUAccessFlags	= D3D11_CPU_ACCESS_READ;

		if( !renderContext.m_secondaryDevice11 )
		{
			return E_FAIL;
		}
		CR_RETURN_HR( 
			renderContext.m_secondaryDevice11->CreateBuffer( 
						&bd, 
						nullptr, 
						&m_staging ) );
			
		if( !m_staging )
		{
			return E_FAIL;
		}
	}

	renderContext.m_context->CopyResource( m_staging, m_buffer );
	
	D3D11_MAPPED_SUBRESOURCE ms = { nullptr, 0, 0 };
	HRESULT hr = renderContext.m_context->Map( m_staging, 0, D3D11_MAP_READ, 0, &ms );
	*data = ms.pData;
	if( !ms.pData )
	{
		return E_FAIL;
	}
	if( SUCCEEDED( hr ) )
	{
		m_currentLock = LOCK_READONLY;
	}
    return hr;
}

ALResult Tr2BufferImplAL::UnlockReading( Tr2RenderContextAL & renderContext )
{
	if( renderContext.m_context )
	{
		renderContext.m_context->Unmap( m_staging, 0 );
	}
	m_currentLock = LOCK_INVALID;
	return S_OK;
}

ALResult Tr2BufferImplAL::LockWriting(	
	uint32_t offset, 
	uint32_t /*size*/, 
	void** data, 
	Tr2RenderContextEnum::LockType lockType, 
	Tr2RenderContextAL & renderContext )
{
	if( !m_buffer || !renderContext.m_context )
	{
		*data = 0;
		return E_FAIL;
	}

	if( m_usage & USAGE_LOCK_FREQUENTLY )
	{
		D3D11_MAPPED_SUBRESOURCE ms = { nullptr, 0, 0 };
		D3D11_MAP mapType = lockType == LOCK_NO_OVERWRITE ? D3D11_MAP_WRITE_NO_OVERWRITE : D3D11_MAP_WRITE_DISCARD;
		HRESULT hr = renderContext.m_context->Map( m_buffer, 0, mapType, 0, &ms );
		if( FAILED( hr ) )
		{
			*data = nullptr;
			return hr;
		}
		if( !ms.pData )
		{
			*data = nullptr;
			return E_FAIL;
		}
#if TRINITY_AL_GUARD_LOCKS
		if( lockType == LOCK_NO_OVERWRITE )
		{
			*data = ms.pData;
		}
		else
		{
			m_lockGuard.Lock( m_lengthInBytes, ms.pData );
			*data = m_lockGuard.GetMemory();
		}
#else
		*data = ms.pData;
#endif
		if( lockType == LOCK_NO_OVERWRITE && *data )
		{
			reinterpret_cast<uint8_t*&>( *data ) += offset;
		}
		m_currentLock = lockType;

		return hr;
	}
	if( lockType == LOCK_NO_OVERWRITE )
	{
		// Can't use LOCK_NO_OVERWRITE on non-dynamic buffers
		return E_INVALIDARG;
	}

	if( ( m_usage & Tr2RenderContextEnum::USAGE_CPU_WRITE ) == 0 )
	{
		return E_INVALIDCALL;
	}

	m_writeLockMemory.resize( "m_writeLockMemory", m_lengthInBytes );
	*data = m_writeLockMemory.get();
	m_currentLock = LOCK_WRITEONLY;
	return S_OK;
}

ALResult Tr2BufferImplAL::UnlockWriting( Tr2RenderContextAL & renderContext )
{
	if( m_usage & USAGE_LOCK_FREQUENTLY )
	{
		if( renderContext.m_context )
		{
#if TRINITY_AL_GUARD_LOCKS
			m_lockGuard.Unlock();
#endif
			renderContext.m_context->Unmap( m_buffer, 0 );
		}
	}
	else
	{
		if( renderContext.m_context )
		{
			renderContext.m_context->UpdateSubresource( m_buffer, 0, nullptr, m_writeLockMemory.get(), 0, 0 );
			m_writeLockMemory.clear();
		}
	}
	m_currentLock = LOCK_INVALID;
	return S_OK;
}

ALResult Tr2BufferImplAL::UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext )
{
	if( !renderContext.IsValid() || !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( offset + size > m_lengthInBytes )
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
	D3D11_BOX box = { 0, 0, 0, size, 1, 1 };
	renderContext.m_context->UpdateSubresource( m_buffer, 0, &box, data, 0, 0 );
	return S_OK;
}

void Tr2BufferImplAL::Destroy()
{
	if( m_currentLock != LOCK_INVALID )
	{
		CCP_AL_LOGERR( "Attempting to destroy a locked buffer" );
	}

	m_currentLock = LOCK_INVALID;
	m_buffer = nullptr;
	m_staging = nullptr;
}

#endif
