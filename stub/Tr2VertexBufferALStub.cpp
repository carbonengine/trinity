#include "StdAfx.h"
#include "Tr2VertexBufferALStub.h"


#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2VertexBufferAL::Tr2VertexBufferAL() 
	:m_usage( 0 )
{
}

Tr2VertexBufferAL::~Tr2VertexBufferAL()
{
}

Tr2VertexBufferAL& Tr2VertexBufferAL::operator=( Tr2VertexBufferAL&& other )
{
	if( this != &other )
	{
		m_usage = other.m_usage;
		m_buffer = std::move(other.m_buffer);
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
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	if( ( usage == USAGE_IMMUTABLE && !initialData ) || !lengthInBytes )
	{
		return E_INVALIDARG;
	}
	m_buffer.resize("Tr2VertexBufferALStub::m_buffer", lengthInBytes);
	m_usage = usage;
	if ( initialData )
	{
		memcpy(m_buffer.get(), initialData, lengthInBytes);
	}
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
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( lockType == LOCK_NO_OVERWRITE && !(m_usage & USAGE_LOCK_FREQUENTLY) )
	{
		return E_INVALIDARG;
	}
	if( offset + size > m_buffer.size() )
	{
		return E_INVALIDCALL;
	}
	*data = m_buffer.get() + offset;
	return S_OK;
}

ALResult Tr2VertexBufferAL::Unlock( Tr2RenderContextAL& /*renderContext*/ )
{
	return S_OK;
}

ALResult Tr2VertexBufferAL::UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext )
{
	if( !IsValid() || 
		 m_usage & USAGE_IMMUTABLE || 
		 (m_usage & USAGE_CPU_WRITE) == 0 || 
		 m_usage & USAGE_LOCK_FREQUENTLY ||
		 offset + size > m_buffer.size())
	{
		return E_INVALIDCALL;
	}

	memcpy( m_buffer.get() + offset, data, size );
	return S_OK;
}

bool Tr2VertexBufferAL::IsValid() const
{
	return !m_buffer.empty();
}

void Tr2VertexBufferAL::Destroy()
{
	m_buffer.clear();
}

#endif
