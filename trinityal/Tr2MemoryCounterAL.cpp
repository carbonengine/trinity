// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2MemoryCounterAL.h"

CCP_STATS_DECLARE( gpuMemoryTotalEst, "Trinity/AL/gpuMemoryEst/total", false, CST_MEMORY, "Total estimated GPU memory" );
CCP_STATS_DECLARE( gpuMemoryBufferEst, "Trinity/AL/gpuMemoryEst/buffers", false, CST_MEMORY, "Total estimated buffer GPU memory" );
CCP_STATS_DECLARE( gpuMemoryTextureEst, "Trinity/AL/gpuMemoryEst/textures", false, CST_MEMORY, "Total estimated texture GPU memory" );
CCP_STATS_DECLARE( gpuMemoryOtherEst, "Trinity/AL/gpuMemoryEst/other", false, CST_MEMORY, "Total estimated GPU memory (neither buffer nor textures)" );


Tr2MemoryCounterAL::Tr2MemoryCounterAL() :
	m_memoryType( SYSTEM_MEMORY ),
	m_size( 0 )
{
}

Tr2MemoryCounterAL::Tr2MemoryCounterAL( Tr2MemoryCounterAL&& other ) :
	m_memoryType( other.m_memoryType ),
	m_size( other.m_size )
{
	other.m_size = 0;
}

Tr2MemoryCounterAL::~Tr2MemoryCounterAL()
{
	Reset();
}

Tr2MemoryCounterAL& Tr2MemoryCounterAL::operator=( Tr2MemoryCounterAL&& other )
{
	if( this == &other )
	{
		return *this;
	}
	Reset();

	m_memoryType = other.m_memoryType;
	m_size = other.m_size;
	other.m_size = 0;
	return *this;
}

void Tr2MemoryCounterAL::Set( MemoryType memoryType, size_t size )
{
	Reset();

	m_memoryType = memoryType;
	m_size = size;
	if( !m_size )
	{
		return;
	}
	UpdateCounters( int32_t( m_size ) );
}

void Tr2MemoryCounterAL::Grow( size_t size )
{
	m_size += size;
	UpdateCounters( int32_t( size ) );
}

void Tr2MemoryCounterAL::Shrink( size_t size )
{
	CCP_ASSERT( size <= m_size );

	m_size -= size;
	UpdateCounters( -int32_t( size ) );
}

void Tr2MemoryCounterAL::Set( MemoryType memoryType, const Tr2BitmapDimensions& bitmap, const Tr2MsaaDesc& msaa )
{
	Reset();

	m_memoryType = memoryType;
	m_size = 0;
	uint32_t mult = bitmap.GetArraySize() * std::max( msaa.samples, 1u );
	for( uint32_t i = 0; i < bitmap.GetTrueMipCount(); ++i )
	{
		m_size += bitmap.GetMipSize( i ) * mult;
	}
	UpdateCounters( int32_t( m_size ) );
}

void Tr2MemoryCounterAL::Reset()
{
	if( !m_size )
	{
		return;
	}
	UpdateCounters( -int32_t( m_size ) );
	m_size = 0;
}

void Tr2MemoryCounterAL::UpdateCounters( int32_t inc ) const
{
	switch( m_memoryType )
	{
	case SYSTEM_MEMORY:
		return;
	case BUFFER:
		CCP_STATS_ADD( gpuMemoryBufferEst, inc );
		break;
	case TEXTURE:
		CCP_STATS_ADD( gpuMemoryTextureEst, inc );
		break;
	case OTHER:
		CCP_STATS_ADD( gpuMemoryOtherEst, inc );
		break;
	default:
		break;
	}
	CCP_STATS_ADD( gpuMemoryTotalEst, inc );
}
