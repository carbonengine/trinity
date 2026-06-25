// Copyright © 2019 CCP ehf.

#pragma once

#if TRINITY_PLATFORM == TRINITY_DIRECTX12

#include "../include/Tr2SamplerStateAL.h"
#include "./util/DescriptorHeapViewDx12.h"


namespace TrinityALImpl
{
class Tr2RtShaderTableAL;

class Tr2SamplerStateAL : public Tr2DeviceResourceAL<Tr2SamplerStateAL>
{
public:
	Tr2SamplerStateAL();

	ALResult Create( const Tr2SamplerDescription& description, Tr2PrimaryRenderContextAL& renderContext );

	uint32_t GetIndexInHeap() const;

	void Destroy();

	bool IsValid() const;
	Tr2ALMemoryType GetMemoryClass() const;
	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;
	ALResult SetName( const char* name );

private:
	std::shared_ptr<SamplerStateDx12> m_samplerState;
	uint32_t m_indexInHeap;
	D3D12_SAMPLER_DESC m_sampler;
	std::string m_name;
	bool m_isValid;

	friend class Tr2RenderContextAL;
	friend class Tr2ResourceSetAL;
	friend class TrinityALImpl::Tr2RtShaderTableAL;
};

}

#endif
