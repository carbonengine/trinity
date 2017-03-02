#include "StdAfx.h"
#include "Tr2IndexBufferALStub.h"


#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "ALLog.h"

using namespace Tr2RenderContextEnum;

extern bool g_useManagedDX9Buffers;

Tr2IndexBufferAL::Tr2IndexBufferAL()
	: m_numIndices( 0 ),
	m_usage( 0 ),
	m_bitCount(IB_16BIT)
{
}

Tr2IndexBufferAL::~Tr2IndexBufferAL()
{
}

Tr2IndexBufferAL& Tr2IndexBufferAL::operator=( Tr2IndexBufferAL&& other )
{
	if( this != &other )
	{
		m_usage			= other.m_usage;
		m_buffer.clear();
		m_buffer = std::move(other.m_buffer);

		m_numIndices	= other.m_numIndices;
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
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}
	if( (usage & USAGE_IMMUTABLE) && !initialData )
	{
		return E_INVALIDARG;
	}
	else if( !ValidateUsage(usage))
	{
		return E_INVALIDARG;
	}
	
	size_t lengthInBytes = (bitCount + 1) * 16;
	m_numIndices = numberOfIndices;
	m_bitCount = bitCount;
	m_usage = usage;
	m_buffer.resize("Tr2IndexBufferALStub::m_buffer", GetTotalSizeInBytes());
	if( m_buffer.empty() )
	{
		return E_FAIL;
	}
	if( initialData )
	{
		memcpy(m_buffer.get(), initialData, GetTotalSizeInBytes() );
	}

	return S_OK;
}

ALResult Tr2IndexBufferAL::Lock(	
	uint32_t offset, 
	uint32_t sizeInBytes, 
	uint16_t *&data, 
	LockType lockType, 
	Tr2RenderContextAL & renderContext )
{
	return Lock(offset, sizeInBytes, (void**)&data, lockType, renderContext);
}

ALResult Tr2IndexBufferAL::Lock(	
	uint32_t offset, 
	uint32_t sizeInBytes, 
	uint32_t *&data, 
	LockType lockType, 
	Tr2RenderContextAL & renderContext )
{
	return Lock(offset, sizeInBytes, (void**)&data, lockType, renderContext);
}

ALResult Tr2IndexBufferAL::Lock(	
	uint32_t offset, 
	uint32_t size, 
	void** data, 
	LockType lockType, 
	Tr2RenderContextAL & /*renderContext*/ )
{
	if( !IsValid() )
	{
		return E_FAIL;
	}
	if( lockType == LOCK_NO_OVERWRITE && !(m_usage & USAGE_LOCK_FREQUENTLY) )
	{
		return E_INVALIDARG;
	}
	if( (m_usage & USAGE_CPU_READ) == 0 && lockType == LOCK_READONLY )
	{
		return E_INVALIDARG;
	}
	if( (m_usage & USAGE_CPU_WRITE) == 0 && lockType == LOCK_WRITEONLY )
	{
		return E_INVALIDARG;
	}
	if( !offset && !size  )
	{
		size = GetTotalSizeInBytes();
	}
	if( offset + size > GetTotalSizeInBytes() )
	{
		return E_FAIL;
	}
	*data = m_buffer.get() + offset;
	return S_OK;
}

ALResult Tr2IndexBufferAL::Unlock( Tr2RenderContextAL & /*renderContext*/ )
{
	return S_OK;
}

ALResult Tr2IndexBufferAL::UpdateBuffer( uint32_t offset, uint32_t size, const void* data, Tr2RenderContextAL & renderContext )
{
	if( m_usage & USAGE_IMMUTABLE || 
		 (m_usage & USAGE_CPU_WRITE) == 0 ||
		 m_usage & USAGE_LOCK_FREQUENTLY)
	{
		return E_INVALIDCALL;
	}
	if( !IsValid() )
	{
		return E_INVALIDCALL;
	}
	if( offset + size > GetTotalSizeInBytes() )
	{
		return E_FAIL;
	}
	memcpy(m_buffer.get() + offset, data, size);
	
	return S_OK;
}

bool Tr2IndexBufferAL::IsValid() const
{
	return !m_buffer.empty();
}

void Tr2IndexBufferAL::Destroy()
{
	m_buffer.clear();
}

#endif
