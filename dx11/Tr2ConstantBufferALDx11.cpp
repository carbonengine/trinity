#include "StdAfx.h"
#include "Tr2ConstantBufferALDx11.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "ALLog.h"

using namespace Tr2RenderContextEnum;

namespace
{

const BufferUsage s_defaultUsage = USAGE_CPU_WRITE;

}

Tr2ConstantBufferAL::Tr2ConstantBufferAL()
	: m_size( 0 )
	, m_frameUse( FRAME_USE_NOT_USED_YET )
	, m_usage( s_defaultUsage )
{
}

Tr2ConstantBufferAL::~Tr2ConstantBufferAL()
{
}

Tr2ConstantBufferAL::Tr2ConstantBufferAL( Tr2ConstantBufferAL&& other )
	: m_buffer( std::move( other.m_buffer ) )
	, m_bufferMirror( std::move( other.m_bufferMirror ) )
	, m_size( other.m_size )
	, m_usage( other.m_usage )
	, m_frameUse( other.m_frameUse )
{}

Tr2ConstantBufferAL& Tr2ConstantBufferAL::operator=( Tr2ConstantBufferAL&& other )
{
	if( this != &other )
	{
		m_buffer.Attach( other.m_buffer.Detach() );
		m_size = other.m_size;
		m_frameUse = other.m_frameUse;
		m_usage = other.m_usage;
	}

	return *this;
}

ALResult Tr2ConstantBufferAL::Create( uint32_t size, Tr2RenderContextEnum::BufferUsage usage, const void* initialData, Tr2PrimaryRenderContextAL & renderContext )
{
	m_buffer = nullptr;
	m_size = 0;
	m_usage = s_defaultUsage;
	m_bufferMirror.clear();

	if( !renderContext.m_d3dDevice11 )
	{
		return E_FAIL;
	}

	if( ( usage & USAGE_IMMUTABLE ) && !initialData )
	{
		CCP_AL_LOGERR( "Create: Trying to create an immutable buffer without providing data" );
		return E_INVALIDARG;
	}

	if( ( usage & USAGE_LOCK_FREQUENTLY ) != 0 )
	{
		m_bufferMirror.resize( "Tr2ConstantBufferAL.m_bufferMirror", size );
		if( initialData )
		{
			memcpy( m_bufferMirror.get(), initialData, size );
		}

		m_size = size;
		m_frameUse = FRAME_USE_NOT_USED_YET;
		m_usage = usage;
		ChangeObjectId();

		return S_OK;
	}
	else
	{
		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));

		bd.ByteWidth		= size;
		bd.BindFlags		= D3D11_BIND_CONSTANT_BUFFER;
		if( usage & USAGE_IMMUTABLE )
		{
			bd.Usage			= D3D11_USAGE_IMMUTABLE;
		}
		else if( usage & USAGE_CPU_WRITE )
		{
			bd.Usage			= D3D11_USAGE_DYNAMIC;
			bd.CPUAccessFlags	= D3D11_CPU_ACCESS_WRITE;
		}
		else
		{
			bd.Usage			= D3D11_USAGE_DEFAULT;
			bd.CPUAccessFlags	= 0;
		}

		D3D11_SUBRESOURCE_DATA data;
		if( initialData )
		{
			data.pSysMem = initialData;
			data.SysMemPitch = 0;
			data.SysMemSlicePitch = 0;
		}
		CR_RETURN_HR( renderContext.m_d3dDevice11->CreateBuffer( &bd, initialData ? &data : nullptr, &m_buffer ) );

		m_size = size;
		m_frameUse = FRAME_USE_NOT_USED_YET;
		m_usage = usage;
		ChangeObjectId();

		return S_OK;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Lock the constant buffer; this discard its contents, so you must rewrite the entire
