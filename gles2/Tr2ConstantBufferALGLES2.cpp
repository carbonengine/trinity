#include "StdAfx.h"
#include "Tr2ConstantBufferALGLES2.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "ALLog.h"

using namespace Tr2RenderContextEnum;

Tr2ConstantBufferAL::Tr2ConstantBufferAL()
	: m_debugUsingMirror( false ),
	m_usage( 0 )
{
}

Tr2ConstantBufferAL::~Tr2ConstantBufferAL()
{
}

ALResult Tr2ConstantBufferAL::Create( uint32_t size, Tr2RenderContextEnum::BufferUsage usage, const void* initialData, Tr2RenderContextAL&renderContext )
{
	if( !renderContext.IsValid() )
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
		return E_FAIL;
	}

	if( initialData )
	{
		memcpy( m_shadowCopy.get(), initialData, size );
	}
	m_usage = usage;
	ChangeObjectId();
	
	return S_OK;
}

ALResult Tr2ConstantBufferAL::Lock( void** data, Tr2RenderContextAL& /*renderContext*/ )
{
	CCP_ASSERT( !m_debugUsingMirror );
	if( m_shadowCopy.empty() )
	{
		*data = 0;
		return E_FAIL;
	}

	*data = m_shadowCopy.get();
	return S_OK;
}

ALResult Tr2ConstantBufferAL::Unlock( Tr2RenderContextAL & /*renderContext*/ )
{
	CCP_ASSERT( !m_debugUsingMirror );

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
void* Tr2ConstantBufferAL::GetBufferMirror( uint32_t minimumSize, Tr2RenderContextAL& renderContext )
{
	if( minimumSize > GetSize() )
	{
		if( FAILED( Create( minimumSize, renderContext ) ) )
		{
			return nullptr;
		}
	}

	m_debugUsingMirror = true;

	return m_shadowCopy.get();
}

ALResult Tr2ConstantBufferAL::UpdateFromMirror( Tr2RenderContextAL& /*renderContext*/ )
{
	CCP_ASSERT( m_debugUsingMirror );
	m_debugUsingMirror = false;

	return S_OK;
}

#endif