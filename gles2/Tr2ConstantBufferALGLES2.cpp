#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "Tr2ConstantBufferALGLES2.h"
#include "ALLog.h"
#include "Tr2RenderContextGLES2.h"

namespace TrinityALImpl
{

	Tr2ConstantBufferAL::Tr2ConstantBufferAL()
	{
	}

	ALResult Tr2ConstantBufferAL::Create( uint32_t size, Tr2ConstantUsageAL::Type usage, const void* initialData, Tr2RenderContextAL&renderContext )
	{
		if( !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}

		if( ( usage == Tr2ConstantUsageAL::IMMUTABLE ) && !initialData )
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

		return S_OK;
	}

	ALResult Tr2ConstantBufferAL::Lock( void** data, Tr2RenderContextAL& /*renderContext*/ )
	{
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

	uint32_t Tr2ConstantBufferAL::GetSize() const
	{
		return static_cast<uint32_t>( m_shadowCopy.size() );
	}

	Tr2ALMemoryType Tr2ConstantBufferAL::GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	void Tr2ConstantBufferAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2ConstantBufferAL";
		description["size"] = std::to_string( (long long)( m_shadowCopy.size() ) );
	}
}

#endif