//   buffer.  If you want contents to persist so you can make incremental changes, use
//   GetBufferMirror.
//   You cannot mix-and-match use of Lock+memcpy+Unlock and GetBufferMirror+UpdateFromMirror.
// --------------------------------------------------------------------------------------
ALResult Tr2ConstantBufferAL::Lock( void** data, Tr2RenderContextAL & renderContext )
{
	bool isDynamic = ( m_usage & USAGE_LOCK_FREQUENTLY ) != 0;

	if( isDynamic )
	{
		if( !m_bufferMirror )
		{
			*data = 0;
			return E_FAIL;
		}
	}
	else
	{
		if( !m_buffer )
		{
			*data = 0;
			return E_FAIL;
		}
	}

	CCP_ASSERT( m_frameUse != FRAME_USE_MIRRORED );	// if this fires, you've used GetBufferMirror, and should call UpdateFromMirror.
	CCP_ASSERT( m_frameUse != FRAME_USE_LOCKING  );	// if this fires, you've already locked this CB without then having bound it to the device.
													// Since CBs are always DISCARD, whatever you did in that previous Lock will be lost in this
													// one, so that's almost certainly not what you want.
	m_frameUse = FRAME_USE_LOCKING;

	if( isDynamic )
	{
		*data = m_bufferMirror.get();
		return S_OK;
	}
	else
	{
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
}

ALResult Tr2ConstantBufferAL::Unlock( Tr2RenderContextAL & renderContext )
{
	CCP_ASSERT( m_frameUse != FRAME_USE_MIRRORED );	// if this fires, you've used GetBufferMirror, and should call UpdateFromMirror.
	CCP_ASSERT( m_frameUse == FRAME_USE_LOCKING	 );	// if this fires, you didn't call Lock (or it didn't work and you ignored HR).

	if( ( m_usage & USAGE_LOCK_FREQUENTLY ) != 0 )
	{
		if( !m_bufferMirror )
		{
			CCP_ASSERT( false && "Unlocking a null buffer" );
			return E_FAIL;
		}

		return S_OK;
	}
	else
	{
		if( !m_buffer )
		{
			CCP_ASSERT( false && "Unlocking a null buffer" );
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
	m_usage = s_defaultUsage;
	m_size = 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Allocate and return a CPU-side memory buffer, that will persist its contents. Use this
//   to make incremental changes to CPU-side data, and copy it over to the GPU buffer with
//   UpdateFromMirror.
//   You cannot mix-and-match use of Lock+memcpy+Unlock and GetBufferMirror+UpdateFromMirror.
// Arguments:
//   minimumSize - if not zero, the buffer will automaticalyl recreate itself to guarantee at least this many bytes.
// See Also:
//   UpdateFromMirror
// --------------------------------------------------------------------------------------
void* Tr2ConstantBufferAL::GetBufferMirror( uint32_t minimumSize, Tr2RenderContextAL& /*renderContext*/ )
{
	using namespace Tr2RenderContextEnum;

	if( minimumSize > GetSize() )
	{
		if( FAILED( Create( minimumSize, m_usage, nullptr, Tr2RenderContextAL::GetPrimaryRenderContext() ) ) )
		{
			return nullptr;
		}
	}

	if( m_size == 0 )
	{
		return nullptr;
	}

	if( m_bufferMirror.size() < m_size )
	{
		m_bufferMirror.resize( "Tr2ConstantBufferAL::m_bufferMirror", m_size );
	}

	CCP_ASSERT( m_frameUse != FRAME_USE_LOCKING );	// if this fires, you've used Lock, and should call Unlock.
	m_frameUse = FRAME_USE_MIRRORED;

	return m_bufferMirror.get();
}

// --------------------------------------------------------------------------------------
// Description:
//   Update the GPU buffer with the CPU side mirror.
//   You cannot mix-and-match use of Lock+memcpy+Unlock and GetBufferMirror+UpdateFromMirror.
// See Also:
//   GetBufferMirror
// --------------------------------------------------------------------------------------
ALResult Tr2ConstantBufferAL::UpdateFromMirror( Tr2RenderContextAL & renderContext )
{
	if( m_usage & USAGE_IMMUTABLE )
	{
		return E_INVALIDCALL;
	}		

	bool isDynamic = ( m_usage & USAGE_LOCK_FREQUENTLY ) != 0;
	if( !isDynamic && !m_buffer || m_bufferMirror.size() < m_size || m_bufferMirror.empty() )
	{
		return E_FAIL;
	}

	CCP_ASSERT( m_frameUse != FRAME_USE_LOCKING );	// if this fires, you've used Lock, and should call Unlock.
	m_frameUse = FRAME_USE_MIRRORED;	// might not be set yet -- which is fine, the mirror persists across frames (but the CB itself might be shared)

	if( isDynamic )
	{
		return S_OK;
	}

	if( m_usage & USAGE_CPU_WRITE )
	{
		D3D11_MAPPED_SUBRESOURCE ms = { nullptr, 0, 0 };
		ms.pData = nullptr;
		HRESULT hr = renderContext.m_context->Map( m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms );
		CR_RETURN_HR( hr );
		if( !ms.pData )
		{
			return E_FAIL;
		}
		memcpy( ms.pData, m_bufferMirror.get(), m_size );
		renderContext.m_context->Unmap( m_buffer, 0 );
		return hr;
	}
	else
	{
		renderContext.m_context->UpdateSubresource( m_buffer, 0, nullptr, m_bufferMirror.get(), 0, 0 );
		return S_OK;
	}
}

#endif