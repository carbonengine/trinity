#include "StdAfx.h"
#include "../include/Tr2ResourceSetAL.h"

#include TRINITY_AL_PLATFORM_INCLUDE( Tr2ResourceSetAL )


void Tr2ResourceSetDescriptionAL::Set( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2GpuBufferAL& buffer )
{
	auto key = Key( stage, registerIndex );
	auto resource = Resource();
	resource.type = BUFFER;
	resource.buffer = &buffer;
	resource.colorSpace = Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
	m_resources[key] = resource;
}

void Tr2ResourceSetDescriptionAL::Set( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, Tr2RenderContextEnum::ColorSpace colorSpace )
{
	auto key = Key( stage, registerIndex );
	auto resource = Resource();
	resource.type = TEXTURE;
	resource.texture = &texture;
	resource.colorSpace = colorSpace;
	m_resources[key] = resource;
}

void Tr2ResourceSetDescriptionAL::Set( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler )
{
	auto key = Key( stage, registerIndex );
	auto resource = Resource();
	resource.type = SAMPLER;
	resource.sampler = &sampler;
	resource.colorSpace = Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
	m_samplers[key] = resource;
}

bool Tr2ResourceSetDescriptionAL::operator==( const Tr2ResourceSetDescriptionAL& other ) const
{
	return m_resources == other.m_resources && m_samplers == other.m_samplers;
}

Tr2ResourceSetDescriptionAL::Key::Key( Tr2RenderContextEnum::ShaderType stage_, uint32_t registerIndex_ )
	:stage( stage_ ),
	registerIndex( registerIndex_ )
{
}

bool Tr2ResourceSetDescriptionAL::Key::operator==( const Key& other ) const
{
	return stage == other.stage && registerIndex == other.registerIndex;
}

bool Tr2ResourceSetDescriptionAL::Key::operator<( const Key& other ) const
{
	if( stage < other.stage )
	{
		return true;
	}
	if( stage > other.stage )
	{
		return false;
	}
	return registerIndex < other.registerIndex;
}

bool Tr2ResourceSetDescriptionAL::Resource::operator==( const Resource& other ) const
{
	return type == other.type && buffer == other.buffer;
}


namespace 
{
	std::shared_ptr<TrinityALImpl::Tr2ResourceSetAL> nullRS = std::make_shared<TrinityALImpl::Tr2ResourceSetAL>();
}


Tr2ResourceSetAL::Tr2ResourceSetAL()
	:m_resourceSet( nullRS )
{
}

ALResult Tr2ResourceSetAL::Create( const Tr2ResourceSetDescriptionAL& description, Tr2PrimaryRenderContextAL& renderContext )
{
	m_resourceSet = std::make_shared<TrinityALImpl::Tr2ResourceSetAL>();
	auto result = m_resourceSet->Create( description, renderContext );
	if( FAILED( result ) )
	{
		m_resourceSet = nullRS;
	}
	return result;
}

bool Tr2ResourceSetAL::IsValid() const
{
	return m_resourceSet->IsValid();
}

Tr2ALMemoryType Tr2ResourceSetAL::GetMemoryClass() const
{
	return m_resourceSet->GetMemoryClass();
}
