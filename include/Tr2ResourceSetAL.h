#pragma once

#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"
#include "Tr2BufferAL.h"
#include "Tr2TextureAL.h"
#include "Tr2SamplerStateAL.h"

class Tr2PrimaryRenderContextAL;

namespace TrinityALImpl
{
	class Tr2ResourceSetAL;
}

class Tr2ResourceSetDescriptionAL
{
public:
	static const uint32_t MAX_RESOURCES_IN_STAGE = 32;

	bool Set( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer );
	bool Set( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, Tr2RenderContextEnum::ColorSpace colorSpace = Tr2RenderContextEnum::COLOR_SPACE_LINEAR );
	bool Set( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler );

	bool operator==( const Tr2ResourceSetDescriptionAL& other ) const;
private:
	enum ResourceType
	{
		NONE,
		BUFFER,
		TEXTURE,
	};

	struct Resource
	{
		Resource();

		bool operator==( const Resource& other ) const;

		Tr2TextureAL texture;
		Tr2BufferAL buffer;
		ResourceType type;
		Tr2RenderContextEnum::ColorSpace colorSpace;
	};

	struct Sampler
	{
		Sampler();

		bool operator==( const Sampler& other ) const;

		Tr2SamplerStateAL sampler;
		bool assigned;
	};

	Resource m_resources[Tr2RenderContextEnum::SHADER_TYPE_COUNT][MAX_RESOURCES_IN_STAGE];
	Sampler m_samplers[Tr2RenderContextEnum::SHADER_TYPE_COUNT][MAX_RESOURCES_IN_STAGE];

	friend class TrinityALImpl::Tr2ResourceSetAL;
};

class Tr2ResourceSetAL
{
public:
	Tr2ResourceSetAL();

	ALResult Create( const Tr2ResourceSetDescriptionAL& description, Tr2PrimaryRenderContextAL& renderContext );
	bool IsValid() const;

	Tr2ALMemoryType GetMemoryClass() const;
private:
	std::shared_ptr<TrinityALImpl::Tr2ResourceSetAL> m_resourceSet;

	friend class Tr2RenderContextAL;
};