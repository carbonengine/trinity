// Copyright © 2023 CCP ehf.

#pragma once

#if ( TRINITY_PLATFORM == TRINITY_METAL )

#include "../include/Tr2SamplerStateAL.h"
#include "MetalContext.h"

namespace TrinityALImpl
{
class Tr2SamplerStateAL : public Tr2DeviceResourceAL<Tr2SamplerStateAL>
{
public:
	Tr2SamplerStateAL();

	ALResult Create( const Tr2SamplerDescription& description, Tr2RenderContextAL& renderContext );
	void Destroy();

	uint32_t GetIndexInHeap() const;

	bool IsValid() const;

	Tr2ALMemoryType GetMemoryClass() const
	{
		return AL_MEMORY_MANAGED;
	}

	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	ALResult SetName( const char* name );

	id<MTLSamplerState> GetMetalSamplerState()
	{
		return m_mtlSamplerState;
	};

private:
	MetalContext* m_metalContext;
	id<MTLSamplerState> m_mtlSamplerState;
	uint32_t m_heapIndex;
	std::string m_name;
};
}

#endif
