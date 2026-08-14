#include "StdAfx.h"
#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "Tr2ResourceSetALVulkan.h"
#include "Tr2PrimaryRenderContextVulkan.h"
#include "Tr2SamplerStateALVulkan.h"
#include "Tr2TextureALVulkan.h"
#include "Tr2ShaderProgramALVulkan.h"
#include "UtilitiesVulkan.h"


namespace TrinityALImpl
{
	Tr2ResourceSetAL::Tr2ResourceSetAL()
		:m_owner( nullptr ),
		m_pool( VK_NULL_HANDLE ),
		m_descriptorSet( VK_NULL_HANDLE )
	{
	}

	Tr2ResourceSetAL::~Tr2ResourceSetAL()
	{
		Destroy();
	}

	ALResult Tr2ResourceSetAL::Create( const Tr2ResourceSetDescriptionAL& description, const ::Tr2ShaderProgramAL& program, Tr2PrimaryRenderContextAL& renderContext )
	{
		Destroy();

		if( !renderContext.IsValid() || !program.IsValid() )
		{
			return E_INVALIDARG;
		}

		if( program.GetRegisterMap() != description.m_registerMap )
		{
			return E_INVALIDARG;
		}

		if( program.m_program->m_resourceLayout )
		{
			VkDescriptorPoolCreateInfo poolDesc = { 
				VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, 
				nullptr, 
				0, 
				1,
				program.m_program->m_poolSizes.size(),
				program.m_program->m_poolSizes.data()
			};

			CR_RETURN_HR( Vk2Al( vkCreateDescriptorPool( renderContext.m_device, &poolDesc, nullptr, &m_pool) ) );

			VkDescriptorSetAllocateInfo allocateInfo = {
				VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				nullptr,
				m_pool,
				1,
				&program.m_program->m_resourceLayout
			};

			CR_RETURN_HR( Vk2Al( vkAllocateDescriptorSets( renderContext.m_device, &allocateInfo, &m_descriptorSet ) ) );

			std::vector<VkWriteDescriptorSet> descriptorWrites;
			descriptorWrites.reserve( program.m_program->m_registerInput.size() );
			std::vector<VkDescriptorImageInfo> imageInfos;
			imageInfos.reserve( program.m_program->m_registerInput.size() );

			for( auto it = begin( program.m_program->m_registerInput ); it != end( program.m_program->m_registerInput ); ++it )
			{
				VkWriteDescriptorSet d = {
					VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					nullptr,
					m_descriptorSet,
					it->binding,
					0,
					1,
				};
				switch( it->type )
				{
				case Tr2ShaderRegisterAL::CONSTANTS:
					//d.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					continue;
				case Tr2ShaderRegisterAL::SAMPLER:
				{
					if( !description.m_samplers[description.m_registerMap.samplers[it->stage][it->registerIndex]].sampler.IsValid() )
					{
						return E_FAIL;
					}
					VkDescriptorImageInfo imageInfo = { description.m_samplers[description.m_registerMap.samplers[it->stage][it->registerIndex]].sampler.m_sampler->m_sampler };
					imageInfos.push_back( imageInfo );
					d.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
					d.pImageInfo = &imageInfos.back();
					break;
				}
				case Tr2ShaderRegisterAL::UAV:
					return E_FAIL;
				default:
					d.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					if( !description.m_srv[description.m_registerMap.srvs[it->stage][it->registerIndex]].texture.IsValid() )
					{
						return E_FAIL;
					}
					VkDescriptorImageInfo imageInfo = {  };
					imageInfo.imageView = description.m_srv[description.m_registerMap.srvs[it->stage][it->registerIndex]].texture.m_texture->m_imageViews[0];
					imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfos.push_back( imageInfo );
					d.pImageInfo = &imageInfos.back();
					break;
				}
				descriptorWrites.push_back( d );
			}
			vkUpdateDescriptorSets( renderContext.m_device, uint32_t( descriptorWrites.size() ), descriptorWrites.data(), 0, nullptr );
		}

		m_owner = &renderContext;

		return S_OK;
	}

	void Tr2ResourceSetAL::Destroy()
	{
		if( m_owner )
		{
			//m_owner->DestroyLaterVulkan( m_descriptorSets, vkFreeDescriptorSets );
			m_owner->DestroyLaterVulkan( m_pool, vkDestroyDescriptorPool );
			m_owner = nullptr;
			m_pool = VK_NULL_HANDLE;
			m_descriptorSet = VK_NULL_HANDLE;
		}
	}

	bool Tr2ResourceSetAL::IsValid() const
	{
		return m_owner != nullptr;
	}

	Tr2ALMemoryType Tr2ResourceSetAL::GetMemoryClass() const
	{
		return AL_MEMORY_VIDEO;
	}

	void Tr2ResourceSetAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2ResourceSetAL";
	}

	ALResult Tr2ResourceSetAL::SetName( const char* )
	{
		return E_NOTIMPL;
	}
}


#endif