#include "StdAfx.h"
#include "../include/Tr2ResourceSetAL.h"
#include "../include/Tr2TextureAL.h"

#include TRINITY_AL_PLATFORM_INCLUDE( Tr2ResourceSetAL )


bool Tr2ResourceSetDescriptionAL::SetSrv( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer )
{
	auto& resource = m_srv[stage][registerIndex];
	if( resource.Is( buffer, 0 ) )
	{
		return false;
	}
	resource.type = BUFFER;
	resource.buffer = buffer;
	resource.initialCount = 0;
	return true;
}

bool Tr2ResourceSetDescriptionAL::SetSrv( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, Tr2RenderContextEnum::ColorSpace colorSpace )
{
	auto& resource = m_srv[stage][registerIndex];
	if( resource.Is( texture, colorSpace ) )
	{
		return false;
	}
	resource.type = TEXTURE;
	resource.texture = texture;
	resource.colorSpace = colorSpace;
	return true;
}

bool Tr2ResourceSetDescriptionAL::SetUav( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer, uint32_t initialCount )
{
	auto& resource = m_uav[stage][registerIndex];
	if( m_uav[stage][registerIndex].Is( buffer, initialCount ) )
	{
		return false;
	}
	resource.type = BUFFER;
	resource.buffer = buffer;
	resource.initialCount = initialCount;
	return true;
}

bool Tr2ResourceSetDescriptionAL::SetUav( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2TextureAL& texture, uint32_t mip )
{
	auto& resource = m_uav[stage][registerIndex];
	if( m_uav[stage][registerIndex].Is( texture, mip ) )
	{
		return false;
	}
	resource.type = TEXTURE;
	resource.texture = texture;
	resource.mip = mip;
	return true;
}

bool Tr2ResourceSetDescriptionAL::SetSampler( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2SamplerStateAL& sampler )
{
	auto& resource = m_samplers[stage][registerIndex];
	if( resource == sampler )
	{
		return false;
	}
	resource.sampler = sampler;
	resource.assigned = true;
	return true;
}

bool Tr2ResourceSetDescriptionAL::operator==( const Tr2ResourceSetDescriptionAL& other ) const
{
	return m_srv == other.m_srv && m_uav == other.m_uav && m_samplers == other.m_samplers;
}

void Tr2ResourceSetDescriptionAL::ClearResources()
{
	for( auto sit = std::begin( m_srv ); sit != std::end( m_srv ); ++sit )
	{
		for( auto rit = std::begin( *sit ); rit != std::end( *sit ); ++rit )
		{
			rit->type = NONE;
			rit->texture = Tr2TextureAL();
			rit->buffer = Tr2BufferAL();
		}
	}
	for( auto sit = std::begin( m_uav ); sit != std::end( m_uav ); ++sit )
	{
		for( auto rit = std::begin( *sit ); rit != std::end( *sit ); ++rit )
		{
			rit->type = NONE;
			rit->texture = Tr2TextureAL();
			rit->buffer = Tr2BufferAL();
		}
	}
}


Tr2ResourceSetDescriptionAL::Resource::Resource()
	:type( NONE ),
	colorSpace( Tr2RenderContextEnum::COLOR_SPACE_LINEAR )
{
}

bool Tr2ResourceSetDescriptionAL::Resource::operator==( const Resource& other ) const
{
	return type == other.type && buffer == other.buffer && texture == other.texture && initialCount == other.initialCount;
}

bool Tr2ResourceSetDescriptionAL::Resource::Is( const Tr2BufferAL& other, uint32_t otherInitialCount ) const
{
	return type == BUFFER && buffer == other && initialCount == otherInitialCount;
}

bool Tr2ResourceSetDescriptionAL::Resource::Is( const Tr2TextureAL& other, Tr2RenderContextEnum::ColorSpace otherColorSpace ) const
{
	return type == TEXTURE && texture == other && colorSpace == otherColorSpace;
}

bool Tr2ResourceSetDescriptionAL::Resource::Is( const Tr2TextureAL& other, uint32_t otherMip ) const
{
	return type == TEXTURE && texture == other && mip == otherMip;
}

Tr2ResourceSetDescriptionAL::Sampler::Sampler()
	:assigned( false )
{
}

bool Tr2ResourceSetDescriptionAL::Sampler::operator==( const Sampler& other ) const
{
	return sampler == other.sampler && assigned == other.assigned;
}

bool Tr2ResourceSetDescriptionAL::Sampler::operator==( const Tr2SamplerStateAL& other ) const
{
	return sampler == other && assigned;
}


namespace 
{
	std::shared_ptr<TrinityALImpl::Tr2ResourceSetAL> nullRS = std::make_shared<TrinityALImpl::Tr2ResourceSetAL>();
}


Tr2ResourceSetAL::Tr2ResourceSetAL()
	:m_resourceSet( nullRS )
{
}

ALResult Tr2ResourceSetAL::Create( const Tr2ResourceSetDescriptionAL& description, const Tr2ShaderProgramAL& program, Tr2PrimaryRenderContextAL& renderContext )
{
	m_resourceSet = std::make_shared<TrinityALImpl::Tr2ResourceSetAL>();
	auto result = m_resourceSet->Create( description, program, renderContext );
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
