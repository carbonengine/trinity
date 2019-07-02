////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN

#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"
#include "../include/Tr2ShaderAL.h"

class Tr2ShaderProgramAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_SHADER_PROGRAM>
{
public:
	ALResult Create( Tr2ShaderAL* shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();
	bool IsValid() const;

	Tr2ALMemoryType GetMemoryClass() const;
private:
	std::vector<VkPipelineShaderStageCreateInfo> m_shaderInfo;
	std::vector<Tr2ShaderAL> m_shaders;
	std::vector<Tr2ShaderPipelineInputAL> m_shaderInputs;

	friend class Tr2RenderContextAL;
};

#endif