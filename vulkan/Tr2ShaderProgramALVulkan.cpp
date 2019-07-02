#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "Tr2ShaderProgramALVulkan.h"
#include "Tr2PrimaryRenderContextVulkan.h"


using namespace Tr2RenderContextEnum;


ALResult Tr2ShaderProgramAL::Create( Tr2ShaderAL* shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext )
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

	uint32_t bitmask = 0;

	for( size_t i = 0; i < count; ++i )
	{
		if( !shaders[i].IsValid() )
		{
			return E_INVALIDARG;
		}
		auto mask = 1 << shaders[i].GetType();
		if( ( mask & bitmask ) != 0 )
		{
			return E_INVALIDARG;
		}
		bitmask |= mask;
	}
	auto csBit = 1 << COMPUTE_SHADER;
	if( ( bitmask & csBit ) != 0 && ( bitmask & ~csBit ) != 0 )
	{
		return E_INVALIDARG;
	}

	m_shaderInfo.reserve( count );
	m_shaders.reserve( count );

	for( size_t i = 0; i < count; ++i )
	{
		VkPipelineShaderStageCreateInfo info = {
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			nullptr,
		};
		switch( shaders[i].GetType() )
		{
		case VERTEX_SHADER:
			info.stage = VK_SHADER_STAGE_VERTEX_BIT;
			m_shaderInputs = shaders[i].m_shader->m_signature.pipelineInputs;
			break;
		case PIXEL_SHADER:
			info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			break;
		case COMPUTE_SHADER:
			info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			break;
		case GEOMETRY_SHADER:
			info.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			break;
		case HULL_SHADER:
			info.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			break;
		case DOMAIN_SHADER:
			info.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			break;
		}
		info.module = shaders[i].m_shader->m_shader;
		info.pName = "main";
		m_shaderInfo.push_back( info );
		m_shaders.push_back( shaders[i] );
	}

	return S_OK;
}


void Tr2ShaderProgramAL::Destroy()
{
	m_shaders.clear();
	m_shaderInfo.clear();
	m_shaderInputs.clear();
}

bool Tr2ShaderProgramAL::IsValid() const
{
	return !m_shaderInfo.empty();
}

Tr2ALMemoryType Tr2ShaderProgramAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}


#endif