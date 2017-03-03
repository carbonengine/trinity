#include "StdAfx.h"
#include "Tr2ConstantBufferALStub.h"

#if( TRINITY_PLATFORM==TRINITY_STUB )

#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2ConstantBufferAL::Tr2ConstantBufferAL()
	:m_usage( 0 )
{
}

Tr2ConstantBufferAL::~Tr2ConstantBufferAL()
{
}

ALResult Tr2ConstantBufferAL::Create( uint32_t size, Tr2RenderContextEnum::BufferUsage usage, const void* initialData, Tr2RenderContextAL &renderContext )
{
	if( !renderContext.IsValid() )
	{
		return E_INVALIDARG;
	}

	if( size == 0 )
	{
		return E_INVALIDARG;
	}

	if( ( usage & USAGE_IMMUTABLE ) && !initialData )
	{
		CCP_AL_LOGERR( "Create: Trying to create an immutable buffer without providing data" );
		return E_INVALIDARG;
	}

	m_shadowCopy.resize( "Tr2ConstantBufferAL::m_shadowCopy", size );
	if( m_shadowCopy.empty() )
	{
		return E_OUTOFMEMORY;
	}

	m_usage = usage;
	ChangeObjectId();
	
	return S_OK;
}

ALResult Tr2ConstantBufferAL::Lock( void** data, Tr2RenderContextAL & /*renderContext*/ )
{
	if( m_usage & USAGE_IMMUTABLE )
	{
		return E_INVALIDCALL;
	}

	if( m_shadowCopy.empty() )
	{
		*data = nullptr;
		return E_FAIL;
	}

	*data = m_shadowCopy.get();
	return S_OK;
}

ALResult Tr2ConstantBufferAL::Unlock( Tr2RenderContextAL & /*renderContext*/ )
{
	return S_OK;
}

bool Tr2ConstantBufferAL::IsValid() const
{
	return !m_shadowCopy.empty();
}

void Tr2ConstantBufferAL::Destroy()
{
	m_shadowCopy.clear();
}

void* Tr2ConstantBufferAL::GetBufferMirror( uint32_t minimumSize, Tr2RenderContextAL& renderContext )
{
	if( minimumSize > GetSize() )
	{
		if( FAILED( Create( minimumSize, renderContext ) ) )
		{
			return nullptr;
		}
	}

	if( m_shadowCopy.empty() )
	{
		return nullptr;
	}

	return m_shadowCopy.get();
}

ALResult Tr2ConstantBufferAL::UpdateFromMirror( Tr2RenderContextAL & /*renderContext*/ )
{
	return S_OK;
}

#endif