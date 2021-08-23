#include "StdAfx.h"
#include "../include/Tr2ResourceSetAL.h"
#include "../include/Tr2TextureAL.h"
#include "../include/Tr2ShaderAL.h"
#include "../include/Tr2ShaderProgramAL.h"


#include TRINITY_AL_PLATFORM_INCLUDE( Tr2ResourceSetAL )


namespace
{

template <typename T>
void HashResourcePtr( const T& resource, uint32_t& hash )
{
	auto p = resource.TrinityALImpl_GetObject();
	hash = CcpHashFNV1( &p, sizeof( p ), hash );
}

}


Tr2RegisterMapAL::Tr2RegisterMapAL() : // cppcheck-suppress uninitMemberVar
	srvCount( 0 ),
	uavCount( 0 ),
	samplerCount( 0 )
{
}

Tr2RegisterMapAL::Tr2RegisterMapAL( Tr2RenderContextEnum::ShaderType stage, const Tr2ShaderSignatureAL& signature ) :
	srvCount( 0 ),
	uavCount( 0 ),
	samplerCount( 0 )
{
	memset( srvs, -1, sizeof( srvs ) );
	memset( uavs, -1, sizeof( uavs ) );
	memset( samplers, -1, sizeof( samplers ) );

	for( auto it = begin( signature.registers ); it != end( signature.registers ); ++it )
	{
		if( it->IsSrv() )
		{
			srvs[stage][it->registerIndex] = uint8_t( srvCount++ );
		}
		else if( it->IsUav() )
		{
			uavs[stage][it->registerIndex] = uint8_t( uavCount++ );
		}
		else if( it->registerType == Tr2ShaderRegisterAL::SAMPLER )
		{
			samplers[stage][it->registerIndex] = uint8_t( samplerCount++ );
		}
	}
}

Tr2RegisterMapAL::Tr2RegisterMapAL( const Tr2ShaderAL* shaders, size_t shaderCount ) :
	srvCount( 0 ),
	uavCount( 0 ),
	samplerCount( 0 )
{
	memset( srvs, -1, sizeof( srvs ) );
	memset( uavs, -1, sizeof( uavs ) );
	memset( samplers, -1, sizeof( samplers ) );

	for( size_t i = 0; i < shaderCount; ++i )
	{
		auto shaderType = shaders[i].GetType();
		auto& signature = shaders[i].GetSignature();
		for( auto it = begin( signature.registers ); it != end( signature.registers ); ++it )
		{
			if( it->IsSrv() )
			{
				srvs[shaderType][it->registerIndex] = uint8_t( srvCount++ );
			}
			else if( it->IsUav() )
			{
				uavs[shaderType][it->registerIndex] = uint8_t( uavCount++ );
			}
			else if( it->registerType == Tr2ShaderRegisterAL::SAMPLER )
			{
				samplers[shaderType][it->registerIndex] = uint8_t( samplerCount++ );
			}
		}
	}
}

Tr2RegisterMapAL::Tr2RegisterMapAL( const Tr2RenderContextEnum::ShaderType* shaders, const Tr2ShaderSignatureAL* signatures, size_t signatureCount ) :
	srvCount( 0 ),
	uavCount( 0 ),
	samplerCount( 0 )
{
	memset( srvs, -1, sizeof( srvs ) );
	memset( uavs, -1, sizeof( uavs ) );
	memset( samplers, -1, sizeof( samplers ) );

	for( size_t i = 0; i < signatureCount; ++i )
	{
		for( auto it = begin( signatures[i].registers ); it != end( signatures[i].registers ); ++it )
		{
			if( it->IsSrv() )
			{
				srvs[shaders[i]][it->registerIndex] = uint8_t( srvCount++ );
			}
			else if( it->IsUav() )
			{
				uavs[shaders[i]][it->registerIndex] = uint8_t( uavCount++ );
			}
			else if( it->registerType == Tr2ShaderRegisterAL::SAMPLER )
			{
				samplers[shaders[i]][it->registerIndex] = uint8_t( samplerCount++ );
			}
		}
	}
}

