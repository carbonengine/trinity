// Copyright © 2023 CCP ehf.

#pragma once

#include <vector>

#include "../ALResult.h"
#include "../Tr2RenderContextEnum.h"
#include "Tr2SamplerStateAL.h"

class Tr2RenderContextAL;

class Tr2StaticResourceBindingsAL
{
public:
	bool SetSampler( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler );
	bool SetSrvHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex );
	bool SetUavHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex );
	bool SetSamplerHeapView( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex );
	void Clear();
	bool IsEmpty() const;

	bool operator==( const Tr2StaticResourceBindingsAL& other ) const;

	ALResult Apply( Tr2RenderContextAL& renderContext ) const;

private:
	enum Kind : uint8_t
	{
		KIND_SAMPLER,
		KIND_SRV_HEAP_VIEW,
		KIND_UAV_HEAP_VIEW,
		KIND_SAMPLER_HEAP_VIEW,
	};

	struct Entry
	{
		Tr2SamplerStateAL sampler;
		uint8_t stage;
		uint8_t registerIndex;
		Kind kind;
	};

	static bool SharesRegisterSpace( Kind a, Kind b );

	bool Set( Kind kind, Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler );

	std::vector<Entry> m_entries;
};
