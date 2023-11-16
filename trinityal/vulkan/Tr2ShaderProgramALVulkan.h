////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "../include/Tr2ShaderProgramAL.h"
#include "../include/Tr2ShaderAL.h"
#include "../include/Tr2ResourceSetAL.h"

namespace TrinityALImpl
{
	class Tr2ShaderProgramAL : public Tr2DeviceResourceAL<Tr2ShaderProgramAL>
	{
	public:
		Tr2ShaderProgramAL();
		~Tr2ShaderProgramAL();

		ALResult Create( ::Tr2ShaderAL* shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext );
		void Destroy();
		bool IsValid() const;
		const Tr2RegisterMapAL& GetRegisterMap() const;

		Tr2ALMemoryType GetMemoryClass() const;
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

	private:
		std::vector<VkPipelineShaderStageCreateInfo> m_shaderInfo;
		std::vector<::Tr2ShaderAL> m_shaders;
		std::vector<Tr2ShaderPipelineInputAL> m_shaderInputs;

		std::vector<VkDescriptorPoolSize> m_poolSizes;

		VkPipelineLayout m_pipelineLayout;

		VkDescriptorSetLayout m_resourceLayout;
		VkDescriptorSetLayout m_constantLayout;

		//std::vector<VkDescriptorSetLayout> m_layouts;
		Tr2PrimaryRenderContextAL* m_owner;

		struct RegisterInput
		{
			uint32_t binding;
			uint32_t stage;
			uint32_t registerIndex;

			Tr2ShaderRegisterAL::RegisterType type;
		};
		std::vector<RegisterInput> m_registerInput;
		Tr2RegisterMapAL m_registerMap;

		friend class Tr2RenderContextAL;
		friend class TrinityALImpl::Tr2ResourceSetAL;
	};
}
#endif