bool Tr2RegisterMapAL::operator==( const Tr2RegisterMapAL& other ) const
{
	if( srvCount != other.srvCount || uavCount != other.uavCount || samplerCount != other.samplerCount )
	{
		return false;
	}
	if( srvCount > 0 && memcmp( srvs, other.srvs, sizeof( srvs ) ) )
	{
		return false;
	}
	if( uavCount > 0 && memcmp( uavs, other.uavs, sizeof( uavs ) ) )
	{
		return false;
	}
	if( samplerCount > 0 && memcmp( samplers, other.samplers, sizeof( samplers ) ) )
	{
		return false;
	}
	return true;
}

bool Tr2RegisterMapAL::operator!=( const Tr2RegisterMapAL& other ) const
{
	return !( *this == other );
}


Tr2ResourceSetDescriptionAL::Tr2ResourceSetDescriptionAL()
{
}

Tr2ResourceSetDescriptionAL::Tr2ResourceSetDescriptionAL( const Tr2ResourceSetDescriptionAL& other ) :
	m_registerMap( other.m_registerMap )
{
	if( m_registerMap.srvCount > 0 )
	{
		m_srv.reset( new Resource[m_registerMap.srvCount] );
		std::copy_n( other.m_srv.get(), m_registerMap.srvCount, m_srv.get() );
	}
	if( m_registerMap.uavCount > 0 )
	{
		m_uav.reset( new Resource[m_registerMap.uavCount] );
		std::copy_n( other.m_uav.get(), m_registerMap.uavCount, m_uav.get() );
	}
	if( m_registerMap.samplerCount > 0 )
	{
		m_samplers.reset( new Sampler[m_registerMap.samplerCount] );
		std::copy_n( other.m_samplers.get(), m_registerMap.samplerCount, m_samplers.get() );
	}
}

Tr2ResourceSetDescriptionAL::Tr2ResourceSetDescriptionAL( Tr2ResourceSetDescriptionAL&& other ) :
	m_registerMap( other.m_registerMap )
{
	std::swap( m_srv, other.m_srv );
	std::swap( m_uav, other.m_uav );
	std::swap( m_samplers, other.m_samplers );
}

Tr2ResourceSetDescriptionAL::Tr2ResourceSetDescriptionAL( const Tr2ShaderProgramAL& program ) :
	m_registerMap( program.GetRegisterMap() )
{
	if( m_registerMap.srvCount > 0 )
	{
		m_srv.reset( new Resource[m_registerMap.srvCount] );
	}
	if( m_registerMap.uavCount > 0 )
	{
		m_uav.reset( new Resource[m_registerMap.uavCount] );
	}
	if( m_registerMap.samplerCount > 0 )
	{
		m_samplers.reset( new Sampler[m_registerMap.samplerCount] );
	}
}

Tr2ResourceSetDescriptionAL::Tr2ResourceSetDescriptionAL( const Tr2RegisterMapAL& registers ) :
	m_registerMap( registers )
{
	if( m_registerMap.srvCount > 0 )
	{
		m_srv.reset( new Resource[m_registerMap.srvCount] );
	}
	if( m_registerMap.uavCount > 0 )
	{
		m_uav.reset( new Resource[m_registerMap.uavCount] );
	}
	if( m_registerMap.samplerCount > 0 )
	{
		m_samplers.reset( new Sampler[m_registerMap.samplerCount] );
	}
}

Tr2ResourceSetDescriptionAL& Tr2ResourceSetDescriptionAL::operator=( const Tr2ResourceSetDescriptionAL& other )
{
	if( &other == this )
	{
		return *this;
	}
	m_registerMap = other.m_registerMap;
	if( m_registerMap.srvCount > 0 )
	{
		m_srv.reset( new Resource[m_registerMap.srvCount] );
		std::copy_n( other.m_srv.get(), m_registerMap.srvCount, m_srv.get() );
	}
	else
	{
		m_srv.reset();
	}
	if( m_registerMap.uavCount > 0 )
	{
		m_uav.reset( new Resource[m_registerMap.uavCount] );
		std::copy_n( other.m_uav.get(), m_registerMap.uavCount, m_uav.get() );
	}
	else
	{
		m_uav.reset();
	}
	if( m_registerMap.samplerCount > 0 )
	{
		m_samplers.reset( new Sampler[m_registerMap.samplerCount] );
		std::copy_n( other.m_samplers.get(), m_registerMap.samplerCount, m_samplers.get() );
	}
	else
	{
		m_samplers.reset();
	}
	return *this;
}

