#include "StdAfx.h"

#if TRINITY_PLATFORM==TRINITY_DIRECTX9 

#include "../include/Tr2ShaderAL.h"
#include "Tr2ShaderProgramALDx9.h"
#include "Tr2ShaderALDx9.h"
#include "Tr2RenderContextDx9.h"


using namespace Tr2RenderContextEnum;

namespace TrinityALImpl
{

	Tr2ShaderProgramAL::Tr2ShaderProgramAL()
		:m_isValid( false )
	{
	}

	ALResult Tr2ShaderProgramAL::Create( ::Tr2ShaderAL* shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( !renderContext.IsValid() )
		{
			return E_INVALIDCALL;
		}

		if( count != 2 )
		{
			return E_INVALIDARG;
		}

		uint32_t mask = 0;
		for( size_t i = 0; i < count; ++i )
		{
			if( !shaders[i].IsValid() )
			{
				return E_INVALIDARG;
			}
			uint32_t bit = 1 << shaders[i].GetType();
			if( ( mask & bit ) != 0 )
			{
				return E_INVALIDARG;
			}
			mask |= bit;
		}
		if( mask != ( ( 1 << VERTEX_SHADER ) | ( 1 << PIXEL_SHADER ) ) )
		{
			return E_INVALIDARG;
		}

		if( shaders[0].GetType() == VERTEX_SHADER )
		{
			m_vertexShader = shaders[0];
			m_pixelShader = shaders[1];
		}
		else
		{
			m_vertexShader = shaders[1];
			m_pixelShader = shaders[0];
		}

		m_registerMap = Tr2RegisterMapAL( shaders, count );
		for( uint32_t stage = VERTEX_SHADER; stage <= PIXEL_SHADER; ++stage )
		{
			for( uint32_t i = 0; i < Tr2RegisterMapAL::MAX_RESOURCES_IN_STAGE; ++i)
			{
				if( m_registerMap.samplers[stage][i] < m_registerMap.samplerCount )
				{
					if( m_registerMap.srvs[stage][i] >= m_registerMap.srvCount )
					{
						m_registerMap.srvs[stage][i] = uint8_t( m_registerMap.srvCount++ );
					}
				}
			}
		}

		m_isValid = true;
		return S_OK;
	}

	void Tr2ShaderProgramAL::Destroy()
	{
		m_vertexShader = ::Tr2ShaderAL();
		m_pixelShader = ::Tr2ShaderAL();
		m_registerMap = Tr2RegisterMapAL();
		m_isValid = false;
	}

	bool Tr2ShaderProgramAL::IsValid() const
	{
		return m_isValid;
	}

	Tr2ALMemoryType Tr2ShaderProgramAL::GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	void Tr2ShaderProgramAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2ShaderProgramAL";
	}

	const Tr2RegisterMapAL& Tr2ShaderProgramAL::GetRegisterMap() const
	{
		return m_registerMap;
	}
}
#endif
