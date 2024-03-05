#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "Tr2ConstantBufferALDx12.h"
#include "Tr2PrimaryRenderContextDx12.h"
#include "Utilities.h"


namespace
{
	const uint32_t MAX_CB_SIZE = 64 * 1024;
}

namespace TrinityALImpl
{

	Tr2ConstantBufferAL::Tr2ConstantBufferAL()
		:m_size( 0 ),
		m_owner( nullptr )
	{
	}

	Tr2ConstantBufferAL::~Tr2ConstantBufferAL()
	{
		Destroy();
	}

	ALResult Tr2ConstantBufferAL::Create( uint32_t size, Tr2ConstantUsageAL::Type usage, const void* initialData, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}

		if( !size || size > MAX_CB_SIZE )
		{
			return E_INVALIDARG;
		}
		if( ( usage == Tr2ConstantUsageAL::IMMUTABLE ) != 0 && !initialData )
		{
			return E_INVALIDARG;
		}

		m_size = size;
		m_data.resize( "Tr2ConstantBufferAL::m_data", m_size );
		m_owner = &renderContext;

		if( initialData )
		{
			memcpy( m_data.get(), initialData, m_size );
		}

		return S_OK;
	}

	void Tr2ConstantBufferAL::Destroy()
	{
		m_size = 0;
		m_data.clear();
		m_owner = nullptr;
	}

	ALResult Tr2ConstantBufferAL::Lock( void** data, Tr2RenderContextAL& )
	{
		if( !IsValid() )
		{
			return E_INVALIDCALL;
		}
		// JB: Lock now just returns a handle to m_data
		*data = (void*)m_data.get();
		return S_OK;
	}

	ALResult Tr2ConstantBufferAL::Unlock( Tr2RenderContextAL& )
	{
		// JB: Forcibly dirty the render token causing a re-upload
		m_token.m_frameNumber--;
		return S_OK;
	}

	bool Tr2ConstantBufferAL::IsValid() const
	{
		return !m_data.empty();
	}

	uint32_t Tr2ConstantBufferAL::GetSize() const
	{
		return m_size;
	}

	Tr2ALMemoryType Tr2ConstantBufferAL::GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	void Tr2ConstantBufferAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2ConstantBufferAL";
		description["size"] = std::to_string( long long( m_size ) );
		description["name"] = m_name;
	}

	ALResult Tr2ConstantBufferAL::SetName( const char* name )
	{
		m_name = name;
		return S_OK;
	}
}
#endif