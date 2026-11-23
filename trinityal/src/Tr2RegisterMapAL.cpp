// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "../include/Tr2RegisterMapAL.h"
#include "../include/Tr2ShaderAL.h"


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
