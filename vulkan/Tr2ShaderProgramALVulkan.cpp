#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "Tr2ShaderProgramALVulkan.h"
#include "Tr2PrimaryRenderContextVulkan.h"
#include "Tr2ShaderALVulkan.h"
#include "UtilitiesVulkan.h"


using namespace Tr2RenderContextEnum;


namespace
{
	VkDescriptorType GetDescriptorType( Tr2ShaderRegisterAL::RegisterType registerType )
	{
		switch( registerType )
		{
		case Tr2ShaderRegisterAL::CONSTANTS:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case Tr2ShaderRegisterAL::SAMPLER:
			return VK_DESCRIPTOR_TYPE_SAMPLER;
		case Tr2ShaderRegisterAL::UAV:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		default:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		}
	}
}

namespace TrinityALImpl
{
	Tr2ShaderProgramAL::Tr2ShaderProgramAL()
		:m_owner( nullptr ),
		m_resourceLayout( VK_NULL_HANDLE ),
		m_constantLayout( VK_NULL_HANDLE ),
		m_pipelineLayout( VK_NULL_HANDLE )
	{
	}

	Tr2ShaderProgramAL::~Tr2ShaderProgramAL()
	{
		Destroy();
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

		uint32_t poolSizes[4] = { 0 };

		uint32_t registerOffsets[] = { 0, 2, 1, 0 };

		std::vector<VkDescriptorSetLayoutBinding> resourceSetBindings, constantBindings;

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

			uint32_t registerSize = 32;

			auto& inputs = shaders[i].m_shader->m_signature.registers;
			for( auto it = begin( inputs ); it != end( inputs ); ++it )
			{
				VkDescriptorSetLayoutBinding binding = {
					it->registerIndex + registerOffsets[it->registerType] * 6 * registerSize + shaders[i].GetType() * registerSize,
					GetDescriptorType( it->registerType ),
					1,
					info.stage,
					nullptr
				};

				if( it->registerType == Tr2ShaderRegisterAL::CONSTANTS )
				{
					constantBindings.push_back( binding );
				}
				else
				{
					++poolSizes[it->registerType];
					resourceSetBindings.push_back( binding );
				}

				RegisterInput ri = { binding.binding, shaders[i].GetType(), it->registerIndex, it->registerType };
				m_registerInput.push_back( ri );
			}
		}

		if( !resourceSetBindings.empty() )
		{
			VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0 };
			layoutInfo.bindingCount = uint32_t( resourceSetBindings.size() );
			layoutInfo.pBindings = resourceSetBindings.data();

			VkDescriptorSetLayout layout;
			CR_RETURN_HR( Vk2Al( vkCreateDescriptorSetLayout( renderContext.m_device, &layoutInfo, nullptr, &layout ) ) );

			m_resourceLayout = layout;

			for( uint32_t i = 0; i < _countof( poolSizes ); ++i )
			{
				if( !poolSizes[i] )
				{
					continue;
				}
				VkDescriptorPoolSize poolSize = { GetDescriptorType( Tr2ShaderRegisterAL::RegisterType( i ) ), poolSizes[i] };
				m_poolSizes.push_back( poolSize );
			}
		}

		if( !constantBindings.empty() )
		{
			VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0 };
			layoutInfo.bindingCount = uint32_t( constantBindings.size() );
			layoutInfo.pBindings = constantBindings.data();

			VkDescriptorSetLayout layout;
			CR_RETURN_HR( Vk2Al( vkCreateDescriptorSetLayout( renderContext.m_device, &layoutInfo, nullptr, &layout ) ) );

			m_constantLayout = layout;

			for( uint32_t i = 0; i < _countof( poolSizes ); ++i )
			{
				if( !poolSizes[i] )
				{
					continue;
				}
				VkDescriptorPoolSize poolSize = { GetDescriptorType( Tr2ShaderRegisterAL::RegisterType( i ) ), poolSizes[i] };
				m_poolSizes.push_back( poolSize );
			}
		}

		uint32_t size = 0;
		VkDescriptorSetLayout layouts[2];
		if( m_resourceLayout || m_constantLayout )
		{
			if( m_constantLayout )
			{
				layouts[size++] = m_constantLayout;
			}
			else
			{
				VkDescriptorSetLayoutCreateInfo emptyLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, 0, nullptr };
				VkDescriptorSetLayout emptyLayout = VK_NULL_HANDLE;
				CR_RETURN_HR( Vk2Al( vkCreateDescriptorSetLayout( renderContext.m_device, &emptyLayoutInfo, nullptr, &emptyLayout ) ) );

				layouts[size++] = emptyLayout;
			}
			if( m_resourceLayout )
			{
				layouts[size++] = m_resourceLayout;
			}
		}
		VkPipelineLayoutCreateInfo layoutInfo = {
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			size,
			layouts,
			0,
			nullptr
		};

		CR_RETURN_HR( Vk2Al( vkCreatePipelineLayout( renderContext.m_device, &layoutInfo, nullptr, &m_pipelineLayout ) ) );
		m_owner = &renderContext;

		m_registerMap = Tr2RegisterMapAL( shaders, count );

		return S_OK;
	}


	void Tr2ShaderProgramAL::Destroy()
	{
		if( m_owner )
		{
			m_owner->DestroyLaterVulkan( m_resourceLayout, vkDestroyDescriptorSetLayout );
			m_owner->DestroyLaterVulkan( m_constantLayout, vkDestroyDescriptorSetLayout );
			m_owner->DestroyLaterVulkan( m_pipelineLayout, vkDestroyPipelineLayout );
			m_resourceLayout = VK_NULL_HANDLE;
			m_constantLayout = VK_NULL_HANDLE;
			m_pipelineLayout = VK_NULL_HANDLE;
			m_owner = nullptr;
		}
		m_shaders.clear();
		m_shaderInfo.clear();
		m_shaderInputs.clear();

		m_poolSizes.clear();
		m_registerInput.clear();
		m_registerMap = Tr2RegisterMapAL();
	}

	bool Tr2ShaderProgramAL::IsValid() const
	{
		return !m_shaderInfo.empty();
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