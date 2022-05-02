////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "../include/Tr2VertexLayoutAL.h"
#include "../Tr2VertexDefinition.h"
#include "../include/Tr2ShaderAL.h"


namespace TrinityALImpl
{
	class Tr2VertexLayoutAL : public Tr2DeviceResourceAL<Tr2VertexLayoutAL>
	{
	public:
		Tr2VertexLayoutAL();

		ALResult Create( const Tr2VertexDefinition& definition, Tr2PrimaryRenderContextAL& renderContext );
		bool IsValid() const;
		void Destroy();

		Tr2ALMemoryType GetMemoryClass() const;

		void PopulateInputLayoutVulkan( std::vector<VkVertexInputAttributeDescription>& layout, const std::vector<Tr2ShaderPipelineInputAL>& shaderInputs ) const;
		void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
		ALResult SetName( const char* name );

	private:
		Tr2VertexDefinition m_definition;
		std::vector<VkVertexInputAttributeDescription> m_attributes;
		VkVertexInputRate m_streamRates[4];
		uint32_t m_streamCount;
		bool m_isValid;

		friend class Tr2RenderContextAL;
	};
}

#endif
