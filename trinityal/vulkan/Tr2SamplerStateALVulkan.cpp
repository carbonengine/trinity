////////////////////////////////////////////////////////////
//
//    Created:   May 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "Tr2SamplerStateALVulkan.h"
#include "Tr2PrimaryRenderContextVulkan.h"
#include "UtilitiesVulkan.h"


namespace TrinityALImpl
{
	Tr2SamplerStateAL::Tr2SamplerStateAL()
		:m_sampler( VK_NULL_HANDLE ),
		m_owner( nullptr )
	{
	}

	Tr2SamplerStateAL::~Tr2SamplerStateAL()
	{
		Destroy();
	}

	ALResult Tr2SamplerStateAL::Create( const Tr2SamplerDescription& description, Tr2PrimaryRenderContextAL &renderContext )
	{
		Destroy();
		if( !renderContext.IsValid() )
		{
			return E_INVALIDARG;
		}

		VkBool32 isAnisotropic = VK_FALSE;
		if( description.m_maxAnisotropy > 0 )
		{
			if( description.m_minFilter == Tr2RenderContextEnum::TF_ANISOTROPIC || description.m_mipFilter == Tr2RenderContextEnum::TF_ANISOTROPIC )
			{
				isAnisotropic = VK_TRUE;
			}
		}
		VkSamplerCreateInfo createInfo = {
			VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			nullptr,
			0,
			description.m_magFilter == Tr2RenderContextEnum::TF_POINT ? VK_FILTER_NEAREST : VK_FILTER_LINEAR,
			description.m_minFilter == Tr2RenderContextEnum::TF_POINT ? VK_FILTER_NEAREST : VK_FILTER_LINEAR,
			description.m_mipFilter <= Tr2RenderContextEnum::TF_POINT ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR,
			VkSamplerAddressMode( description.m_addressU - 1 ),
			VkSamplerAddressMode( description.m_addressV - 1 ),
			VkSamplerAddressMode( description.m_addressW - 1 ),
			description.m_mipLODBias,
			isAnisotropic,
			float( description.m_maxAnisotropy ),
			description.m_isComparisonFilter,
			VkCompareOp( description.m_comparisonFunc - 1),
			description.m_minLOD,
			description.m_maxLOD,
			VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
			VK_FALSE
		};

		CR_RETURN_HR( Vk2Al( vkCreateSampler( renderContext.m_device, &createInfo, nullptr, &m_sampler ) ) );
		m_owner = &renderContext;

		return S_OK;
	}

	void Tr2SamplerStateAL::Destroy()
	{
		if( m_owner )
		{
			m_owner->DestroyLaterVulkan( m_sampler, vkDestroySampler );
			m_sampler = VK_NULL_HANDLE;
			m_owner = nullptr;
		}
	}

	bool Tr2SamplerStateAL::IsValid() const
	{
		return m_sampler != VK_NULL_HANDLE;
	}

	Tr2ALMemoryType Tr2SamplerStateAL::GetMemoryClass() const
	{
		return AL_MEMORY_VIDEO;
	}

	void Tr2SamplerStateAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
	{
		description["type"] = "Tr2SamplerStateAL";
	}

	ALResult Tr2SamplerStateAL::SetName( const char* )
	{
		return E_NOTIMPL;
	}
}


#endif