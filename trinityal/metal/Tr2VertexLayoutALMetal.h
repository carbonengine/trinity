// Copyright © 2023 CCP ehf.

#pragma once

#if ( TRINITY_PLATFORM == TRINITY_METAL )

#include "../include/Tr2VertexLayoutAL.h"
#include "../Tr2VertexDefinition.h"
#include "Tr2ShaderProgramALMetal.h"
#include "MetalContext.h"

struct Tr2ShaderPipelineInputAL;

namespace TrinityALImpl
{
class Tr2VertexLayoutAL : public Tr2DeviceResourceAL<Tr2VertexLayoutAL>
{
public:
	Tr2VertexLayoutAL();
	~Tr2VertexLayoutAL();

	ALResult Create( const Tr2VertexDefinition& definition, Tr2RenderContextAL& renderContext );
	bool IsValid() const;
	void Destroy();

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}
	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	ALResult SetName( const char* name );

	void SetVertexLayout( const std::shared_ptr<TrinityALImpl::Tr2ShaderProgramAL>& shaderProgram, Tr2RenderContextAL& renderContext );
	void AddVertexDescriptor( size_t inputHash, MTLVertexDescriptor* descriptor, uint8_t streamMask, bool needsDummyStream );

private:
	Tr2VertexLayoutAL( const Tr2VertexLayoutAL& ) = delete;
	Tr2VertexLayoutAL& operator=( const Tr2VertexLayoutAL& ) = delete;

	struct MetalVertexDescriptor
	{
		MTLVertexDescriptor* descriptor;
		size_t baseHash;
		uint8_t streamMask;
		bool needsDummyStream;
	};

	struct Item
	{
		Tr2VertexDefinition::UsageCode m_usage;
		unsigned m_usageIndex;
		unsigned m_offset;
		unsigned m_stream;

		MTLVertexFormat m_format;
		MTLVertexStepFunction m_stepFunction;
		unsigned m_stepRate;
	};

	MetalVertexDescriptor GenerateVertexDescriptor( const std::vector<Tr2ShaderPipelineInputAL>& shaderInputs ) const;
	void CalculateHash( MetalVertexDescriptor& result ) const;

	std::vector<Item> m_items;
	Tr2RenderContextAL* m_renderContext;
	std::unordered_map<size_t, MetalVertexDescriptor> m_metalVertexDescriptors;
	std::string m_name;
};
}

#endif
