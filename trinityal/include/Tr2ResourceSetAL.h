#pragma once

#include "../ALResult.h"
#include "../Tr2DeviceResourceAL.h"
#include "Tr2BufferAL.h"
#include "Tr2TextureAL.h"
#include "Tr2SamplerStateAL.h"

class Tr2ShaderAL;
class Tr2ShaderProgramAL;
class Tr2PrimaryRenderContextAL;
struct Tr2ShaderSignatureAL;

namespace TrinityALImpl
{
	class Tr2ResourceSetAL;
}

struct Tr2RegisterMapAL
{
	Tr2RegisterMapAL();
	Tr2RegisterMapAL( Tr2RenderContextEnum::ShaderType stage, const Tr2ShaderSignatureAL& signature );
	Tr2RegisterMapAL( const Tr2ShaderAL* shaders, size_t shaderCount );
	Tr2RegisterMapAL( const Tr2RenderContextEnum::ShaderType* shaders, const Tr2ShaderSignatureAL* signatures, size_t signatureCount );

	bool operator==( const Tr2RegisterMapAL& other ) const;
	bool operator!=( const Tr2RegisterMapAL& other ) const;

	static const uint32_t MAX_RESOURCES_IN_STAGE = 32;

	uint32_t srvCount;
	uint32_t uavCount;
	uint32_t samplerCount;
	uint8_t srvs[Tr2RenderContextEnum::SHADER_TYPE_COUNT][MAX_RESOURCES_IN_STAGE];
	uint8_t uavs[Tr2RenderContextEnum::SHADER_TYPE_COUNT][MAX_RESOURCES_IN_STAGE];
	uint8_t samplers[Tr2RenderContextEnum::SHADER_TYPE_COUNT][MAX_RESOURCES_IN_STAGE];
};


class Tr2ResourceSetDescriptionAL
{
public:
	static const uint32_t MAX_RESOURCES_IN_STAGE = 32;

	Tr2ResourceSetDescriptionAL();
	Tr2ResourceSetDescriptionAL( const Tr2ResourceSetDescriptionAL& );
	Tr2ResourceSetDescriptionAL( Tr2ResourceSetDescriptionAL&& );
	explicit Tr2ResourceSetDescriptionAL( const Tr2ShaderProgramAL& program );
	explicit Tr2ResourceSetDescriptionAL( const Tr2RegisterMapAL& registers );
	Tr2ResourceSetDescriptionAL& operator=( const Tr2ResourceSetDescriptionAL& other );
	Tr2ResourceSetDescriptionAL& operator=( Tr2ResourceSetDescriptionAL&& other );

	bool SetSrv( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer );
	bool SetSrv( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, Tr2RenderContextEnum::ColorSpace colorSpace = Tr2RenderContextEnum::COLOR_SPACE_LINEAR );
	bool SetUav( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer );
	bool SetUav( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, uint32_t mip = 0 );
	bool SetSampler( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler );
	void ClearResources();
	
	uint32_t ComputeHash() const;

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
		bool Is( const Tr2BufferAL& other ) const;
		bool Is( const Tr2TextureAL& other, Tr2RenderContextEnum::ColorSpace otherColorSpace ) const;
		bool Is( const Tr2TextureAL& other, uint32_t otherMip ) const;
		void UpdateHash( uint32_t& hash ) const;

		Tr2TextureAL texture;
		Tr2BufferAL buffer;
		ResourceType type;
		union
		{
			Tr2RenderContextEnum::ColorSpace colorSpace;
			uint32_t mip;
		};
	};

	struct Sampler
	{
		Sampler();

		bool operator==( const Sampler& other ) const;
		bool operator==( const Tr2SamplerStateAL& other ) const;

		void UpdateHash( uint32_t& hash ) const;

		Tr2SamplerStateAL sampler;
		bool assigned;
	};

	Tr2RegisterMapAL m_registerMap;
	std::unique_ptr<Resource[]> m_srv;
	std::unique_ptr<Resource[]> m_uav;
	std::unique_ptr<Sampler[]> m_samplers;

	friend class TrinityALImpl::Tr2ResourceSetAL;
};

class Tr2ResourceSetAL
{
public:
	Tr2ResourceSetAL();

	ALResult Create( const Tr2ResourceSetDescriptionAL& description, const Tr2ShaderProgramAL& program, Tr2PrimaryRenderContextAL& renderContext );
	bool IsValid() const;

	Tr2ALMemoryType GetMemoryClass() const;

	ALResult SetName( const char* name );

private:
	std::shared_ptr<TrinityALImpl::Tr2ResourceSetAL> m_resourceSet;

	friend class Tr2RenderContextAL;
};
