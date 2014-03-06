#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_ORBIS )

#include "Tr2VertexBufferALOrbis.h"
#include "Tr2MemoryAllocator.h"
#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2VertexBufferAL::Tr2VertexBufferAL()
	:m_lengthInBytes( 0 ),
	m_usage( 0 )
{
}

Tr2VertexBufferAL::~Tr2VertexBufferAL()
{
	Destroy();
}

ALResult Tr2VertexBufferAL::Create( 
	uint32_t lengthInBytes,
	Tr2RenderContextEnum::BufferUsage usage,
	const void* initialData,
	Tr2RenderContextAL& renderContext )
{
	Destroy();
	
	if ( !ValidateUsage( usage ) )
	{
		CCP_AL_LOGERR( "Tr2VertexBufferAL::Create: invalid combination of USAGE flags" );
		return E_INVALIDARG;
	}

	if( ( usage & USAGE_IMMUTABLE ) && !initialData )
	{
		CCP_AL_LOGERR( "Tr2VertexBufferAL::Create: trying to create an immutable buffer without providing data" );
		return E_INVALIDARG;
	}

	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}

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

	m_lengthInBytes = lengthInBytes;
	m_usage = usage;
	ChangeObjectId();
	return S_OK;
}

ALResult Tr2VertexBufferAL::Create( uint32_t length,
					Tr2RenderContextAL& renderContext )
{
	return Create( length, 0, nullptr, renderContext );
}

ALResult Tr2VertexBufferAL::CreateEx( uint32_t lengthInBytes,
					Tr2RenderContextEnum::BufferUsage usage,
					const void* initialData,
					uint32_t /*exFlags*/,
					Tr2PrimaryRenderContextAL& renderContext )
{
	return Create( lengthInBytes, usage, initialData, renderContext );
}

bool Tr2VertexBufferAL::IsValid() const
{
	return m_buffer.IsValid();
}

void Tr2VertexBufferAL::Destroy()
{
	m_buffer.Destroy();
	m_lengthInBytes = 0;
	m_usage = 0;
}

bool Tr2VertexBufferAL::operator==( const Tr2VertexBufferAL& other ) const
{
	return &other == this;
}

ALResult Tr2VertexBufferAL::Lock( uint32_t offset,
				uint32_t size,
				void** data,
				Tr2RenderContextEnum::LockType lockType,
				Tr2RenderContextAL& renderContext )
{
	if( !IsValid() || !renderContext.IsValid() )
	{
		return E_INVALIDCALL;
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

ALResult Tr2VertexBufferAL::Unlock( Tr2RenderContextAL& renderContext )
{
	return S_OK;
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

	memcpy( reinterpret_cast<uint8_t*>( m_buffer.GetMemoryForCpuWriting( renderContext ) ) + offset, data, size );
}

#endif