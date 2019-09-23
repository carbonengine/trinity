#pragma once

#include "../ALResult.h"
#include "../Tr2DeviceResourceAL.h"
#include "Tr2BufferAL.h"
#include "Tr2TextureAL.h"
#include "Tr2SamplerStateAL.h"

class Tr2ShaderProgramAL;
class Tr2PrimaryRenderContextAL;

namespace TrinityALImpl
{
	class Tr2ResourceSetAL;
}

class Tr2ResourceSetDescriptionAL
{
public:
	static const uint32_t MAX_RESOURCES_IN_STAGE = 32;

	bool SetSrv( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer );
	bool SetSrv( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, Tr2RenderContextEnum::ColorSpace colorSpace = Tr2RenderContextEnum::COLOR_SPACE_LINEAR );
	bool SetUav( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer, uint32_t initialCount = -1 );
	bool SetUav( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, uint32_t mip = 0 );
	bool SetSampler( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler );
	void ClearResources();

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
		bool Is( const Tr2BufferAL& other, uint32_t otherInitialCount ) const;
		bool Is( const Tr2TextureAL& other, Tr2RenderContextEnum::ColorSpace otherColorSpace ) const;
		bool Is( const Tr2TextureAL& other, uint32_t otherMip ) const;

		Tr2TextureAL texture;
		Tr2BufferAL buffer;
		ResourceType type;
		union
		{
			Tr2RenderContextEnum::ColorSpace colorSpace;
			uint32_t mip;
			uint32_t initialCount;
		};
	};

	struct Sampler
	{
		Sampler();

		bool operator==( const Sampler& other ) const;
		bool operator==( const Tr2SamplerStateAL& other ) const;

		Tr2SamplerStateAL sampler;
		bool assigned;
	};

	Resource m_srv[Tr2RenderContextEnum::SHADER_TYPE_COUNT][MAX_RESOURCES_IN_STAGE];
	Resource m_uav[Tr2RenderContextEnum::SHADER_TYPE_COUNT][MAX_RESOURCES_IN_STAGE];
	Sampler m_samplers[Tr2RenderContextEnum::SHADER_TYPE_COUNT][MAX_RESOURCES_IN_STAGE];

	friend class TrinityALImpl::Tr2ResourceSetAL;
};

class Tr2ResourceSetAL
{
public:
	Tr2ResourceSetAL();

	ALResult Create( const Tr2ResourceSetDescriptionAL& description, const Tr2ShaderProgramAL& program, Tr2PrimaryRenderContextAL& renderContext );
	bool IsValid() const;

	Tr2ALMemoryType GetMemoryClass() const;
private:
	std::shared_ptr<TrinityALImpl::Tr2ResourceSetAL> m_resourceSet;

	friend class Tr2RenderContextAL;
};