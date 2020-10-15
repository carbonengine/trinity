////////////////////////////////////////////////////////////
//
//    Created:   April 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "Tr2ShaderALVulkan.h"
#include "Tr2PrimaryRenderContextVulkan.h"
#include "UtilitiesVulkan.h"


namespace TrinityALImpl
{

	Tr2ShaderAL::Tr2ShaderAL()
		:m_shader( 0 ),
		m_owner( nullptr )
	{
	}

	Tr2ShaderAL::~Tr2ShaderAL()
	{
		Destroy();
	}

	ALResult Tr2ShaderAL::Create(
		Tr2RenderContextEnum::ShaderType type,
		const Tr2ShaderBytecodeAL& bytecode,
		const Tr2ShaderBytecodeAL& patchedBytecode,
		const Tr2ShaderSignatureAL& signature,
		Tr2PrimaryRenderContextAL &renderContext )
	{
		Destroy();

		if( !bytecode.size )
		{
			return E_INVALIDARG;
		}
		if( !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}

		VkShaderModuleCreateInfo shader_module_create_info = {
			VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			nullptr,
			0,
			bytecode.size,
			reinterpret_cast<const uint32_t*>( bytecode.bytecode )
		};

		VkShaderModule shader;
		CR_RETURN_HR( Vk2Al( vkCreateShaderModule( renderContext.m_device, &shader_module_create_info, nullptr, &shader ) ) );

		m_bytecode.resize( "Tr2ShaderAL::m_bytecode", bytecode.size );
		if( m_bytecode.empty() )
		{
			vkDestroyShaderModule( renderContext.m_device, shader, nullptr );
			return E_OUTOFMEMORY;
		}
		m_shader = shader;
		m_owner = &renderContext;
		m_signature = signature;
		m_type = type;
		memcpy( m_bytecode.get(), bytecode.bytecode, bytecode.size );

		return S_OK;
	}

	void Tr2ShaderAL::Destroy()
	{
		if( m_shader )
		{
			vkDestroyShaderModule( m_owner->m_device, m_shader, nullptr );
			m_shader = 0;
			m_owner = nullptr;
			m_bytecode.clear();
			m_type = Tr2RenderContextEnum::INVALID_SHADER;
			m_signature = Tr2ShaderSignatureAL();
			m_type = Tr2RenderContextEnum::INVALID_SHADER;
		}
	}

	bool Tr2ShaderAL::IsValid() const
	{
		return m_shader != 0;
	}

	bool Tr2ShaderAL::operator==( const Tr2ShaderAL& shader ) const
	{
		return this == &shader;
	}

	Tr2RenderContextEnum::ShaderType Tr2ShaderAL::GetType() const 
	{ 
		return m_type;
	}
	
	ALResult Tr2ShaderAL::GetBytecode( Tr2ShaderBytecodeAL& bytecode ) const
	{
		if( !IsValid() )
		{
			bytecode = Tr2ShaderBytecodeAL();
			return E_INVALIDCALL;
		}
		bytecode.bytecode = m_bytecode.get();
		bytecode.size = m_bytecode.size();
		return S_OK;
	}

	const Tr2ShaderSignatureAL& Tr2ShaderAL::GetSignature() const
	{
		return m_signature;
	}

	Tr2ALMemoryType Tr2ShaderAL::GetMemoryClass() const 
	{ 
		return AL_MEMORY_MANAGED; 
	}

	void Tr2ShaderAL::SetNullShaderType( Tr2RenderContextEnum::ShaderType type )
	{
		Destroy();
		m_type = type;
	}

	void Tr2ShaderAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2ShaderAL";
	}

}
#endif