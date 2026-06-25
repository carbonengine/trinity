// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#if TRINITY_PLATFORM == TRINITY_DIRECTX11

#include "Tr2ResourceSetALDx11.h"
#include "Tr2BufferALDx11.h"
#include "Tr2TextureALDx11.h"
#include "Tr2SamplerStateALDx11.h"
#include "../include/Tr2ShaderProgramAL.h"

using namespace Tr2RenderContextEnum;

namespace TrinityALImpl
{
Tr2ResourceSetAL::Tr2ResourceSetAL() :
	m_isValid( false ),
	m_empty( true ),
	m_uavCount( 0 ),
	m_csUavs( false )
{
}

ALResult Tr2ResourceSetAL::Create( const Tr2ResourceSetDescriptionAL& description, const ::Tr2ShaderProgramAL& program, Tr2PrimaryRenderContextAL& /*renderContext*/ )
{
	Destroy();

	if( program.GetRegisterMap() != description.m_registerMap )
	{
		return E_INVALIDARG;
	}

	ON_BLOCK_EXIT_WITH_UNUSED( [&] { if( !IsValid() ) Destroy(); } );

	for( auto it = std::begin( m_stages ); it != std::end( m_stages ); ++it )
	{
		it->resourceOffset = MAX_RESOURCES;
		it->samplerOffset = MAX_RESOURCES;
	}
	bool hasPsUavs = false;
	m_uavOffset = MAX_RESOURCES;
	for( uint32_t stageIndex = 0; stageIndex < SHADER_TYPE_COUNT; ++stageIndex )
	{
		auto& stage = m_stages[stageIndex];
		for( uint32_t registerIndex = 0; registerIndex < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++registerIndex )
		{
			if( description.m_registerMap.srvs[stageIndex][registerIndex] >= description.m_registerMap.srvCount )
			{
				continue;
			}
			auto& desc = description.m_srv[description.m_registerMap.srvs[stageIndex][registerIndex]];
			switch( desc.type )
			{
			case Tr2ResourceSetDescriptionAL::Resource::BUFFER:
				stage.resources[registerIndex] = desc.buffer.m_buffer->m_srv;
				break;
			case Tr2ResourceSetDescriptionAL::Resource::TEXTURE:
				stage.resources[registerIndex] = desc.texture.m_texture->m_view[desc.colorSpace];
				break;
			case Tr2ResourceSetDescriptionAL::Resource::NONE:
				continue;
			default:
				return E_INVALIDARG;
			}
			stage.resourceOffset = std::min( stage.resourceOffset, registerIndex );
			stage.resourceCount = std::max( stage.resourceCount, registerIndex + 1 );
		}
		for( uint32_t registerIndex = 0; registerIndex < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++registerIndex )
		{
			if( description.m_registerMap.samplers[stageIndex][registerIndex] >= description.m_registerMap.samplerCount )
			{
				continue;
			}
			auto& desc = description.m_samplers[description.m_registerMap.samplers[stageIndex][registerIndex]];
			if( desc.type == Tr2ResourceSetDescriptionAL::Sampler::SAMPLER )
			{
				stage.samplers[registerIndex] = desc.sampler.m_sampler->m_samplerState;
				stage.samplerOffset = std::min( stage.samplerOffset, registerIndex );
				stage.samplerCount = std::max( stage.samplerCount, registerIndex + 1 );
			}
		}
		for( uint32_t registerIndex = 0; registerIndex < Tr2ResourceSetDescriptionAL::MAX_RESOURCES_IN_STAGE; ++registerIndex )
		{
			if( description.m_registerMap.uavs[stageIndex][registerIndex] >= description.m_registerMap.uavCount )
			{
				continue;
			}
			auto& desc = description.m_uav[description.m_registerMap.uavs[stageIndex][registerIndex]];
			if( desc.type == Tr2ResourceSetDescriptionAL::Resource::NONE )
			{
				continue;
			}
			if( stageIndex != PIXEL_SHADER && stageIndex != COMPUTE_SHADER )
			{
				return E_INVALIDARG;
			}
			if( stageIndex == PIXEL_SHADER )
			{
				hasPsUavs = true;
			}
			else if( hasPsUavs )
			{
				return E_INVALIDARG;
			}
			switch( desc.type )
			{
			case Tr2ResourceSetDescriptionAL::Resource::BUFFER:
				m_uavs[registerIndex] = desc.buffer.m_buffer->m_uav;
				m_uavCount = registerIndex + 1;
				m_uavOffset = std::min( m_uavOffset, registerIndex );
				break;
			case Tr2ResourceSetDescriptionAL::Resource::TEXTURE:
				if( desc.mip < desc.texture.m_texture->m_uav.size() )
				{
					m_uavs[registerIndex] = desc.texture.m_texture->m_uav[desc.mip];
				}
				else
				{
					m_uavs[registerIndex] = nullptr;
				}
				m_uavCount = registerIndex + 1;
				m_uavOffset = std::min( m_uavOffset, registerIndex );
				break;
			default:
				return E_INVALIDARG;
			}
			m_csUavs = stageIndex == COMPUTE_SHADER;
		}
	}

	if( m_uavCount )
	{
		m_empty = false;
		m_uavCount -= m_uavOffset;
	}
	else
	{
		m_empty = true;
		m_uavOffset = 0;
	}

	for( auto it = std::begin( m_stages ); it != std::end( m_stages ); ++it )
	{
		if( it->resourceCount > it->resourceOffset )
		{
			it->resourceHash = 0;
			for( uint32_t i = it->resourceOffset; i != it->resourceCount; ++i )
			{
				it->resourceHash ^= uint32_t( reinterpret_cast<uintptr_t>( it->resources[i].p ) & 0xffffffff ) << i;
			}
			it->resourceCount -= it->resourceOffset;
			m_empty = false;
		}
		else
		{
			it->resourceCount = 0;
			it->resourceHash = 0;
		}
		if( it->samplerCount > it->samplerOffset )
		{
			it->samplerHash = 0;
			for( uint32_t i = it->samplerOffset; i != it->samplerCount; ++i )
			{
				it->samplerHash ^= uint32_t( reinterpret_cast<uintptr_t>( it->samplers[i].p ) & 0xffffffff ) << i;
			}
			it->samplerCount -= it->samplerOffset;
			m_empty = false;
		}
		else
		{
			it->samplerCount = 0;
			it->samplerHash = 0;
		}
	}
	m_isValid = true;
	return S_OK;
};

bool Tr2ResourceSetAL::IsValid() const
{
	return m_isValid;
}

void Tr2ResourceSetAL::Destroy()
{
	for( auto it = std::begin( m_stages ); it != std::end( m_stages ); ++it )
	{
		it->Destroy();
	}
	std::fill_n( m_uavs, MAX_RESOURCES, nullptr );
	m_uavCount = 0;
	m_uavOffset = 0;
	m_isValid = false;
	m_empty = true;
	m_csUavs = false;
}

Tr2ALMemoryType Tr2ResourceSetAL::GetMemoryClass() const
{
	return AL_MEMORY_MANAGED;
}


Tr2ResourceSetAL::StageInput::StageInput() :
	resourceCount( 0 ),
	resourceOffset( 0 ),
	samplerCount( 0 ),
	samplerOffset( 0 ),
	resourceHash( 0 ),
	samplerHash( 0 )
{
}

void Tr2ResourceSetAL::StageInput::Destroy()
{
	std::fill_n( resources, MAX_RESOURCES, nullptr );
	std::fill_n( samplers, MAX_RESOURCES, nullptr );
	resourceCount = 0;
	samplerCount = 0;
	resourceHash = 0;
	samplerHash = 0;
}

void Tr2ResourceSetAL::Describe( Tr2DeviceResourceDescriptionAL& description ) const
{
	description["type"] = "Tr2ResourceSetAL";
	description["name"] = m_name;
}

ALResult Tr2ResourceSetAL::SetName( const char* name )
{
	m_name = name;
	return S_OK;
}
}

#endif