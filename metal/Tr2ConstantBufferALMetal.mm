#include "StdAfx.h"

#if( TRINITY_PLATFORM == TRINITY_METAL )

#include "Tr2ConstantBufferALMetal.h"
#include "ALLog.h"
#include "Tr2RenderContextMetal.h"

bool g_fullSizeConstantBuffers = false;

namespace TrinityALImpl
{
	Tr2ConstantBufferAL::Tr2ConstantBufferAL()
		: m_metalContext( nullptr ),
		m_buffer( nullptr ),
		m_lockTag( 0 ),
		m_size( 0 )
	{
	}
	
	Tr2ConstantBufferAL::~Tr2ConstantBufferAL()
	{
		Destroy();
	}

	ALResult Tr2ConstantBufferAL::Create( uint32_t size, Tr2ConstantUsageAL::Type usage, const void* initialData, Tr2RenderContextAL &renderContext )
	{
		if( !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}

		if( size == 0 )
		{
			return E_INVALIDARG;
		}

		if( ( usage == Tr2ConstantUsageAL::IMMUTABLE ) && !initialData )
		{
			CCP_AL_LOGERR( "Create: Trying to create an immutable buffer without providing data" );
			return E_INVALIDARG;
		}
		
		m_buffer = CCPAlignedMalloc( g_fullSizeConstantBuffers ? TRINITY_PLATFORM_MAX_CONSTANT_BUFFER_SIZE : size, 32 );
		if( !m_buffer )
		{
			return E_OUTOFMEMORY;
		}
		
		if( initialData )
		{
			memcpy( m_buffer, initialData, size );
		}

		m_metalContext = renderContext.GetMetalContext();
		m_size = size;

		return S_OK;
	}

	ALResult Tr2ConstantBufferAL::Lock( void** data, Tr2RenderContextAL & /*renderContext*/ )
	{
		if( !IsValid() )
		{
			return E_INVALIDCALL;
		}

		*data = m_buffer;
		++m_lockTag;
		return S_OK;
	}

	ALResult Tr2ConstantBufferAL::Unlock( Tr2RenderContextAL & /*renderContext*/ )
	{
		if( !IsValid() )
		{
			return E_INVALIDCALL;
		}
		return S_OK;
	}

	ALResult Tr2ConstantBufferAL::SetConstants( Tr2RenderContextEnum::ShaderType shaderType, uint32_t constantIndex, Tr2RenderContextAL & renderContext )
	{
		if( m_buffer )
		{
			renderContext.GetMetalWorkQueue()->SetConstants( shaderType, m_buffer, g_fullSizeConstantBuffers ? TRINITY_PLATFORM_MAX_CONSTANT_BUFFER_SIZE : m_size, m_lockTag, METAL_CONST_BUFFER_OFFSET + constantIndex );
		}

		return S_OK;
	}

	bool Tr2ConstantBufferAL::IsValid() const
	{
		return m_buffer != nullptr;
	}

	void Tr2ConstantBufferAL::Destroy()
	{
		if( m_metalContext )
		{
			m_metalContext->DestroyConstantBuffer( m_buffer );
		}
		m_metalContext = nullptr;
		m_buffer = nullptr;
		m_size = 0;
	}

	uint32_t Tr2ConstantBufferAL::GetSize() const
	{
		return m_size;
	}

	void Tr2ConstantBufferAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2ConstantBufferAL";
		description["size"] = std::to_string( m_size );
	}
}
#endif
