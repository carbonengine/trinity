// Copyright © 2023 CCP ehf.

#pragma once

#include "../Tr2RenderContextEnum.h"

class Tr2ShaderAL;
struct Tr2ShaderSignatureAL;

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
