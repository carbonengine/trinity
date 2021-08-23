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
	enum Type
	{
		FLOAT,
		INT,
		UINT,
		OTHER,
	};
	Tr2ShaderPipelineInputAL();
	Tr2ShaderPipelineInputAL( Tr2VertexDefinition::UsageCode usage, uint32_t usageIndex, uint32_t registerIndex, Type type, uint32_t dimension, uint32_t usedMask = 0xf );

	Tr2VertexDefinition::UsageCode usage;
	uint32_t usageIndex;
	uint32_t registerIndex;
	uint32_t usedMask;
	Type type;
	uint32_t dimension;
};


struct Tr2ShaderRegisterAL
{
	static const uint32_t SRV_REGISTER_FLAG = 1 << 5;
	static const uint32_t UAV_REGISTER_FLAG = 1 << 6;

	enum RegisterType
	{
		CONSTANT_BUFFER,
		SAMPLER,

		SRV_BUFFER = SRV_REGISTER_FLAG,
		SRV_STRUCTURED_BUFFER,
		SRV_TEXTURE1D,
		SRV_TEXTURE1DARRAY,
		SRV_TEXTURE2D,
		SRV_TEXTURE2DARRAY,
		SRV_TEXTURE2DMS,
		SRV_TEXTURE2DMSARRAY,
		SRV_TEXTURE3D,
		SRV_TEXTURECUBE,
		SRV_TEXTURECUBEARRAY,

		UAV_BUFFER = UAV_REGISTER_FLAG,
		UAV_STRUCTURED_BUFFER,
		UAV_TEXTURE1D,
		UAV_TEXTURE1DARRAY,
		UAV_TEXTURE2D,
		UAV_TEXTURE2DARRAY,
		UAV_TEXTURE2DMS,
		UAV_TEXTURE2DMSARRAY,
		UAV_TEXTURE3D,
		UAV_TEXTURECUBE,
		UAV_TEXTURECUBEARRAY,
	};

	Tr2ShaderRegisterAL();
	Tr2ShaderRegisterAL( RegisterType registerType, uint32_t registerIndex );

	bool IsSrv() const;
	bool IsUav() const;

	RegisterType registerType;
	uint32_t registerIndex;
};


struct Tr2ShaderThreadGroupSizeAL
{
	Tr2ShaderThreadGroupSizeAL() : x( 1 ), y( 1 ), z( 1 ) {}
	Tr2ShaderThreadGroupSizeAL( uint32_t x, uint32_t y, uint32_t z ) : x( x ), y( y ), z( z ) {}

	uint32_t x;
	uint32_t y;
	uint32_t z;
};


struct Tr2ShaderSignatureAL
{
	Tr2ShaderSignatureAL& Add( const Tr2ShaderPipelineInputAL& pipelineInput );
	Tr2ShaderSignatureAL& Add( Tr2VertexDefinition::UsageCode usage, uint32_t usageIndex, uint32_t registerIndex, Tr2ShaderPipelineInputAL::Type type, uint32_t dimension, uint32_t usedMask = 0xf );
	Tr2ShaderSignatureAL& Add( const Tr2ShaderRegisterAL& registerDesc );
	Tr2ShaderSignatureAL& Add( Tr2ShaderRegisterAL::RegisterType registerType, uint32_t registerIndex );
	Tr2ShaderSignatureAL& Add( const Tr2ShaderThreadGroupSizeAL& size );

	std::vector<Tr2ShaderPipelineInputAL> pipelineInputs;
	std::vector<Tr2ShaderRegisterAL> registers;
	Tr2ShaderThreadGroupSizeAL threadGroupSize;
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
