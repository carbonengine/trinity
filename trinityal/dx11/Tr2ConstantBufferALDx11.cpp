// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if ( TRINITY_PLATFORM == TRINITY_DIRECTX11 )

#include "Tr2ConstantBufferALDx11.h"
#include "Tr2PrimaryRenderContextDx11.h"
#include "ALLog.h"

namespace TrinityALImpl
{

Tr2ConstantBufferAL::Tr2ConstantBufferAL() :
	m_size( 0 ),
	m_usage( Tr2ConstantUsageAL::REUSABLE )
{
}

ALResult Tr2ConstantBufferAL::Create( uint32_t size, Tr2ConstantUsageAL::Type usage, const void* initialData, Tr2PrimaryRenderContextAL& renderContext )
{
	Destroy();

	if( !renderContext.m_d3dDevice11 )
	{
		return E_FAIL;
	}

	if( usage == Tr2ConstantUsageAL::IMMUTABLE && !initialData )
	{
		CCP_AL_LOGERR( "Create: Trying to create an immutable buffer without providing data" );
		return E_INVALIDARG;
	}

	if( usage == Tr2ConstantUsageAL::ONE_SHOT )
	{
		m_bufferMirror.resize( "Tr2ConstantBufferAL.m_bufferMirror", size );
		if( initialData )
		{
			memcpy( m_bufferMirror.get(), initialData, size );
		}

		m_size = size;
		m_usage = usage;

		return S_OK;
	}
	else
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory( &bd, sizeof( bd ) );

		bd.ByteWidth = size;
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		D3D11_SUBRESOURCE_DATA data;
		if( initialData )
		{
			data.pSysMem = initialData;
			data.SysMemPitch = 0;
			data.SysMemSlicePitch = 0;
		}
		auto hr = renderContext.m_d3dDevice11->CreateBuffer( &bd, initialData ? &data : nullptr, &m_buffer );
		if( FAILED( hr ) )
		{
			return hr;
		}

		m_size = size;
		m_usage = usage;

		return S_OK;
	}
}

ALResult Tr2ConstantBufferAL::Lock( void** data, Tr2RenderContextAL& renderContext )
{
	if( m_usage == Tr2ConstantUsageAL::ONE_SHOT )
	{
		if( !m_bufferMirror )
		{
			*data = 0;
			return E_FAIL;
		}
		*data = m_bufferMirror.get();
		return S_OK;
	}

	if( !m_buffer )
	{
		*data = 0;
		return E_FAIL;
	}
	D3D11_MAPPED_SUBRESOURCE ms = { nullptr, 0, 0 };
	HRESULT hr = renderContext.m_context->Map( m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms );
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
#ifdef TRINITY_AL_GUARD_LOCKS
	m_lockGuard.Lock( m_size, ms.pData );
	*data = m_lockGuard.GetMemory();
#else
	*data = ms.pData;
#endif
	return hr;
}

ALResult Tr2ConstantBufferAL::Unlock( Tr2RenderContextAL& renderContext )
{
	if( m_usage == Tr2ConstantUsageAL::ONE_SHOT )
	{
		if( !m_bufferMirror )
		{
			CCP_ASSERT_M( false, "Unlocking a null buffer" );
			return E_FAIL;
		}

		return S_OK;
	}
	else
	{
		if( !m_buffer )
		{
			CCP_ASSERT_M( false, "Unlocking a null buffer" );
			return E_FAIL;
		}

#ifdef TRINITY_AL_GUARD_LOCKS
		m_lockGuard.Unlock();
#endif
		renderContext.m_context->Unmap( m_buffer, 0 );
	}
	return S_OK;
}

void Tr2ConstantBufferAL::Destroy()
{
	m_buffer = nullptr;
	m_bufferMirror.clear();
	m_usage = Tr2ConstantUsageAL::REUSABLE;
	m_size = 0;
}

bool Tr2ConstantBufferAL::IsValid() const
{
	return m_size > 0 && ( m_usage == Tr2ConstantUsageAL::ONE_SHOT || m_buffer != nullptr );
}

Tr2ConstantUsageAL::Type Tr2ConstantBufferAL::GetUsage() const
{
	return m_usage;
}

uint32_t Tr2ConstantBufferAL::GetSize() const
{
	return m_size;
}

Tr2ALMemoryType Tr2ConstantBufferAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}

void Tr2ConstantBufferAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2ConstantBufferAL";
	description["usage"] = std::to_string( int( m_usage ) );
	description["size"] = std::to_string( m_size );
	description["name"] = m_name;
}

ALResult Tr2ConstantBufferAL::SetName( const char* name )
{
	m_name = name;
	SetDebugName( m_buffer, name );
	return S_OK;
}
}
#endif