#pragma once

#include "../ALResult.h"
#include "../Tr2TrackedALObject.h"

class Tr2PrimaryRenderContextAL;

namespace TrinityALImpl
{
	class Tr2ResourceSetAL;
}

class Tr2ResourceSetDescriptionAL
{
public:
	void Set( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2GpuBufferAL& buffer );
	void Set( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, Tr2RenderContextEnum::ColorSpace colorSpace = Tr2RenderContextEnum::COLOR_SPACE_LINEAR );
	void Set( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler );

	bool operator==( const Tr2ResourceSetDescriptionAL& other ) const;
private:
	enum ResourceType
	{
		NONE,
		BUFFER,
		TEXTURE,
		SAMPLER,
	};

	struct Resource
	{
		bool operator==( const Resource& other ) const;

		union
		{
			const Tr2GpuBufferAL* buffer;
			const Tr2TextureAL* texture;
			const Tr2SamplerStateAL* sampler;
		};
		ResourceType type;
		Tr2RenderContextEnum::ColorSpace colorSpace;
	};

	struct Key
	{
		Key( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex );

		bool operator==( const Key& other ) const;
		bool operator<( const Key& other ) const;

		Tr2RenderContextEnum::ShaderType stage;
		uint32_t registerIndex;
	};

	std::map<Key, Resource> m_resources;
	std::map<Key, Resource> m_samplers;

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