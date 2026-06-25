// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2LockGuard.h"

Tr2LockGuard::Tr2LockGuard() :
	m_memory( nullptr ),
	m_originalMemory( nullptr ),
	m_size( 0 )
{
}

Tr2LockGuard::~Tr2LockGuard()
{
	CCPFreeWithGuard( m_memory );
}

void Tr2LockGuard::Lock( size_t size, void* originalMemory )
{
	CCP_ASSERT( size );
	if( !size )
	{
		CCP_LOGWARN( "TrinityAL: guarded lock buffer is locked with size 0" );
	}
	else
	{
		CCP_ASSERT( originalMemory );
		if( !originalMemory )
		{
			CCP_LOGWARN( "TrinityAL: guarded lock buffer is locked with original memory NULL" );
		}
	}
	CCP_ASSERT( !m_memory );
	if( m_memory )
	{
		CCP_LOGWARN( "TrinityAL: guarded lock buffer is not NULL when locking" );
	}
	CCPFreeWithGuard( m_memory );
	m_memory = CCPMallocWithGuard( size );
	if( !m_memory )
	{
		CCP_LOGERR( " TrinityAL: guarded lock out of memory (size: %zu)", size );
	}
	else
	{
		memset( m_memory, 0xde, size );
	}
	m_originalMemory = originalMemory;
	m_size = size;
}

void Tr2LockGuard::Unlock()
{
	CCP_ASSERT( m_memory );
	if( !m_memory )
	{
		CCP_LOGWARN( "TrinityAL: guarded lock buffer memory is NULL when unlocking" );
		return;
	}
	CCP_ASSERT( m_originalMemory );
	if( !m_originalMemory )
	{
		CCP_LOGWARN( "TrinityAL: guarded lock original buffer memory is NULL when unlocking" );
		return;
	}
	memcpy( m_originalMemory, m_memory, m_size );
	CCPFreeWithGuard( m_memory );
	m_memory = nullptr;
	m_originalMemory = nullptr;
}

void* Tr2LockGuard::GetMemory()
{
	return m_memory;
}
