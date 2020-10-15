#pragma once

#include "../Tr2DeviceResourceAL.h"
#include "../ALResult.h"
#include "../Tr2RenderContextEnum.h"
#include "../Tr2VertexDefinition.h"


struct Tr2ShaderBytecodeAL
{
	Tr2ShaderBytecodeAL();
	Tr2ShaderBytecodeAL( const void* bytecode, size_t size );

	template <typename T, size_t Size>
	Tr2ShaderBytecodeAL( const T( &bytecode_ )[Size] )
		:bytecode( bytecode_ ),
		size( Size * sizeof( T ) )
	{
	}

	const void* bytecode;
	size_t size;
};


struct Tr2ShaderPipelineInputAL
{
	Tr2ShaderPipelineInputAL();
	Tr2ShaderPipelineInputAL( Tr2VertexDefinition::UsageCode usage, uint32_t usageIndex, uint32_t registerIndex, uint32_t usedMask = 0xf );

	Tr2VertexDefinition::UsageCode usage;
	uint32_t usageIndex;
	uint32_t registerIndex;
	uint32_t usedMask;
};


struct Tr2ShaderRegisterAL
{
	enum RegisterType
	{
		CONSTANTS,
		RESOURCE,
		UAV,
		SAMPLER,
	};

	Tr2ShaderRegisterAL();
	Tr2ShaderRegisterAL( RegisterType registerType, uint32_t registerIndex );

	RegisterType registerType;
	uint32_t registerIndex;
};


struct Tr2ShaderSignatureAL
{
	Tr2ShaderSignatureAL& Add( const Tr2ShaderPipelineInputAL& pipelineInput );
	Tr2ShaderSignatureAL& Add( Tr2VertexDefinition::UsageCode usage, uint32_t usageIndex, uint32_t registerIndex, uint32_t usedMask = 0xf );
	Tr2ShaderSignatureAL& Add( const Tr2ShaderRegisterAL& registerDesc );
	Tr2ShaderSignatureAL& Add( Tr2ShaderRegisterAL::RegisterType registerType, uint32_t registerIndex );

	std::vector<Tr2ShaderPipelineInputAL> pipelineInputs;
	std::vector<Tr2ShaderRegisterAL> registers;
};


class Tr2ShaderAL;

namespace TrinityALImpl
{
	class Tr2ShaderAL;
	class Tr2ShaderProgramAL;
}

class Tr2PrimaryRenderContextAL;

class Tr2ShaderAL
{
public:
	Tr2ShaderAL();

	ALResult Create(
		Tr2RenderContextEnum::ShaderType type,
		const Tr2ShaderBytecodeAL& bytecode,
		const Tr2ShaderBytecodeAL& patchedBytecode,
		const Tr2ShaderSignatureAL& signature,
		Tr2PrimaryRenderContextAL &renderContext
	);

	ALResult Create(
		Tr2RenderContextEnum::ShaderType type,
		const Tr2ShaderBytecodeAL& bytecode,
		const Tr2ShaderSignatureAL& signature,
		Tr2PrimaryRenderContextAL &renderContext
	);

	bool IsValid() const;

	Tr2RenderContextEnum::ShaderType GetType() const;
	ALResult GetBytecode( Tr2ShaderBytecodeAL& bytecode ) const;
	const Tr2ShaderSignatureAL& GetSignature() const;

	bool operator==( const Tr2ShaderAL& other ) const;
	bool operator!=( const Tr2ShaderAL& other ) const;
private:
	Tr2ShaderAL( std::shared_ptr<TrinityALImpl::Tr2ShaderAL> shader );

	std::shared_ptr<TrinityALImpl::Tr2ShaderAL> m_shader;

	friend class Tr2RenderContextAL;
	friend class TrinityALImpl::Tr2ShaderProgramAL;
};
