// Copyright © 2023 CCP ehf.

#pragma once

#include "../Tr2DeviceResourceAL.h"
#include "../ALResult.h"
#include "../Tr2RenderContextEnum.h"

class Tr2ShaderAL;
class Tr2PrimaryRenderContextAL;
struct Tr2RegisterMapAL;
namespace TrinityALImpl
{
class Tr2ShaderProgramAL;
class Tr2ResourceSetAL;
class PSODescription;
}

struct Tr2ConstantBufferRegisterAL
{
	Tr2RenderContextEnum::ShaderType stage;
	uint32_t registerIndex;
};
struct Tr2IndirectBufferLayoutAL
{
	Tr2ConstantBufferRegisterAL constantBuffers[8];
	uint32_t constantBufferCount = 0;
	uint32_t drawArgOffset = 0;
	uint32_t drawStride = 0;
	uint32_t drawIndexedStride = 0;
};

class Tr2ShaderProgramAL
{
public:
	Tr2ShaderProgramAL();

	ALResult Create( Tr2ShaderAL* shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext );
	ALResult CreateCommandSignatures( Tr2IndirectBufferLayoutAL& bufferLayout, Tr2PrimaryRenderContextAL& renderContext );

	bool IsValid() const;
	Tr2ALMemoryType GetMemoryClass() const;

	bool operator==( const Tr2ShaderProgramAL& other ) const;

	const Tr2RegisterMapAL& GetRegisterMap() const;

	ALResult SetName( const char* name );

	TrinityALImpl::Tr2ShaderProgramAL* TrinityALImpl_GetObject() const;

private:
	std::shared_ptr<TrinityALImpl::Tr2ShaderProgramAL> m_program;

	friend class Tr2RenderContextAL;
	friend class Tr2PrimaryRenderContextAL;
	friend class TrinityALImpl::Tr2ResourceSetAL;
	friend class TrinityALImpl::PSODescription;
};