Tr2ResourceSetDescriptionAL& Tr2ResourceSetDescriptionAL::operator=( Tr2ResourceSetDescriptionAL&& other )
{
	if( &other == this )
	{
		return *this;
	}
	m_registerMap = other.m_registerMap;
	std::swap( m_srv, other.m_srv );
	other.m_srv.reset();
	std::swap( m_uav, other.m_uav );
	other.m_uav.reset();
	std::swap( m_samplers, other.m_samplers );
	other.m_samplers.reset();
	return *this;
}


bool Tr2ResourceSetDescriptionAL::SetSrv( Tr2RenderContextEnum::ShaderType stage, uint32_t registerIndex, const Tr2BufferAL& buffer )
{
	if( m_registerMap.srvCount == 0 )
	{
		return false;
	}
	auto index = m_registerMap.srvs[stage][registerIndex];
	if( index >= m_registerMap.srvCount )
	{
		return false;
	}
	auto& resource = m_srv[index];
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
	if( m_registerMap.srvCount == 0 )
	{
		return false;
	}
	auto index = m_registerMap.srvs[stage][registerIndex];
	if( index >= m_registerMap.srvCount )
	{
		return false;
	}
	auto& resource = m_srv[index];
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
	if( m_registerMap.uavCount == 0 )
	{
		return false;
	}
	auto index = m_registerMap.uavs[stage][registerIndex];
	if( index >= m_registerMap.uavCount )
	{
		return false;
	}
	auto& resource = m_uav[index];
	if( resource.Is( buffer, initialCount ) )
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
	if( m_registerMap.uavCount == 0 )
	{
		return false;
	}
	auto index = m_registerMap.uavs[stage][registerIndex];
	if( index >= m_registerMap.uavCount )
	{
		return false;
	}
	auto& resource = m_uav[index];
	if( resource.Is( texture, mip ) )
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
	if( m_registerMap.samplerCount == 0 )
	{
		return false;
	}
	auto index = m_registerMap.samplers[stage][registerIndex];
	if( index >= m_registerMap.samplerCount )
	{
		return false;
	}
	auto& resource = m_samplers[index];
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
	for( uint32_t i = 0; i < m_registerMap.srvCount; ++i )
	{
		auto& srv = m_srv[i];
		srv.type = NONE;
		srv.texture = Tr2TextureAL();
		srv.buffer = Tr2BufferAL();
	}
	for( uint32_t i = 0; i < m_registerMap.uavCount; ++i )
	{
		auto& uav = m_uav[i];
		uav.type = NONE;
		uav.texture = Tr2TextureAL();
		uav.buffer = Tr2BufferAL();
	}
}

uint32_t Tr2ResourceSetDescriptionAL::ComputeHash() const
{
	uint32_t hash = 0;
	for( uint32_t i = 0; i < m_registerMap.srvCount; ++i )
	{
		m_srv[i].UpdateHash( hash );
	}
	for( uint32_t i = 0; i < m_registerMap.uavCount; ++i )
	{
		m_uav[i].UpdateHash( hash );
	}
	for( uint32_t i = 0; i < m_registerMap.samplerCount; ++i )
	{
		m_samplers[i].UpdateHash( hash );
	}
	return hash;
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

void Tr2ResourceSetDescriptionAL::Resource::UpdateHash( uint32_t& hash ) const
{
	if( type == BUFFER )
	{
		HashResourcePtr( buffer, hash );
	}
	else if( type == TEXTURE )
	{
		HashResourcePtr( texture, hash );
	}
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

void Tr2ResourceSetDescriptionAL::Sampler::UpdateHash( uint32_t& hash ) const
{
	if( assigned )
	{
		HashResourcePtr( sampler, hash );
	}
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
