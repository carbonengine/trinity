#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2ConstantBufferALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "Utilities.h"


namespace
{
	const uint32_t MAX_CB_SIZE = 4 * 1024 * 1024;
}

Tr2ConstantBufferAL::Tr2ConstantBufferAL()
	:m_owner( nullptr ),
	m_size( 0 )
{
}

Tr2ConstantBufferAL::~Tr2ConstantBufferAL()
{
	Destroy();
}

ALResult Tr2ConstantBufferAL::Create( uint32_t size, Tr2PrimaryRenderContextAL & renderContext )
{
	return Create( size, Tr2RenderContextEnum::USAGE_CPU_WRITE, nullptr, renderContext );
}

ALResult Tr2ConstantBufferAL::Create( uint32_t size, Tr2RenderContextEnum::BufferUsage usage, const void* initialData, Tr2PrimaryRenderContextAL & renderContext )
{
	Destroy();

	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	if( !size || size > MAX_CB_SIZE )
	{
		return E_INVALIDARG;
	}
	if( ( usage & Tr2RenderContextEnum::USAGE_IMMUTABLE ) != 0 && !initialData )
	{
		return E_INVALIDARG;
	}

	FORWARD_HR( m_buffer.Create( TrinityALImpl::Tr2ResourceHelper::DYNAMIC, size, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, 0, nullptr, renderContext ) );

	m_size = size;
	m_owner = &renderContext;

	return S_OK;
}

void Tr2ConstantBufferAL::Destroy()
{
	if( m_owner )
	{
		m_buffer.Destroy( *m_owner );
	}
	m_size = 0;
	m_owner = nullptr;
}

ALResult Tr2ConstantBufferAL::Lock( void** data, Tr2RenderContextAL& )
{
	if( !m_owner )
	{
		return E_INVALIDCALL;
	}

	return m_buffer.MapForWriting( *data, Tr2LockType::SYNCHRONIZED, *m_owner );
}

ALResult Tr2ConstantBufferAL::Unlock( Tr2RenderContextAL& )
{
	m_buffer.UnmapForWriting( *m_owner );
	return S_OK;
}

bool Tr2ConstantBufferAL::IsValid() const
{
	return m_owner != nullptr;
}

uint32_t Tr2ConstantBufferAL::GetSize() const 
{ 
	return m_size; 
}

void* Tr2ConstantBufferAL::GetBufferMirror( uint32_t minimumSize, Tr2RenderContextAL& )
{
	if( !IsValid() )
	{
		return nullptr;
	}
	if( !minimumSize )
	{
		minimumSize = m_size;
	}
	if( m_mirror.size() < minimumSize )
	{
		m_mirror.resize( "Tr2ConstantBufferAL::m_mirror", minimumSize );
	}
	return m_mirror.get();
}

void* Tr2ConstantBufferAL::GetBufferMirror( Tr2RenderContextAL& renderContext )
{
	return GetBufferMirror( 0, renderContext );
}

ALResult Tr2ConstantBufferAL::UpdateFromMirror( Tr2RenderContextAL& renderContext )
{
	if( !IsValid() || m_mirror.empty() )
	{
		return E_INVALIDCALL;
	}
	if( m_mirror.size() > m_size )
	{
		auto owner = m_owner;
		Destroy();
		CR_RETURN_HR( Create( m_mirror.size(), *owner ) );
	}
	void* data;
	CR_RETURN_HR( Lock( &data, renderContext ) );
	memcpy( data, m_mirror.get(), m_size );
	CR_RETURN_HR( Unlock( renderContext ) );
	return S_OK;
}

bool Tr2ConstantBufferAL::operator==( const Tr2ConstantBufferAL& other ) const
{
	return this == &other;
}

Tr2ALMemoryType Tr2ConstantBufferAL::GetMemoryClass() const 
{ 
	return AL_MEMORY_MANAGED; 
}

#endif