#include "StdAfx.h"

#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )

#include "Tr2ShaderProgramALDx11.h"
#include "Tr2ShaderALDx11.h"
#include "Tr2PrimaryRenderContextDx11.h"

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

		if( count == 0 )
		{
			return E_INVALIDARG;
		}

		for( size_t i = 0; i < count; ++i )
		{
			if( !shaders[i].IsValid() )
			{
				return E_INVALIDARG;
			}
			switch( shaders[i].GetType() )
			{
			case VERTEX_SHADER:
				if( m_shaders.vertexShader )
				{
					return E_INVALIDARG;
				}
				m_shaders.vertexShader = shaders[i].m_shader->m_shader.vertexShader;
				m_vertexShader = shaders[i];
				break;
			case PIXEL_SHADER:
				if( m_shaders.pixelShader )
				{
					return E_INVALIDARG;
				}
				m_shaders.pixelShader = shaders[i].m_shader->m_shader.pixelShader;
				break;
			case COMPUTE_SHADER:
				if( m_shaders.computeShader )
				{
					return E_INVALIDARG;
				}
				m_shaders.computeShader = shaders[i].m_shader->m_shader.computeShader;
				break;
			case GEOMETRY_SHADER:
				if( m_shaders.geometryShader )
				{
					return E_INVALIDARG;
				}
				m_shaders.geometryShader = shaders[i].m_shader->m_shader.geometryShader;
				break;
			case HULL_SHADER:
				if( m_shaders.hullShader )
				{
					return E_INVALIDARG;
				}
				m_shaders.hullShader = shaders[i].m_shader->m_shader.hullShader;
				break;
			case DOMAIN_SHADER:
				if( m_shaders.domainShader )
				{
					return E_INVALIDARG;
				}
				m_shaders.domainShader = shaders[i].m_shader->m_shader.domainShader;
				break;
			default:
				return E_INVALIDARG;
			}
		}
		m_registerMap = Tr2RegisterMapAL( shaders, count );

		m_isValid = true;
		return S_OK;
	}

	void Tr2ShaderProgramAL::Destroy()
	{
		m_shaders.vertexShader = nullptr;
		m_shaders.pixelShader = nullptr;
		m_shaders.computeShader = nullptr;
		m_shaders.geometryShader = nullptr;
		m_shaders.hullShader = nullptr;
		m_shaders.domainShader = nullptr;
		m_isValid = false;
		m_vertexShader = ::Tr2ShaderAL();
		m_registerMap = Tr2RegisterMapAL();
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
		description["name"] = m_name;
	}

	const Tr2RegisterMapAL& Tr2ShaderProgramAL::GetRegisterMap() const
	{
		return m_registerMap;
	}

	ALResult Tr2ShaderProgramAL::SetName( const char* name )
	{
		m_name = name;
		return S_OK;
	}
}
#endif