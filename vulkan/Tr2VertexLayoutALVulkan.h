////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once

#if TRINITY_PLATFORM == TRINITY_VULKAN
// -------------------------------------------------------------
// Description
//   Class to convert a platform agnostic Tr2VertexDeclaration to a DX11 specific
//   declaration, and build a ID3D11InputLayout out of it.
//   Not intended to be used directly, instead use Tr2VertexDefinition, apply
//   the definition to the Tr2EffectStateManager, and let things happen behind the scenes.
// -------------------------------------------------------------

#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"
#include "../Tr2VertexDefinition.h"
#include "../include/Tr2ShaderAL.h"


class Tr2PrimaryRenderContextAL;
class Tr2ShaderAL;


class Tr2VertexLayoutAL : public Tr2TrackedALObject<Tr2RenderContextEnum::OT_VERTEX_LAYOUT>
{
public:
	Tr2VertexLayoutAL();

	ALResult Create( const Tr2VertexDefinition& definition, Tr2PrimaryRenderContextAL& renderContext );
	bool IsValid() const;
	void Destroy();

	Tr2ALMemoryType GetMemoryClass() const;

	void PopulateInputLayoutVulkan( std::vector<VkVertexInputAttributeDescription>& layout, const std::vector<Tr2ShaderPipelineInputAL>& shaderInputs ) const;
private:
	Tr2VertexDefinition m_definition;
	std::vector<VkVertexInputAttributeDescription> m_attributes;
	VkVertexInputRate m_streamRates[4];
	uint32_t m_streamCount;
	bool m_isValid;

	friend class Tr2RenderContextAL;
};

#endif
