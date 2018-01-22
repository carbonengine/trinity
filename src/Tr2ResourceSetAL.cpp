#include "StdAfx.h"
#include "../include/Tr2ResourceSetAL.h"
#include "../include/Tr2TextureAL.h"

#include TRINITY_AL_PLATFORM_INCLUDE( Tr2ResourceSetAL )


bool Tr2ResourceSetDescriptionAL::Set( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer )
{
	auto resource = Resource();
	resource.type = BUFFER;
	resource.buffer = buffer;
	resource.colorSpace = Tr2RenderContextEnum::COLOR_SPACE_LINEAR;

	if( m_resources[stage][registerIndex] == resource )
	{
		return false;
	}
	m_resources[stage][registerIndex] = resource;
	return true;
}

bool Tr2ResourceSetDescriptionAL::Set( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, Tr2RenderContextEnum::ColorSpace colorSpace )
{
	auto resource = Resource();
	resource.type = TEXTURE;
	resource.texture = texture;
	resource.colorSpace = colorSpace;

	if( m_resources[stage][registerIndex] == resource )
	{
		return false;
	}
	m_resources[stage][registerIndex] = resource;
	return true;
}

bool Tr2ResourceSetDescriptionAL::Set( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler )
{
	auto resource = Sampler();
	resource.sampler = sampler;
	resource.assigned = true;

	if( m_samplers[stage][registerIndex] == resource )
	{
		return false;
	}
	m_samplers[stage][registerIndex] = resource;
	return true;
}

bool Tr2ResourceSetDescriptionAL::operator==( const Tr2ResourceSetDescriptionAL& other ) const
{
	return m_resources == other.m_resources && m_samplers == other.m_samplers;
}


Tr2ResourceSetDescriptionAL::Resource::Resource()
	:type( NONE ),
	colorSpace( Tr2RenderContextEnum::COLOR_SPACE_LINEAR )
{
}

bool Tr2ResourceSetDescriptionAL::Resource::operator==( const Resource& other ) const
{
	return type == other.type && buffer == other.buffer && texture == other.texture;
}

Tr2ResourceSetDescriptionAL::Sampler::Sampler()
	:assigned( false )
{
}

bool Tr2ResourceSetDescriptionAL::Sampler::operator==( const Sampler& other ) const
{
	return sampler == other.sampler && assigned == other.assigned;
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
