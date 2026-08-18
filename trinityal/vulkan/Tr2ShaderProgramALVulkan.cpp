#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "Tr2ShaderProgramALVulkan.h"
#include "Tr2PrimaryRenderContextVulkan.h"
#include "Tr2ShaderALVulkan.h"
#include "Tr2ShaderBindingABIVulkan.h"
#include "UtilitiesVulkan.h"


using namespace Tr2RenderContextEnum;


namespace
{
	VkDescriptorType GetDescriptorType( Tr2ShaderRegisterAL::RegisterType registerType )
	{
		switch( registerType )
		{
		case Tr2ShaderRegisterAL::CONSTANT_BUFFER:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case Tr2ShaderRegisterAL::SAMPLER:
			return VK_DESCRIPTOR_TYPE_SAMPLER;
		default:
			// HLSL Buffer<T> compiles to a uniform texel buffer and RWBuffer<T> to a
			// storage texel buffer (verified from dxc SPIR-V: OpTypeImage ... Buffer,
			// Sampled=1 and Sampled=2), so the typed UAV register takes a texel
			// descriptor + VkBufferView, while UAV_STRUCTURED_BUFFER takes a
			// STORAGE_BUFFER + VkDescriptorBufferInfo. Textures take STORAGE_IMAGE.
			if( registerType & Tr2ShaderRegisterAL::UAV_REGISTER_FLAG )
			{
				if( registerType == Tr2ShaderRegisterAL::UAV_BUFFER )
				{
					return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
				}
				if( registerType == Tr2ShaderRegisterAL::UAV_STRUCTURED_BUFFER )
				{
					return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				}
				return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			}
			// The SRV mirror of the above: typed SRV_BUFFER is a uniform texel buffer,
			// SRV_STRUCTURED_BUFFER is a storage buffer, textures are sampled images.
			if( registerType & Tr2ShaderRegisterAL::SRV_REGISTER_FLAG )
			{
				if( registerType == Tr2ShaderRegisterAL::SRV_BUFFER )
				{
					return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
				}
				if( registerType == Tr2ShaderRegisterAL::SRV_STRUCTURED_BUFFER )
				{
					return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				}
				return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			}
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		}
	}

	// The binding-number block a register lives in -- RegisterClassOffset, and the
	// whole binding formula with it -- now lives in Tr2ShaderBindingABIVulkan.h, so
	// that trinityal/tests/ShaderBindingABI.cpp can derive its expected SPIR-V
	// decorations by calling the same code this file calls, instead of tabulating
	// sixteen expected numbers that would keep passing while the formula drifted.
	// That header carries the full rationale (four HLSL classes vs. six descriptor
	// kinds; why b and u can share block 0; what Tasks 4e and 4f got wrong).
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

		std::map<VkDescriptorType, uint32_t> poolCounts;

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

			auto& inputs = shaders[i].m_shader->m_signature.registers;
			for( auto it = begin( inputs ); it != end( inputs ); ++it )
			{
				VkDescriptorSetLayoutBinding binding = {
					Tr2VulkanBindingABI::BindingNumber( it->registerType, it->registerIndex, shaders[i].GetType() ),
					GetDescriptorType( it->registerType ),
					1,
					info.stage,
					nullptr
				};

				if( it->registerType == Tr2ShaderRegisterAL::CONSTANT_BUFFER )
				{
					constantBindings.push_back( binding );
				}
				else
				{
					++poolCounts[binding.descriptorType];
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
		}

		if( !constantBindings.empty() )
		{
			VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0 };
			layoutInfo.bindingCount = uint32_t( constantBindings.size() );
			layoutInfo.pBindings = constantBindings.data();

			VkDescriptorSetLayout layout;
			CR_RETURN_HR( Vk2Al( vkCreateDescriptorSetLayout( renderContext.m_device, &layoutInfo, nullptr, &layout ) ) );

			m_constantLayout = layout;
		}

		// The descriptor pool carries one size entry per distinct VkDescriptorType.
		// poolCounts was tallied from each register's real descriptor type, so typed
		// and structured buffers land in distinct entries rather than being collapsed
		// (the pre-Phase-2b code both collapsed SRV/UAV buffer types and then emitted
		// duplicate entries from the resource array twice). Constants are excluded
		// here -- their write path (descriptor set 0) is separate and comes later.
		for( auto it = begin( poolCounts ); it != end( poolCounts ); ++it )
		{
			VkDescriptorPoolSize poolSize = { it->first, it->second };
			m_poolSizes.push_back( poolSize );
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

	ALResult Tr2ShaderProgramAL::SetName( const char* )
	{
		return E_NOTIMPL;
	}
}

#endif