#include "StdAfx.h"
#include "../include/Tr2SamplerStateAL.h"
#include "../include/Tr2PrimaryRenderContextAL.h"
#include TRINITY_AL_PLATFORM_INCLUDE( Tr2SamplerStateAL )

namespace
{
	std::shared_ptr<TrinityALImpl::Tr2SamplerStateAL> nullSampler = std::make_shared<TrinityALImpl::Tr2SamplerStateAL>();
}

// Force anisotropic filtering switch:
// 0 means use whatever is specified in the effect (default)
// 1 means turn off anisotropic filtering (fallback to linear)
// >1 - force max anisotropy to specified number
uint32_t g_forceAnisotropy = 0;


Tr2SamplerStateAL::Tr2SamplerStateAL()
	:m_sampler( nullSampler )
{
}

ALResult Tr2SamplerStateAL::Create( const Tr2SamplerDescription& description, Tr2PrimaryRenderContextAL &renderContext )
{
	return renderContext.m_samplerStateFactory.Get( m_sampler, description, renderContext );
}

bool Tr2SamplerStateAL::IsValid() const
{
	return m_sampler->IsValid();
}

Tr2ALMemoryType Tr2SamplerStateAL::GetMemoryClass() const
{
	return m_sampler->GetMemoryClass();
}

bool Tr2SamplerStateAL::operator==( const Tr2SamplerStateAL& other ) const
{
	return m_sampler == other.m_sampler;
}

TrinityALImpl::Tr2SamplerStateAL* Tr2SamplerStateAL::TrinityALImpl_GetObject() const
{
	return m_sampler.get();
}
