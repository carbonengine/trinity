#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2IndexBufferALOrbis.h"
#include "Tr2MemoryAllocator.h"
#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2IndexBufferAL::Tr2IndexBufferAL()
	:m_usage( 0 ),
	m_numIndices( 0 ),
	m_is16Bit( false )
{
}

Tr2IndexBufferAL::~Tr2IndexBufferAL()
{
	Destroy();
}

Tr2IndexBufferAL& Tr2IndexBufferAL::operator=( Tr2IndexBufferAL&& other )
{
	if( this == &other )
	{
		return *this;
	}
	m_numIndices = other.m_numIndices;
	m_is16Bit = other.m_is16Bit;
	m_usage = other.m_usage;
	m_buffer = std::move( other.m_buffer );

	other.m_numIndices = 0;
	other.m_usage = 0;
	ChangeObjectId();

	return *this;
}

void Tr2IndexBufferAL::Destroy()
{
	m_buffer.Destroy();
	m_numIndices = 0;
	m_usage = 0;
}

ALResult Tr2IndexBufferAL::Create(	
	uint32_t numberOfIndices, 
	Tr2RenderContextEnum::BufferUsage usage, 
	Tr2RenderContextEnum::IndexBufferBitcount bitCount, 
	const void* initialData, 
	Tr2PrimaryRenderContextAL &renderContext )
{
	Destroy();

	AL_FUZZ( OT_INDEX_BUFFER );
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !ValidateUsage( usage ) )
	{
		CCP_AL_LOGERR( "Tr2IndexBufferAL::Create: invalid combination of USAGE flags" );
		return E_INVALIDARG;
	}

	if( ( usage & USAGE_IMMUTABLE ) && !initialData )
	{
		CCP_AL_LOGERR( "Tr2IndexBufferAL::Create: trying to create an immutable buffer without providing data" );
		return E_INVALIDARG;
	}

	if( !renderContext.IsValid() )
	{
		return E_FAIL;
	}

	size_t lengthInBytes = numberOfIndices * ( bitCount == IB_16BIT ? 2 : 4 );
	Tr2BufferImplAL::BufferType bufferType = ( usage & USAGE_LOCK_FREQUENTLY ) ? Tr2BufferImplAL::DYNAMIC : Tr2BufferImplAL::STATIC;
	void* initialDataPtr = nullptr;
	CR_RETURN_HR( m_buffer.Create( 
		lengthInBytes, 
		sce::Gnm::kAlignmentOfBufferInBytes,
		Tr2BufferImplAL::READ_WRITE, 
		Tr2BufferImplAL::READ_ONLY,
		bufferType, 
		bufferType == Tr2BufferImplAL::DYNAMIC ? Tr2MemoryAllocator::ONION : Tr2MemoryAllocator::GARLIC,
		&initialDataPtr, 
		renderContext ) );
	if( initialData )
	{
		memcpy( initialDataPtr, initialData, lengthInBytes );
	}
	m_is16Bit = bitCount == IB_16BIT;
	m_numIndices = numberOfIndices;
	m_usage = usage;
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
	AL_FUZZ_LOCK( OT_INDEX_BUFFER );

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
	AL_FUZZ_LOCK( OT_INDEX_BUFFER );

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
	Tr2RenderContextAL & renderContext )
{
	AL_FUZZ_LOCK( OT_INDEX_BUFFER );
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !IsValid() || !renderContext.IsValid() )
	{
		*data = 0;
		return E_FAIL;
	}

	if( lockType == LOCK_READONLY )
	{
		if( ( m_usage & USAGE_CPU_READ ) == 0 )
		{
			return E_INVALIDARG;
		}
	}
	else
	{
		if( ( m_usage & USAGE_CPU_WRITE ) == 0 )
		{
			return E_INVALIDARG;
		}
	}

	switch( lockType )
	{
	case LOCK_READONLY:
		*data = reinterpret_cast<uint8_t*>( m_buffer.GetMemoryForCpuReading( renderContext ) ) + offset;
		break;
	case LOCK_WRITEONLY:
		*data = reinterpret_cast<uint8_t*>( m_buffer.GetMemoryForCpuWriting( renderContext ) ) + offset;
		break;
	case LOCK_NO_OVERWRITE:
		*data = reinterpret_cast<uint8_t*>( m_buffer.GetMemoryForCpuWritingNoSync( renderContext ) ) + offset;
		break;
	default:
		return E_INVALIDARG;
	}
	return S_OK;
}

ALResult Tr2IndexBufferAL::Unlock( Tr2RenderContextAL & /*renderContext*/ )
{
	AL_FUZZ_LOCK( OT_INDEX_BUFFER );
	return S_OK;
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

	memcpy( reinterpret_cast<uint8_t*>( m_buffer.GetMemoryForCpuWriting( renderContext ) ) + offset, data, size );
}

bool Tr2IndexBufferAL::operator==( const Tr2IndexBufferAL& other ) const
{
	return this == &other;
}

#endif