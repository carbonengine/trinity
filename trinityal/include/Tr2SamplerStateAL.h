// Copyright © 2023 CCP ehf.

#pragma once

#include "../ALResult.h"
#include "../Tr2DeviceResourceAL.h"
#include "../Tr2ObjectFactory.h"

class Tr2PrimaryRenderContextAL;
struct Tr2SamplerDescription;
namespace TrinityALImpl
{
class Tr2SamplerStateAL;
class Tr2ResourceSetAL;
}

class Tr2SamplerStateAL
{
public:
	Tr2SamplerStateAL();
	ALResult Create( const Tr2SamplerDescription& description, Tr2PrimaryRenderContextAL& renderContext );

	uint32_t GetIndexInHeap() const;

	bool IsValid() const;

	Tr2ALMemoryType GetMemoryClass() const;

	bool operator==( const Tr2SamplerStateAL& other ) const;

	TrinityALImpl::Tr2SamplerStateAL* TrinityALImpl_GetObject() const;

	ALResult SetName( const char* name );

private:
	std::shared_ptr<TrinityALImpl::Tr2SamplerStateAL> m_sampler;
	friend class Tr2RenderContextAL;
	friend class TrinityALImpl::Tr2ResourceSetAL;
};

namespace TrinityALImpl
{
typedef Tr2ObjectFactory<Tr2SamplerStateAL, Tr2SamplerDescription> Tr2SamplerStateALFactory;
}
